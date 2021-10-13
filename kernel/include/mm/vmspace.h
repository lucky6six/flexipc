#pragma once

#include <common/list.h>
#include <common/lock.h>
#include <common/radix.h>
#include <arch/mmu.h>

#ifdef CHCORE
#include <machine.h>
#endif

struct vmregion {
	struct list_head node; /* vmr_list */
	vaddr_t start;
	size_t size;
	vmr_prop_t perm;
	struct pmobject *pmo;
};

struct vmspace {
	/* List head of vmregion (vmr_list) */
	struct list_head vmr_list;
	/* Root page table */
	vaddr_t *pgtbl;

	/* The lock for manipulating vmregions */
	struct lock vmspace_lock;
	/* The lock for manipulating the page table */
	struct lock pgtbl_lock;

	/*
	 * For TLB flushing:
	 * Record the all the CPU that a vmspace ran on.
	 */
	#ifdef CHCORE
	u8 history_cpus[PLAT_CPU_NUM];
	#endif

	/* Heap-related: only used for user processes */
	struct lock heap_lock;
	struct vmregion *heap_vmr;
	vaddr_t user_current_heap;

	/* For the virtual address of mmap */
	vaddr_t user_current_mmap_addr;
};

typedef u64 pmo_type_t;
#define PMO_ANONYM     0 /* lazy allocation */
#define PMO_DATA       1 /* immediate allocation */
#define PMO_FILE       2 /* file backed */
#define PMO_SHM        3 /* shared memory */
#define PMO_USER_PAGER 4 /* support user pager */
#define PMO_DEVICE     5 /* memory mapped device registers */

#define PMO_FORBID     10 /* Forbidden area: avoid overflow */

struct pmobject {
	struct radix *radix; /* record physical pages */
	paddr_t start;
	size_t size;
	pmo_type_t type;
	atomic_cnt refcnt; // TODO: free this pmo when refcnt is 0
};

int vmspace_init(struct vmspace *vmspace);
void pmo_init(struct pmobject *pmo, pmo_type_t type, size_t len, paddr_t paddr);

int vmspace_map_range(struct vmspace *vmspace, vaddr_t va, size_t len,
		      vmr_prop_t flags, struct pmobject *pmo);
int vmspace_unmap_range(struct vmspace *vmspace, vaddr_t va, size_t len);


struct vmregion *find_vmr_for_va(struct vmspace *vmspace, vaddr_t addr);

void switch_vmspace_to(struct vmspace *);

void commit_page_to_pmo(struct pmobject *pmo, u64 index, paddr_t pa);
paddr_t get_page_from_pmo(struct pmobject *pmo, u64 index);

struct vmregion *init_heap_vmr(struct vmspace *vmspace, vaddr_t va,
			       struct pmobject *pmo);

void kprint_vmr(struct vmspace *vmspace);
