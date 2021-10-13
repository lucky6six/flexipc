#include <common/macro.h>
#include <common/util.h>
#include <common/list.h>
#include <common/errno.h>
#include <common/lock.h>
#include <common/kprint.h>
#include <mm/vmspace.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <arch/mmu.h>

/* Local functions */

static struct vmregion *alloc_vmregion(void)
{
	struct vmregion *vmr;

	vmr = kmalloc(sizeof(*vmr));
	return vmr;
}

static void free_vmregion(struct vmregion *vmr)
{
	kfree((void*)vmr);
}

/*
 * This function should be surrounded with a lock.
 * Returns 0 when no intersection detected.
 *
 * Search the list with lock protection.
 * Otherwise, concurrent operations may lead to erros.
 */
static int check_vmr_intersect(struct vmspace *vmspace,
				struct vmregion  *vmr_to_add)
{
	struct vmregion *vmr;
	vaddr_t new_start, start;
	vaddr_t new_end, end;

	if (unlikely(vmr_to_add->size == 0))
		return 0;

	new_start = vmr_to_add->start;
	new_end = new_start + vmr_to_add->size - 1;

	for_each_in_list(vmr, struct vmregion, node, &(vmspace->vmr_list)) {
		start = vmr->start;
		end   = start + vmr->size;
		if (!((new_start >= end) || (new_end <= start))) {
			kwarn("new_start: %p, new_ned: %p, start: %p, end: %p\n",
			      new_start, new_end, start, end);
			return 1;
		}
	}

	return 0;
}

/*
 * This function should be surrounded with a lock.
 *
 * Search the list with lock protection.
 * Otherwise, concurrent operations may lead to erros.
 */
static int is_vmr_in_vmspace(struct vmspace *vmspace, struct vmregion *vmr)
{
	struct vmregion *iter;

	/*
	 * Search the list with lock protection.
	 * Otherwise, concurrent operations may lead to erros.
	 */
	for_each_in_list(iter, struct vmregion, node, &(vmspace->vmr_list)) {
		if (iter == vmr)
			return 1;
	}
	return 0;
}

/*
 * This function should be surrounded with a lock.
 *
 * Modify the list with lock protection.
 * Otherwise, concurrent operations may lead to erros.
 */
static int add_vmr_to_vmspace(struct vmspace *vmspace, struct vmregion *vmr)
{
	if (check_vmr_intersect(vmspace, vmr) != 0) {
		kwarn("Detecting: vmr overlap\n");
		BUG_ON(1);
		return -EINVAL;
	}

	list_add(&(vmr->node), &(vmspace->vmr_list));
	return 0;
}

/*
 * This function should be surrounded with a lock.
 *
 * Modify the list with lock protection.
 * Otherwise, concurrent operations may lead to erros.
 */
static void del_vmr_from_vmspace(struct vmspace *vmspace, struct vmregion *vmr)
{
	if(is_vmr_in_vmspace(vmspace, vmr))
		list_del(&(vmr->node));
	free_vmregion(vmr);
}

static int fill_page_table(struct vmspace *vmspace, struct vmregion *vmr)
{
	size_t pm_size;
	paddr_t pa;
	vaddr_t va;
	int ret;

	pm_size = vmr->pmo->size;
	pa = vmr->pmo->start;
	va = vmr->start;

	lock(&vmspace->pgtbl_lock);
	ret = map_range_in_pgtbl(vmspace, va, pa, pm_size, vmr->perm);
	unlock(&vmspace->pgtbl_lock);

	return ret;
}

static void free_pmo(struct pmobject *pmo, vaddr_t va, size_t len)
{
	// TODO: decrease the pmo refcnt and free the related pages if the
	// refcnt becomes zero.

	/* Different way to free different pmo types */
}

static int unmap_vmrs(struct vmspace *vmspace, vaddr_t va, size_t len)
{
	struct vmregion *vmr;
	struct pmobject *pmo;

	if (len == 0)
		return 0;

	vmr = find_vmr_for_va(vmspace, va);
	if (vmr == NULL) {
		kwarn("%s: no vmr found for the va 0x%lx.\n", __func__, va);
		kwarn("TODO: we need to unmap any address in [va, va+len)\n");
		return 0;
	}

	pmo = vmr->pmo;

	if (vmr->size > len) {
		kwarn("%s: shrink one vmr (va is 0x%lx).\n", __func__, va);

		/*
		 * TODO: only support **shrink** a vmr whose pmo is PMO_ANONYM.
		 * shrink: unmap some part of the vmr.
		 */
		BUG_ON(pmo->type != PMO_ANONYM);

		/* Modify the vmregion start and size */
		vmr->size -= len;
		vmr->start += len;

		lock(&vmspace->pgtbl_lock);
		unmap_range_in_pgtbl(vmspace, va, len);
		unlock(&vmspace->pgtbl_lock);

		free_pmo(pmo, va, len);
		return 0;
	}

	/* Umap a whole vmr */
	lock(&vmspace->pgtbl_lock);
	unmap_range_in_pgtbl(vmspace, va, len);
	unlock(&vmspace->pgtbl_lock);
	free_pmo(pmo, va, len);
	/* delete the vmr from the vmspace */
	del_vmr_from_vmspace(vmspace, vmr);


	va += vmr->size;
	len -= vmr->size;
	return unmap_vmrs(vmspace, va, len);
}



/* End of local functions */


/*
 * Tracing/debugging:
 * This function is for dumping the vmr_list after a thread crashes.
 */
void kprint_vmr(struct vmspace *vmspace)
{
	struct vmregion *vmr;
	vaddr_t start, end;

	for_each_in_list(vmr, struct vmregion,
			 node, &(vmspace->vmr_list)) {
		start = vmr->start;
		end = start + vmr->size;
		kinfo("[vmregion] start=%p end=%p. vmr->pmo->type=%d\n",
		      start, end, vmr->pmo->type);
	}
}

/*
 * This function should be surrounded with a lock.
 *
 * Search the list with lock protection.
 * Otherwise, concurrent operations may lead to erros.
 */
struct vmregion *find_vmr_for_va(struct vmspace *vmspace, vaddr_t addr)
{
	struct vmregion *vmr;
	vaddr_t start, end;

	for_each_in_list(vmr, struct vmregion, node, &(vmspace->vmr_list)) {
		start = vmr->start;
		end   = start + vmr->size;
		if(addr >= start && addr < end)
			return vmr;
	}
	return NULL;
}



int vmspace_map_range(struct vmspace *vmspace, vaddr_t va, size_t len,
		      vmr_prop_t flags, struct pmobject *pmo)
{
	struct vmregion *vmr;
	int ret;

	/* Check whether the pmo type is supported */
	BUG_ON((pmo->type != PMO_DATA) && (pmo->type != PMO_ANONYM) &&
	       (pmo->type != PMO_DEVICE) && (pmo->type != PMO_SHM));

	/* Align a vmr to PAGE_SIZE */
	va = ROUND_DOWN(va, PAGE_SIZE);
	if (len < PAGE_SIZE)
		len = PAGE_SIZE;

	vmr = alloc_vmregion();
	if (!vmr) {
		ret = -ENOMEM;
		goto out_fail;
	}
	vmr->start = va;
	vmr->size = len;
	vmr->perm = flags;
	if (unlikely(pmo->type == PMO_DEVICE))
		vmr->perm |= VMR_DEVICE;

	/* Currently, one vmr has exactly one pmo */
	vmr->pmo = pmo;

	/*
	 * Note that each operation on the vmspace should be protected by
	 * the per_vmspace lock, i.e., vmspace_lock
	 */
	lock(&vmspace->vmspace_lock);
	ret = add_vmr_to_vmspace(vmspace, vmr);
	unlock(&vmspace->vmspace_lock);

	if (ret < 0) {
		kwarn("add_vmr_to_vmspace fails\n");
		goto out_free_vmr;
	}

	/*
	 * Case-1:
	 * If the pmo type is PMO_DATA or PMO_DEVICE, we directly add mappings
	 * in the page table because the corresponding physical pages are
	 * prepared. In this case, early mapping avoids page faults and brings
	 * better performance.
	 *
	 * Case-2:
	 * Otherwise (for PMO_ANONYM and PMO_SHM), we use on-demand mapping.
	 * In this case, lazy mapping reduces the usage of physical memory resource.
	 */
	if ((pmo->type == PMO_DATA) || (pmo->type == PMO_DEVICE))
		fill_page_table(vmspace, vmr);

	/* On success */
	return 0;
out_free_vmr:
	free_vmregion(vmr);
out_fail:
	return ret;
}

int vmspace_unmap_range(struct vmspace *vmspace, vaddr_t va, size_t len)
{
	struct vmregion *vmr;
	vaddr_t start;
	size_t size;
	struct pmobject *pmo;
	int ret;

	/*
	 * Protect `find_vmr_for_va` and `del_vmr_from_vmspace`
	 * with the vmspace_lock
	 */
	lock(&vmspace->vmspace_lock);
	vmr = find_vmr_for_va(vmspace, va);
	if (!vmr) {
		kwarn("unmap a non-exist vmr.\n");
		ret = -1;
		goto out;
	}
	start = vmr->start;
	size  = vmr->size;

	/* Sanity check: unmap the whole vmr */
	if (((va != start) || (len != size)) && (len != 0)) {
		kdebug("va: %p, start: %p, len: %p, size: %p\n",
		      va, start, len, size);
		kwarn("Only support unmapping a whole vmregion now.\n");
		BUG_ON(1);
	}

	del_vmr_from_vmspace(vmspace, vmr);
	unlock(&vmspace->vmspace_lock);


	pmo = vmr->pmo;
	/* No pmo is mapped */
	if (pmo == NULL) {
		ret = 0;
		goto out;
	}

	/*
	 * Remove the mappings in the page table.
	 * When the pmo-type is DATA/DEVICE, each mapping must exist.
	 *
	 * Otherwise, the mapping is added on demand, which may not exist.
	 * However, simply clearing non-present ptes is OK.
	 */

	if (likely(len != 0)) {
		lock(&vmspace->pgtbl_lock);
		unmap_range_in_pgtbl(vmspace, va, len);
		unlock(&vmspace->pgtbl_lock);
	}

	/*
	 * TODO: free physical pages in the PMO.
	 * But, we need the refcnt mechansim to track the pmo usage.
	 */

	ret = 0;
out:
	return ret;
}



/* In the beginning, a vmspace ran on zero CPU */
static inline void reset_history_cpus(struct vmspace *vmspace)
{
	int i;

	for (i = 0; i < PLAT_CPU_NUM; ++i)
		vmspace->history_cpus[i] = 0;
}

void record_history_cpu(struct vmspace *vmspace, u32 cpuid)
{
	BUG_ON(cpuid >= PLAT_CPU_NUM);
	/*
	 * Note that lock/atomic_ops are not required here
	 * because only CPU X will modify (record/clear)
	 * history_cpus[X].
	 */
	vmspace->history_cpus[cpuid] = 1;
}

void clear_history_cpu(struct vmspace *vmspace, u32 cpuid)
{
	BUG_ON(cpuid >= PLAT_CPU_NUM);
	/*
	 * Note that lock/atomic_ops are not required here
	 * because only CPU X will modify (record/clear)
	 * history_cpus[X].
	 */
	vmspace->history_cpus[cpuid] = 0;
}


/*
 * The heap region of each process starts at HEAP_START and can at most grow
 * to (MMAP_START-1). (up to 16 TB)
 *
 * TODO: add guard vmr in between HEAP_START and MMAP_START.
 * TODO: add guard vmr for each thread's stack (which should be iniated by the
 * user library).
 *
 * The mmap region of each process starts at MMAP_START and can at most grow
 * to USER_SPACE_END. (up to 16 TB)
 *
 * For x86_64:
 * In 64-bit mode, an address is considered to be in canonical form
 * if address bits 63 through to the most-significant implemented bit
 * by the microarchitecture are set to either all ones or all zeros.
 * The kernel and user share the 48-bit address (0~2^48-1).
 * As usual, we let the kernel use the top half and the user use the
 * bottom half.
 * So, the user address is 0 ~ 2^47-1 (USER_SPACE_END).
 *
 *
 * For aarch64:
 * With 4-level page tables:
 * TTBR0_EL1 translates 0 ~ 2^48-1 .
 * TTBR1_EL1 translates (0xFFFF) + 0 ~ 2^48-1.
 * But, we only use 0 ~ USER_SPACE_END (as x86_64).
 * With 3-level page tables:
 * The same as x86_64.
 */

#define HEAP_START	(0x600000000000UL)
#define MMAP_START	(0x700000000000UL)
#define USER_SPACE_END  (0x800000000000UL)

/* Each process has a vmr specially for heap */
struct vmregion *init_heap_vmr(struct vmspace *vmspace, vaddr_t va,
			       struct pmobject *pmo)
{
	struct vmregion *vmr;
	int ret;

	vmr = alloc_vmregion();
	if (!vmr) {
		kwarn("%s fails\n", __func__);
		goto out_fail;
	}
	vmr->start = va;
	vmr->size = 0;
	vmr->perm = VMR_READ | VMR_WRITE;
	vmr->pmo = pmo;

	lock(&vmspace->vmspace_lock);
	ret = add_vmr_to_vmspace(vmspace, vmr);
	unlock(&vmspace->vmspace_lock);

	if (ret < 0)
		goto out_free_vmr;

	return vmr;

out_free_vmr:
	free_vmregion(vmr);
out_fail:
	return NULL;
}

u64 vmspace_mmap_with_pmo(struct vmspace *vmspace, struct pmobject *pmo,
			  size_t len, vmr_prop_t perm)
{
	struct vmregion *vmr;
	int ret;

	vmr = alloc_vmregion();
	if (!vmr) {
		kwarn("%s fails\n", __func__);
		goto out_fail;
	}

	/* Protect vmspace->user_current_mmap_addr with vmspace_lock */
	lock(&vmspace->vmspace_lock);

	vmr->start = vmspace->user_current_mmap_addr;

	BUG_ON(len % PAGE_SIZE);
	/* TODO: for simplicity, just keep increasing the mmap_addr now */
	vmspace->user_current_mmap_addr += len;

	vmr->size = len;
	vmr->perm = perm;

	/*
	 * Currently, we restrict the pmo types, which must be
	 * pmo_anonym or pmo_shm or pmo_file.
	 */
	BUG_ON((pmo->type != PMO_ANONYM)
	       && (pmo->type != PMO_SHM)
	       && (pmo->type != PMO_FILE));

	vmr->pmo = pmo;

	ret = add_vmr_to_vmspace(vmspace, vmr);
	unlock(&vmspace->vmspace_lock);

	if (ret < 0)
		goto out_free_vmr;

	return vmr->start;

out_free_vmr:
	free_vmregion(vmr);
out_fail:
	return (u64)-1L;
}

int vmspace_munmap_with_addr(struct vmspace *vmspace, vaddr_t va, size_t len)
{
	// TODO: consider different cases (non-expected arguments) for munmap
	// TODO: recycle the virtual memory range

	int ret;

	lock(&vmspace->vmspace_lock);
	ret = unmap_vmrs(vmspace, va, len);
	unlock(&vmspace->vmspace_lock);

	return ret;
}

int vmspace_unmap_shm_vmr(struct vmspace *vmspace, vaddr_t va)
{
	struct vmregion *vmr;
	struct pmobject *pmo;

	lock(&vmspace->vmspace_lock);

	vmr = find_vmr_for_va(vmspace, va);
	if (vmr == NULL) {
		kwarn("%s: no vmr found for the va 0x%lx.\n", __func__, va);
		goto fail_out;
	}

	pmo = vmr->pmo;

	/* Sanity check */
	/* check-1: this interface is only used for shmdt */
	BUG_ON(pmo->type != PMO_SHM);
	/* check-2: the va should be the start address of the shm */
	BUG_ON(va != vmr->start);

	/* Umap a whole vmr */
	lock(&vmspace->pgtbl_lock);
	unmap_range_in_pgtbl(vmspace, vmr->start, vmr->size);
	unlock(&vmspace->pgtbl_lock);

	/*
	 * Physical resources free should be done when the shm object is
	 * removed by shmctl.
	 */
	// free_pmo(pmo, va, vmr->size);

	/* Delete the vmr from the vmspace */
	del_vmr_from_vmspace(vmspace, vmr);

	unlock(&vmspace->vmspace_lock);
	return 0;

fail_out:
	unlock(&vmspace->vmspace_lock);
	return -EINVAL;
}


extern void arch_vmspace_init(struct vmspace *);

int vmspace_init(struct vmspace *vmspace)
{
	init_list_head(&vmspace->vmr_list);
	/* Allocate the root page table page */
	vmspace->pgtbl = get_pages(0);
	BUG_ON(vmspace->pgtbl == NULL);
	memset((void*)vmspace->pgtbl, 0, PAGE_SIZE);

	/* Architecture-dependent initilization */
	arch_vmspace_init(vmspace);

	/*
	 * Note: acquire vmspace_lock before pgtbl_lock
	 * when locking them together.
	 */
	lock_init(&vmspace->vmspace_lock);
	lock_init(&vmspace->pgtbl_lock);

	vmspace->user_current_heap = HEAP_START;
	lock_init(&vmspace->heap_lock);

	/* The vmspace does not run on any CPU for now */
	reset_history_cpus(vmspace);

	/* Set the mmap area: this variable is protected by the vmspace_lock */
	vmspace->user_current_mmap_addr = MMAP_START;

	return 0;
}

/*
 * TODO: release the resource when a process exits.
 * No need to acquire the lock.
 */
int destroy_vmspace(struct vmspace *vmspace)
{
	struct vmregion *vmr;
	vaddr_t start;
	size_t size;

	/* Unmap each vmregion in vmspace->vmr_list */
	for_each_in_list(vmr, struct vmregion, node, &(vmspace->vmr_list)) {
		start = vmr->start;
		size  = vmr->size;
		del_vmr_from_vmspace(vmspace, vmr);
		unmap_range_in_pgtbl(vmspace, start, size);
	}

	// TODO: free the page table pages
	kfree(vmspace);
	return 0;
}

/*
 * TODO: pmo_deinit:
 *	 - step-1: free physical pages according to the radix
 *	 - step-2: radix_free
 */

/*
 * Initialize an allocated pmobject.
 * @paddr is only used when @type == PMO_DEVICE.
 */
void pmo_init(struct pmobject *pmo, pmo_type_t type, size_t len, paddr_t paddr)
{
	memset((void*)pmo, 0, sizeof(*pmo));

	len = ROUND_UP(len, PAGE_SIZE);
	pmo->size = len;
	pmo->type = type;

	switch (type) {
	case PMO_DATA: {
		/*
		 * For PMO_DATA, the user will use it soon (we expect).
		 * So, we directly allocate the physical memory.
		 * Note that kmalloc(>2048) returns continous physical pages.
		 */
		pmo->start = (paddr_t)virt_to_phys(kmalloc(len));
		break;
	}
	case PMO_FILE: /* PMO backed by a file. It also uses the radix. */
	case PMO_ANONYM:
	case PMO_SHM: {
		/*
		 * For PMO_ANONYM (e.g., stack and heap) or PMO_SHM,
		 * we do not allocate the physical memory at once.
		 */
		pmo->radix = new_radix();
		init_radix(pmo->radix);
		break;
	}
	case PMO_DEVICE: {
		/*
		 * For device memory (e.g., for DMA).
		 * We must ensure the range [paddr, paddr+len) is not
		 * in the main memory region.
		 */
		pmo->start = paddr;
		break;
	}
	case PMO_FORBID: {
		/* This type marks the corresponding area cannot be accessed */
		break;
	}
	default: {
		kinfo("Unsupported pmo type: %d\n", type);
		BUG_ON(1);
		break;
	}
	}
}

/* Record the physical page allocated to a pmo */
void commit_page_to_pmo(struct pmobject *pmo, u64 index, paddr_t pa)
{
	int ret;

	BUG_ON((pmo->type != PMO_ANONYM) && (pmo->type != PMO_SHM) &&
	       (pmo->type != PMO_FILE));
	/* The radix interfaces are thread-safe */
	ret = radix_add(pmo->radix, index, (void *)pa);
	BUG_ON(ret != 0);
}

/* Return 0 (NULL) when not found */
paddr_t get_page_from_pmo(struct pmobject *pmo, u64 index)
{
	paddr_t pa;

	/* The radix interfaces are thread-safe */
	pa = (paddr_t)radix_get(pmo->radix, index);
	return pa;
}

/* Change vmspace to the target one */
void switch_vmspace_to(struct vmspace *vmspace)
{
	set_page_table(virt_to_phys(vmspace->pgtbl));
}
