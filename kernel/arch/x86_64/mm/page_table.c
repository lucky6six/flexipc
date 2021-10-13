#include <mm/mm.h>
#include <mm/vmspace.h>
#include <mm/kmalloc.h>
#include <common/kprint.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>

#include "page_table.h"

/*
 * Important:
 * We do not flush any TLB when adding new mappings in map_range_in_pgtbl.
 * This is because TLB will not cache empty mappings.
 * Prerequisite: ChCore does not directly change mappings (without unmap).
 */
#define NO_FLUSH_FOR_ADD_MAPPING 1
// TODO: bug on test_malloc.bin
// #define NO_FLUSH_FOR_ADD_MAPPING 0

/* An external function defined in tlb.c */
extern void flush_tlb_local_and_remote(struct vmspace*, vaddr_t, size_t);
extern void invpcid_flush_single_context(u64);

/*
 * Inst/data cache on x86_64 are consistent.
 */
void flush_idcache(void)
{
	/* empty */
}

/*
 * Write cr3 is needed for context switching.
 * Function set_page_table only changes CR3 without flushing TLBs.
 *
 * Chcore enables PCID.
 * We should not rely on the following function to flush TLB.
 */
void set_page_table(paddr_t pgtbl)
{
	/* set the highest bit: do not flush TLB */
	pgtbl |= (1UL << 63);
	asm volatile("mov %0, %%cr3\n\t" : : "r"(pgtbl) : );
}

paddr_t get_page_table(void)
{
	paddr_t pgtbl;

	asm volatile("mov %%cr3, %0\n\t" :"=r"(pgtbl) : : );
	return pgtbl;
}

#define USER_PTE 0
/*
 * the 3rd arg means the kind of PTE (user or kernel PTE)
 */
static int set_pte_flags(pte_t *entry, vmr_prop_t flags, int kind)
{
	/* For enabling MPK, we set U bit everywhere */
	BUG_ON(kind != USER_PTE);
	entry->pteval |= PAGE_USER;

	if (flags & VMR_WRITE)
		entry->pteval |= PAGE_RW;
		/* equals: entry->pte_4K.writeable = 1; */
	else
		entry->pteval &= (~PAGE_RW);
		/* equals: entry->pte_4K.writeable = 0; */

	if (flags & VMR_EXEC)
		entry->pteval &= (~PAGE_NX);
		/* equals: entry->pte_4K.nx = 0; */
	else
		entry->pteval |= PAGE_NX;
		/* equals: entry->pte_4K.nx = 1; */

	// TODO: set memory type
	return 0;
}

#define NORMAL_PTP (0)
#define BLOCK_PTP  (1) /* huge page */

/*
 * Find next page table page for the "va".
 *
 * cur_ptp: current page table page
 * level:   current ptp level
 *
 * next_ptp: returns "next_ptp"
 * pte     : returns "pte" (points to next_ptp) in "cur_ptp"
 *
 * alloc: if true, allocate a ptp when missing
 *
 */
int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va,
			ptp_t **next_ptp, pte_t **pte, bool alloc)
{
	u32 index = 0;
	pte_t *entry;

	if (cur_ptp == NULL)
		return -ENOMAPPING;

	switch (level) {
	case 0:
		index = GET_L0_INDEX(va);
		break;
	case 1:
		index = GET_L1_INDEX(va);
		break;
	case 2:
		index = GET_L2_INDEX(va);
		break;
	case 3:
		index = GET_L3_INDEX(va);
		break;
	default:
		BUG_ON(1);
	}

	entry = &(cur_ptp->ent[index]);
	/* if not present */
	if (!(entry->pteval & PAGE_PRESENT)) {
		if (alloc == false) {
			return -ENOMAPPING;
		}
		else {
			/* alloc a new page table page */
			ptp_t *new_ptp;
			paddr_t new_ptp_paddr;
			pte_t new_pte_val;

			/* get 2^0 physical page */
			new_ptp = (ptp_t *)get_pages(0);
			BUG_ON(new_ptp == NULL);
			memset((void *)new_ptp, 0, PAGE_SIZE);
			new_ptp_paddr = virt_to_phys((void *)new_ptp);

			new_pte_val.pteval = 0;
			new_pte_val.table.present = 1;
			/* For enabling MPK, set U-bit in every level page table */
			new_pte_val.table.user = 1;
			new_pte_val.table.writeable = 1;

			new_pte_val.table.next_table_addr
				= new_ptp_paddr >> PAGE_SHIFT;
			entry->pteval = new_pte_val.pteval;
		}
	}

	*next_ptp = (ptp_t *)GET_NEXT_PTP(entry);
	*pte = entry;

	/* whether it is a PTP or a page (BLOCK_PTP) */
	if ((level == 3) || (entry->table.is_page))
		return BLOCK_PTP;
	else
		return NORMAL_PTP;
}

int debug_query_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	ptp_t *phys_page;
	pte_t *pte;
	int ret;

	// L0 page table
	l0_ptp = (ptp_t *)remove_pcid(pgtbl);
	ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L0 no mapping.\n");
		return ret;
	}
	printk("L0 pte is 0x%lx\n", pte->pteval);

	// L1 page table
	ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L1 no mapping.\n");
		return ret;
	}
	printk("L1 pte is 0x%lx\n", pte->pteval);

	// L2 page table
	ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L2 no mapping.\n");
		return ret;
	}
	printk("L2 pte is 0x%lx\n", pte->pteval);

	// L3 page table
	ret = get_next_ptp(l3_ptp, 3, va, &phys_page, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L3 no mapping.\n");
		return ret;
	}
	printk("L3 pte is 0x%lx\n", pte->pteval);

	*pa = virt_to_phys((void *)phys_page) + GET_VA_OFFSET_L3(va);
	*entry = pte;
	return 0;
}

void sys_debug_va(u64 va)
{
	u64 cr3;
	vaddr_t *pgtbl;
	paddr_t pa;
	pte_t *entry;
	int ret;

#if CHCORE
	asm volatile ("mov %%cr3, %0":"=r" (cr3));
#else
	cr3 = 0;
#endif

	pgtbl = (vaddr_t *)phys_to_virt(cr3);
	ret = debug_query_in_pgtbl(pgtbl, va, &pa, &entry);
	if (ret < 0)
		printk("[sys_debug_va] va is not mapped.\n");
	else
		printk("[sys_debug_va] va 0x%lx --> pa 0x%lx\n", va, pa);
}


int query_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
	/*
	 * On x86_64, pml4 is the highest level page table.
	 *
	 * To make the code similar with that in aarch64,
	 * we use l0_ptp to represent the high level page table.
	 *
	 */
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	ptp_t *phys_page;
	pte_t *pte;
	int ret;

	/* L0 page table / pml4 */
	l0_ptp = (ptp_t *)remove_pcid(pgtbl);
	ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
	if (ret < 0)
		return ret;

	/* L1 page table / pdpte */
	ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
	if (ret < 0)
		return ret;
	else if (ret == BLOCK_PTP) { /* 1G huge page */
		*pa = virt_to_phys((void *)l2_ptp) +
			GET_VA_OFFSET_L1(va);

		if (entry)
			*entry = pte;

		return 0;
	}

	/* L2 page table / pde */
	ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
	if (ret < 0)
		return ret;
	else if (ret == BLOCK_PTP) { /* 2M huge page */
		*pa = virt_to_phys((void *)l3_ptp) +
			GET_VA_OFFSET_L2(va);

		if (entry)
			*entry = pte;

		return 0;
	}

	/* L3 page table / pte */
	ret = get_next_ptp(l3_ptp, 3, va, &phys_page, &pte, false);
	if (ret < 0)
		return ret;

	*pa = virt_to_phys((void *)phys_page) + GET_VA_OFFSET_L3(va);
	if (entry)
		*entry = pte;
	return 0;
}

int map_range_in_pgtbl(struct vmspace *vmspace, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags)
{
	vaddr_t *pgtbl;
	s64 total_page_cnt;
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	pte_t *pte;
	int ret;
	/* the index of pte in the last level page table */
	int pte_index;
	int i;

	/* root page table page must exist */
	pgtbl = vmspace->pgtbl;
	BUG_ON(pgtbl == NULL);
	/* va should be page aligned. */
	BUG_ON(va % PAGE_SIZE);
	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);

	l0_ptp = (ptp_t *)remove_pcid(pgtbl);

	l1_ptp = NULL;
	l2_ptp = NULL;
	l3_ptp = NULL;

	while (total_page_cnt > 0) {
		// l0
		ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, true);
		if (ret != 0) printk("ret: %d\n", ret);
		BUG_ON(ret != 0);

		// l1
		ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, true);
		if (ret != 0) printk("ret: %d\n", ret);
		BUG_ON(ret != 0);

		// l2
		ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, true);
		BUG_ON(ret != 0);

		// l3
		// step-1: get the index of pte
		pte_index = GET_L3_INDEX(va);
		for (i = pte_index; i < PTP_ENTRIES; ++i) {
			pte_t new_pte_val;

			new_pte_val.pteval = 0;

			new_pte_val.pte_4K.present = 1;
			new_pte_val.pte_4K.pfn = pa >> PAGE_SHIFT;

			set_pte_flags(&new_pte_val, flags, USER_PTE);
			l3_ptp->ent[i].pteval = new_pte_val.pteval;

			va += PAGE_SIZE;
			pa += PAGE_SIZE;
			total_page_cnt -= 1;
			if (total_page_cnt == 0)
				break;
		}
	}

	/* Without "ifdef CHCORE", unit test will failed due to invpcid */
	#ifdef CHCORE
	/* without PCID, flush_tlb_all is used */
	// flush_tlb_all();

	/* with PCID, flush_tlb_all is used */
	// invpcid_flush_single_context(get_pcid(pgtbl));


	/* A conservative way: flushing TLB when adding mappings */
	#if NO_FLUSH_FOR_ADD_MAPPING == 0
		flush_tlb_local_and_remote(vmspace, va, len);
	#endif

	#endif

	return 0;
}

/*
 * TODO: free physical pages and page tables
 */
int unmap_range_in_pgtbl(struct vmspace *vmspace, vaddr_t va, size_t len)
{
	vaddr_t *pgtbl;
	s64 total_page_cnt; // must be signed
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	pte_t *pte;
	int ret;
	int pte_index; // the index of pte in the last level page table
	int i;

	pgtbl = vmspace->pgtbl;
	if (pgtbl == NULL) return 0;
	l0_ptp = (ptp_t *)remove_pcid(pgtbl);

	BUG_ON(va % PAGE_SIZE);

	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);
	while (total_page_cnt > 0) {
		/* l0 */
		ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L0_PER_ENTRY_PAGES;
			va += L0_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		/* l1 */
		ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L1_PER_ENTRY_PAGES;
			va += L1_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		/* l2 */
		ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L2_PER_ENTRY_PAGES;
			va += L2_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		/* l3 */
		/* get the index of pte */
		pte_index = GET_L3_INDEX(va);
		for (i = pte_index; i < PTP_ENTRIES; ++i) {
			/* clear the pte */
			l3_ptp->ent[i].pteval = 0;
			va += PAGE_SIZE;
			total_page_cnt -= 1;
			if (total_page_cnt == 0)
				break;
		}
	}

	/* Without "ifdef CHCORE", unit test will failed due to invpcid */
	#ifdef CHCORE
	/* without PCID, flush_tlb_all is used */
	// flush_tlb_all();

	/* with PCID, flush_tlb_all is used */
	// invpcid_flush_single_context(get_pcid(pgtbl));

	flush_tlb_local_and_remote(vmspace, va, len);
	#endif

	return 0;
}

/*
 * Foring supportting mprotect:
 *	- scan the page table and modify the PTEs if they exist
 */
int mprotect_in_pgtbl(struct vmspace *vmspace, vaddr_t va, size_t len, vmr_prop_t flags)
{
	vaddr_t *pgtbl;
	s64 total_page_cnt; // must be signed
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	pte_t *pte;
	int ret;
	int pte_index; // the index of pte in the last level page table
	int i;

	pgtbl = vmspace->pgtbl;
	if (pgtbl == NULL) return 0;
	l0_ptp = (ptp_t *)remove_pcid(pgtbl);

	BUG_ON(va % PAGE_SIZE);

	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);
	while (total_page_cnt > 0) {
		/* l0 */
		ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L0_PER_ENTRY_PAGES;
			va += L0_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		/* l1 */
		ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L1_PER_ENTRY_PAGES;
			va += L1_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		/* l2 */
		ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L2_PER_ENTRY_PAGES;
			va += L2_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		/* l3 */
		/* get the index of pte */
		pte_index = GET_L3_INDEX(va);
		for (i = pte_index; i < PTP_ENTRIES; ++i) {

			/*
			 * Modify the permission in the pte if it exists.
			 * Usually, some ptes exist, so flush tlb is necessary.
			 */
			if (l3_ptp->ent[i].pteval & PAGE_PRESENT)
				set_pte_flags(&(l3_ptp->ent[i]), flags, USER_PTE);

			va += PAGE_SIZE;
			total_page_cnt -= 1;
			if (total_page_cnt == 0)
				break;
		}
	}

	/* Without "ifdef CHCORE", unit test will failed due to invpcid */
	#ifdef CHCORE
	/* without PCID, flush_tlb_all is used */
	// flush_tlb_all();

	/* with PCID, flush_tlb_all is used */
	// invpcid_flush_single_context(get_pcid(pgtbl));

	flush_tlb_local_and_remote(vmspace, va, len);
	#endif

	return 0;
}
