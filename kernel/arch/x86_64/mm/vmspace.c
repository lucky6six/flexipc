#include <common/types.h>
#include <common/kprint.h>
#include <common/lock.h>
#include <mm/mm.h>
#include <mm/vmspace.h>
#include <mm/kmalloc.h>

#include "page_table.h"

/* the virtual address of kernel page table */
extern char CHCORE_PGD[];

/* global varibles */

struct lock pcid_lock;
/* current available PCID */
int current_pcid;

void arch_mm_init(void)
{
	lock_init(&pcid_lock);
	/* kernel uses PCID 0 */
	current_pcid = 1;
}

#define MAX_PCID 4095
// TODO: recycle the PCID
static int alloc_pcid(void)
{
	int pcid;

	lock(&pcid_lock);
	pcid = current_pcid;
	BUG_ON(pcid > MAX_PCID);
	current_pcid += 1;
	unlock(&pcid_lock);

	return pcid;
}

/*
static void pte_set_user_bit(pte_t *entry)
{
	entry->pteval |= PAGE_USER;
}
*/

static void pte_clear_user_bit(pte_t *entry)
{
	entry->pteval &= ~PAGE_USER;
}

/* Initialize the top page table page for a user-level process */
void arch_vmspace_init(struct vmspace *vmspace)
{
	ptp_t *ptp_k;
	ptp_t *ptp_u;
	pte_t *entry_k;
	pte_t *entry_u;
	int index;
	int pcid;

	/*
	 * We map the kernel space in the user pgtbl
	 * but mark them as Supervisor (user cannot access).
	 *
	 * We use 1G huge page for the kernel address mapping.
	 */

	/* Kernel root page table page */
	ptp_k = (ptp_t *)CHCORE_PGD;
	/* User process root page table page */
	ptp_u = (ptp_t *)vmspace->pgtbl;

	/* Map the kernel code part in the user page table */
	index = GET_L0_INDEX(KCODE);
	BUG_ON(index != 511);
	/* The page table entry for the kernel code mapping */
	entry_k = &(ptp_k->ent[index]);
	/* The corresponding entry in the user page table */
	entry_u = &(ptp_u->ent[index]);
	/* Set the mapping in the user page table */
	entry_u->pteval = entry_k->pteval;
	/* Protect kernel form user */
	pte_clear_user_bit(entry_u);

	/* Map the kernel direct mapping part in the user page table */
	index = GET_L0_INDEX(KBASE);
	BUG_ON(index != 510);
	/* The page table entry for the kernel code mapping */
	entry_k = &(ptp_k->ent[index]);
	/* The corresponding entry in the user page table */
	entry_u = &(ptp_u->ent[index]);
	/* Setup the mappings in the user page table */
	entry_u->pteval = entry_k->pteval;
	/* Protect kernel form user */
	pte_clear_user_bit(entry_u);

	/* CR3: the lower 12-bit represent PCID */
	pcid = alloc_pcid();
	vmspace->pgtbl = (vaddr_t *)(((u64)(vmspace->pgtbl)) | pcid);
}
