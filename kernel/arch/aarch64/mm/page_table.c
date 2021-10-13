#ifdef CHCORE
#include <common/util.h>
#include <mm/kmalloc.h>
#endif
#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>
#include <lib/printk.h>
#include <mm/vmspace.h>
#include <mm/mm.h>
#include <arch/mmu.h>

#include "page_table.h"

/* Page_table.c: Use simple impl for now. */
// TODO: add hugepage support for user space.

extern void set_ttbr0_el1(paddr_t);
extern void flush_tlb(void);

void set_page_table(paddr_t pgtbl)
{
	set_ttbr0_el1(pgtbl);
}

#define USER_PTE 0
/*
 * the 3rd arg means the kind of PTE.
 */
static int set_pte_flags(pte_t *entry, vmr_prop_t flags, int kind)
{
	// TODO: only consider USER PTE now
	BUG_ON(kind != USER_PTE);

	// TODO: XOM. VMR_READ is default now for user page table.
	// TODO: EL1 can directly access EL0 now.
	if (flags & VMR_WRITE)
		entry->l3_page.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
	else
		entry->l3_page.AP = ARM64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;

	if (flags & VMR_EXEC)
		entry->l3_page.UXN = ARM64_MMU_ATTR_PAGE_UX;
	else
		entry->l3_page.UXN = ARM64_MMU_ATTR_PAGE_UXN;

	// EL1 cannot directly execute EL0 accessiable region.
	entry->l3_page.PXN = ARM64_MMU_ATTR_PAGE_PXN;
	// TODO: we set AF in advance.
	entry->l3_page.AF  = ARM64_MMU_ATTR_PAGE_AF_ACCESSED;

	// not global
	//entry->l3_page.nG = 1;
	// inner sharable
	entry->l3_page.SH = INNER_SHAREABLE;
	// memory type
	if (flags & VMR_DEVICE)
		entry->l3_page.attr_index = DEVICE_MEMORY;
	else
		entry->l3_page.attr_index = NORMAL_MEMORY;

	return 0;
}

#define GET_PADDR_IN_PTE(entry) \
	(((u64)entry->table.next_table_addr) << PAGE_SHIFT)
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define NORMAL_PTP (0)
#define BLOCK_PTP  (1)

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
static int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va,
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
	if (IS_PTE_INVALID(entry->pte)) {
		if (alloc == false) {
			return -ENOMAPPING;
		}
		else {
			/* alloc a new page table page */
			ptp_t *new_ptp;
			paddr_t new_ptp_paddr;
			pte_t new_pte_val;

			/* alloc a single physical page as a new page table page */
			new_ptp = get_pages(0);
			BUG_ON(new_ptp == NULL);
			memset((void *)new_ptp, 0, PAGE_SIZE);
			new_ptp_paddr = virt_to_phys((vaddr_t)new_ptp);

			new_pte_val.pte = 0;
			new_pte_val.table.is_valid = 1;
			new_pte_val.table.is_table = 1;
			new_pte_val.table.next_table_addr
				= new_ptp_paddr >> PAGE_SHIFT;

			/* same effect as: cur_ptp->ent[index] = new_pte_val; */
			entry->pte = new_pte_val.pte;
		}
	}

	// TODO: is GET_NEXT_PTP still correct for huge page?
	*next_ptp = (ptp_t *)GET_NEXT_PTP(entry);
	*pte = entry;
	if (IS_PTE_TABLE(entry->pte))
		return NORMAL_PTP;
	else
		return BLOCK_PTP;
}

int debug_query_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	ptp_t *phys_page;
	pte_t *pte;
	int ret;

	// L0 page table
	l0_ptp = (ptp_t *)pgtbl;
	ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L0 no mapping.\n");
		return ret;
	}
	printk("L0 pte is 0x%lx\n", pte->pte);

	// L1 page table
	ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L1 no mapping.\n");
		return ret;
	}
	printk("L1 pte is 0x%lx\n", pte->pte);

	// L2 page table
	ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L2 no mapping.\n");
		return ret;
	}
	printk("L2 pte is 0x%lx\n", pte->pte);

	// L3 page table
	ret = get_next_ptp(l3_ptp, 3, va, &phys_page, &pte, false);
	//BUG_ON(ret < 0);
	if (ret < 0) {
		printk("[debug_query_in_pgtbl] L3 no mapping.\n");
		return ret;
	}
	printk("L3 pte is 0x%lx\n", pte->pte);

	*pa = virt_to_phys((vaddr_t)phys_page) + GET_VA_OFFSET_L3(va);
	*entry = pte;
	return 0;
}

void sys_debug_va(u64 va)
{
	u64 ttbr0_el1;
	vaddr_t *pgtbl;
	paddr_t pa;
	pte_t *entry;
	int ret;

#if CHCORE
	asm volatile ("mrs %0, ttbr0_el1":"=r" (ttbr0_el1));
#else
	ttbr0_el1 = 0;
#endif

	pgtbl = (vaddr_t *)phys_to_virt(ttbr0_el1);
	ret = debug_query_in_pgtbl(pgtbl, va, &pa, &entry);
	if (ret < 0)
		printk("[sys_debug_va] va is not mapped.\n");
	else
		printk("[sys_debug_va] va 0x%lx --> pa 0x%lx\n", va, pa);
}

/*
 * Translate a va to pa, and get its pte for the flags
 */
int query_in_pgtbl(vaddr_t *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
	/* On aarch64, l0 is the highest level page table */
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	ptp_t *phys_page;
	pte_t *pte;
	int ret;

	// L0 page table
	l0_ptp = (ptp_t *)pgtbl;
	ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
	if (ret < 0) // assert(ret == -ENOMAPPING);
		return ret;

	// L1 page table
	ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
	if (ret < 0)
		return ret;
	else if (ret == BLOCK_PTP) {
		*pa = virt_to_phys((vaddr_t)l2_ptp) +
			GET_VA_OFFSET_L1(va);
		if (entry)
			*entry = pte;
		return 0;
	}

	// L2 page table
	ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
	if (ret < 0)
		return ret;
	else if (ret == BLOCK_PTP) {
		*pa = virt_to_phys((vaddr_t)l3_ptp) +
			GET_VA_OFFSET_L2(va);
		if (entry)
			*entry = pte;
		return 0;
	}

	// L3 page table
	ret = get_next_ptp(l3_ptp, 3, va, &phys_page, &pte, false);
	if (ret < 0)
		return ret;

	*pa = virt_to_phys((vaddr_t)phys_page) + GET_VA_OFFSET_L3(va);
	if (entry)
		*entry = pte;
	return 0;
}

//TODO: we do not support huge page now
int map_range_in_pgtbl(struct vmspace *vmspace, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags)
{
	vaddr_t *pgtbl;
	s64 total_page_cnt;
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	pte_t *pte;
	int ret;
	int pte_index; // the index of pte in the last level page table
	int i;

	pgtbl = vmspace->pgtbl;
	BUG_ON(pgtbl == NULL); // alloc the root page table page at first
	// FIXME: if va is not aligned, totol_page_cnt may be not correct
	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);

	l0_ptp = (ptp_t *)pgtbl;

	l1_ptp = NULL;
	l2_ptp = NULL;
	l3_ptp = NULL;

	while (total_page_cnt > 0) {
		// l0
		ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, true);
		BUG_ON(ret != 0);

		// l1
		ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, true);
		BUG_ON(ret != 0);

		// l2
		ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, true);
		BUG_ON(ret != 0);

		// l3
		// step-1: get the index of pte
		pte_index = GET_L3_INDEX(va);
		for (i = pte_index; i < PTP_ENTRIES; ++i) {
			pte_t new_pte_val;

			new_pte_val.pte = 0;
			new_pte_val.l3_page.is_valid = 1;
			new_pte_val.l3_page.is_page  = 1;
			new_pte_val.l3_page.pfn = pa >> PAGE_SHIFT;
			set_pte_flags(&new_pte_val, flags, USER_PTE);
			l3_ptp->ent[i].pte = new_pte_val.pte;

			va += PAGE_SIZE;
			pa += PAGE_SIZE;
			total_page_cnt -= 1;
			if (total_page_cnt == 0)
				break;
		}
	}
	flush_tlb();
	return 0;
}

/*
 * TODO: We do not free the physical pages here.
 * We should free them when releasing the related VMO.
 */
int unmap_range_in_pgtbl(struct vmspace* vmspace, vaddr_t va, size_t len)
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
	l0_ptp = (ptp_t *)pgtbl;

	// FIXME: if va is not aligngned, totoal_page_cnt may be wrong!
	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);
	while (total_page_cnt > 0) {
		// l0
		ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L0_PER_ENTRY_PAGES;
			va += L0_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		// l1
		ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L1_PER_ENTRY_PAGES;
			va += L1_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		// l2
		ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L2_PER_ENTRY_PAGES;
			va += L2_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		// l3
		// step-1: get the index of pte
		pte_index = GET_L3_INDEX(va);
		for (i = pte_index; i < PTP_ENTRIES; ++i) {
			l3_ptp->ent[i].pte = PTE_DESCRIPTOR_INVALID;
			va += PAGE_SIZE;
			total_page_cnt -= 1;
			if (total_page_cnt == 0)
				break;
		}
	}

	flush_tlb();
	return 0;
}

int mprotect_in_pgtbl(struct vmspace* vmspace, vaddr_t va, size_t len,
			 vmr_prop_t flags)
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
	l0_ptp = (ptp_t *)pgtbl;

	// FIXME: if va is not aligngned, totoal_page_cnt may be wrong!
	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);
	while (total_page_cnt > 0) {
		// l0
		ret = get_next_ptp(l0_ptp, 0, va, &l1_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L0_PER_ENTRY_PAGES;
			va += L0_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		// l1
		ret = get_next_ptp(l1_ptp, 1, va, &l2_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L1_PER_ENTRY_PAGES;
			va += L1_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		// l2
		ret = get_next_ptp(l2_ptp, 2, va, &l3_ptp, &pte, false);
		if (ret == -ENOMAPPING) {
			total_page_cnt -= L2_PER_ENTRY_PAGES;
			va += L2_PER_ENTRY_PAGES * PAGE_SIZE;
			continue;
		}

		// l3
		// step-1: get the index of pte
		pte_index = GET_L3_INDEX(va);
		for (i = pte_index; i < PTP_ENTRIES; ++i) {

			/* Modify the permission in the pte if it exists */
			if (!IS_PTE_INVALID(l3_ptp->ent[i].pte))
				set_pte_flags(&(l3_ptp->ent[i]), flags, USER_PTE);

			va += PAGE_SIZE;
			total_page_cnt -= 1;
			if (total_page_cnt == 0)
				break;
		}
	}

	flush_tlb();
	return 0;
}
