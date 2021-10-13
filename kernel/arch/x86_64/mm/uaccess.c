#include <common/util.h>
#include <common/vars.h>
#include <common/types.h>
#include <common/kprint.h>
#include <common/macro.h>
#include <mm/kmalloc.h>
#include <mm/vmspace.h>
#include <mm/mm.h>
#include <object/thread.h>
#include <object/object.h>
#include <object/cap_group.h>

/* Another way to access user memory is disabling SMAP and
 * directly access them.
 */

#define PAGE_OFFSET_MASK 0xFFF
static vaddr_t transform_vaddr(char *user_buf)
{
	struct vmspace *vmspace;
	struct vmregion *vmr;
	struct pmobject *pmo;
	vaddr_t kva;

	u64 offset, index;
	vaddr_t va; /* Aligned va of user_buf */
	paddr_t pa;

	vmspace = obj_get(current_cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);

	/* Prevent concurrent operations on the vmspace */
	lock(&vmspace->vmspace_lock);

	vmr = find_vmr_for_va(vmspace, (vaddr_t)user_buf);
	/* Target user address is not valid (not mapped before) */
	BUG_ON(vmr == NULL);
	pmo = vmr->pmo;
	BUG_ON(pmo == NULL);

	/*
	 * A special case: the address is mapped with an anonymous pmo
	 * which is not directly mapped by the user.
	 * And, kernel touches it first.
	 *
	 * To prevent pgfault, kernel maps the page first if necessary.
	 *
	 * pmo->type is data: must be mapped
	 */
	switch (pmo->type) {
	case PMO_DATA: {
		/*
		 * Calculate the kva for the user_buf:
		 * kva = start_addr + offset
		 */
		kva = phys_to_virt(vmr->pmo->start) +
			((vaddr_t)user_buf - vmr->start);
		break;
	}
	case PMO_ANONYM: {
		va = ROUND_DOWN((u64)user_buf, PAGE_SIZE);
		offset = va - vmr->start;
		/* Boundary check */
		BUG_ON(offset >= pmo->size);
		index = offset / PAGE_SIZE;
		/* Get the physical page for va according to the radix tree in the pmo */
		pa = get_page_from_pmo(pmo, index);

		if (pa == 0) {
			/* No physical page allocated to it before */
			pa = virt_to_phys(get_pages(0));
			BUG_ON(pa == 0);
			memset((void *)phys_to_virt(pa), 0, PAGE_SIZE);

			commit_page_to_pmo(pmo, index, pa);

			lock(&vmspace->pgtbl_lock);
			map_range_in_pgtbl(vmspace, va, pa, PAGE_SIZE,
					   vmr->perm);
			unlock(&vmspace->pgtbl_lock);
		}
		/*
		 * Return: start address of the newly allocated page +
		 *         offset in the page (last 12 bits).
		 */
		kva = phys_to_virt(pa) +
			(((vaddr_t)user_buf) & PAGE_OFFSET_MASK);
		break;
	}
	case PMO_SHM:
	case PMO_FILE: {
		va = ROUND_DOWN((u64)user_buf, PAGE_SIZE);
		offset = va - vmr->start;

		/*
		 * Boundary check.
		 *
		 * No boundary check for PMO_FILE because a file can
		 * be mapped to a larger (larger than the file size) vmr.
		 * (allowed in Linux)
		 *
		 * TODO: make the behaviour more accurate in ChCore
		 */
		if (pmo->type == PMO_SHM)
			BUG_ON(offset >= pmo->size);

		index = offset / PAGE_SIZE;
		/* Get the physical page for va according to the radix tree in the pmo */
		pa = get_page_from_pmo(pmo, index);

		if (pa == 0) {
			/*
			 * No physical page allocated to it before.
			 * SHM should not be accessed by kernel first.
			 */
			kwarn("kernel accessing PMO_SHM before user\n");
			BUG_ON(1);
		}
		/*
		 * Return: start address of the newly allocated page +
		 *         offset in the page (last 12 bits).
		 */
		kva = phys_to_virt(pa) +
			(((vaddr_t)user_buf) & PAGE_OFFSET_MASK);
		break;
	}
	default: {
		kinfo("bug: kernel accessing pmo type: %d\n", pmo->type);
		BUG_ON(1);
		break;
	}
	}

	unlock(&vmspace->vmspace_lock);

	obj_put(vmspace);
	return kva;
}

#ifndef FBINFER

int copy_from_user(char *kernel_buf, char *user_buf, size_t size)
{
	vaddr_t kva;
	u64 start_off;
	u64 len;
	u64 left_size;

	len = 0;

	/* Validate user_buf */
	BUG_ON(((u64)user_buf >= KBASE) ||
	       ((u64)user_buf + size >= KBASE));

	/* For the frist user memory page */
	kva = transform_vaddr(user_buf);
	start_off = ((vaddr_t)user_buf) & PAGE_OFFSET_MASK;
	/* No more than one page */
	if (size + start_off <= PAGE_SIZE) {
		memcpy((void *)kernel_buf, (void *)kva, size);
		return 0;
	} else {
		len = PAGE_SIZE - start_off;
		memcpy((void *)kernel_buf, (void *)kva, len);
		user_buf += len;
		kernel_buf += len;
	}

	/* Intermediate memory pages */
	BUG_ON(((u64)user_buf % PAGE_SIZE) != 0);
	left_size = size - len;
	while (left_size > PAGE_SIZE) {
		kva = transform_vaddr(user_buf);
		memcpy((void *)kernel_buf, (void *)kva, PAGE_SIZE);
		user_buf += PAGE_SIZE;
		kernel_buf += PAGE_SIZE;
		left_size -= PAGE_SIZE;
	}

	/* The last memory page */
	kva = transform_vaddr(user_buf);
	memcpy((void *)kernel_buf, (void *)kva, left_size);

	return 0;
}

int copy_to_user(char *user_buf, char *kernel_buf, size_t size)
{
	vaddr_t kva;
	u64 start_off;
	u64 len;
	u64 left_size;

	len = 0;

	/* Validate user_buf */
	BUG_ON(((u64)user_buf >= KBASE) ||
	       ((u64)user_buf + size >= KBASE));

	/* For the frist user memory page */
	kva = transform_vaddr(user_buf);
	start_off = ((vaddr_t)user_buf) & PAGE_OFFSET_MASK;
	/* No more than one page */
	if (size + start_off <= PAGE_SIZE) {
		memcpy((void *)kva, (void *)kernel_buf, size);
		return 0;
	} else {
		len = PAGE_SIZE - start_off;
		memcpy((void *)kva, (void *)kernel_buf, len);
		user_buf += len;
		kernel_buf += len;
	}

	/* Intermediate memory pages */
	BUG_ON(((u64)user_buf % PAGE_SIZE) != 0);
	left_size = size - len;
	while (left_size > PAGE_SIZE) {
		kva = transform_vaddr(user_buf);
		memcpy((void *)kva, (void *)kernel_buf, PAGE_SIZE);
		user_buf += PAGE_SIZE;
		kernel_buf += PAGE_SIZE;
		left_size -= PAGE_SIZE;
	}

	/* The last memory page */
	kva = transform_vaddr(user_buf);
	memcpy((void *)kva, (void *)kernel_buf, left_size);

	return 0;
}

#endif
