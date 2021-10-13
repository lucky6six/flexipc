#include <common/macro.h>
#include <common/util.h>
#include <common/list.h>
#include <common/errno.h>
#include <common/kprint.h>
#include <common/types.h>
#include <common/lock.h>
#include <lib/printk.h>
#include <mm/vmspace.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <mm/vmspace.h>
#include <arch/mmu.h>
#include <object/thread.h>
#include <object/cap_group.h>
#include <sched/context.h>

/* Policy on-demand: only mapping the faulting address */
#define ONDEMAND  0

/*
 * Not implemented now.
 * Policy pre-fault: mapping the serveral continous pages in advance.
 */
#define PREFAULT  1

#define PGFAULT_POLICY ONDEMAND

#if PGFAULT_POLICY == ONDEMAND

int handle_trans_fault(struct vmspace *vmspace, vaddr_t fault_addr)
{
	struct vmregion *vmr;
	struct pmobject *pmo;
	paddr_t pa;
	u64 offset;
	u64 index;
	int ret = 0;

	/*
	 * Grab lock here.
	 * Because two threads (in same process) on different cores
	 * may fault on the same page, so we need to prevent them
	 * from adding the same mapping twice.
	 */

	lock(&vmspace->vmspace_lock);
	vmr = find_vmr_for_va(vmspace, fault_addr);
	if (vmr == NULL) {
		kinfo("handle_trans_fault: no vmr found for va 0x%lx!\n",
		      fault_addr);
		kinfo("process: %p\n", current_cap_group);
		print_thread(current_thread);
		kinfo("faulting IP: 0x%lx, SP: 0x%lx\n",
		      arch_get_thread_next_ip(current_thread),
		      arch_get_thread_stack(current_thread));

		kprint_vmr(vmspace);
		// TODO: kill the process
		kwarn("TODO: kill such faulting process.\n");
		return -ENOMAPPING;
	}

	pmo = vmr->pmo;
	switch (pmo->type) {
	case PMO_FILE:
	case PMO_ANONYM:
	case PMO_SHM: {

		vmr_prop_t perm;

		perm = vmr->perm;

		/* Get the offset in the pmo for faulting addr */
		offset = ROUND_DOWN(fault_addr, PAGE_SIZE) - vmr->start;
		/* Boundary check */
		if ((offset >= pmo->size) && (pmo->type == PMO_FILE)) {
			static int warn = 1;

			if (warn == 1) {
				kwarn("%s (out-of-range writing) offset 0x%lx, "
				      "pmo->size 0x%lx, FILE\n",
				      __func__, offset, pmo->size);
				warn = 0;
			}
			// TODO: send a SIGBUS signal
			// TODO: we simply allow it now by adding new pages
			perm = VMR_READ | VMR_WRITE | VMR_EXEC;

		} else {
			BUG_ON(offset >= pmo->size);
		}

		/* Get the index in the pmo radix for faulting addr */
		index = offset / PAGE_SIZE;

		pa = get_page_from_pmo(pmo, index);
		if (pa == 0) {
			/**
			 * For PMO_FILE, users can mmap a memory that is larger
			 * than the file size. If they accesses bytes beyond
			 * file size, SIGBUS should be triggered.
			 */

			//if (pmo->type == PMO_FILE) {
			//	ret = -ESIGBUS;
			//	break;
			//}

			/*
			 * Not committed before.
			 * Then, allocate the physical page
			 */
			pa = virt_to_phys(get_pages(0));
			BUG_ON(pa == 0);
			/* Clear to 0 for the newly allocated page */
			memset((void *)phys_to_virt(pa), 0, PAGE_SIZE);
			/*
			 * Record the physical page in the radix tree:
			 * the offset is used as index in the radix tree
			 */
			kdebug("commit: index: %ld, 0x%lx\n", index, pa);
			commit_page_to_pmo(pmo, index, pa);

			/* Add mapping in the page table */
			lock(&vmspace->pgtbl_lock);
			fault_addr = ROUND_DOWN(fault_addr, PAGE_SIZE);
			map_range_in_pgtbl(vmspace, fault_addr, pa,
					   PAGE_SIZE, perm);
			unlock(&vmspace->pgtbl_lock);
		}
		else {
			/*
			 * pa != 0: the faulting address has be committed a
			 * physical page.
			 *
			 * For concurrent page faults:
			 *
			 * When type is PMO_ANONYM, the later faulting threads
			 * of the process do not need to modify the page
			 * table because a previous faulting thread will do
			 * that. (This is always true for the same process)
			 * However, if one process map an anonymous pmo for
			 * another process (e.g., main stack pmo), the faulting
			 * thread (e.g, in the new process) needs to update its
			 * page table.
			 * So, for simplicity, we just update the page table.
			 * Note that adding the same mapping is harmless.
			 *
			 * When type is PMO_SHM and PMO_FILE, the later
			 * faulting threads
			 * needs to add the mapping in the page table.
			 * Repeated mapping operations are harmless.
			 */
			if (pmo->type == PMO_SHM || pmo->type == PMO_FILE
			    || pmo->type == PMO_ANONYM) {
				kdebug("fault_addr=%p pa=%p\n", fault_addr, pa);
				/* Add mapping in the page table */
				lock(&vmspace->pgtbl_lock);
				fault_addr = ROUND_DOWN(fault_addr, PAGE_SIZE);
				map_range_in_pgtbl(vmspace, fault_addr, pa,
						   PAGE_SIZE, perm);
				unlock(&vmspace->pgtbl_lock);
			}
		}
		break;
	}
	case PMO_FORBID: {
		kinfo("Forbidden memory access (pmo->type is PMO_FORBID).\n");
	}
	default: {
		kinfo("handle_trans_fault: faulting vmr->pmo->type"
		      "(pmo type %d at 0x%lx)\n", vmr->pmo->type, fault_addr);
		kinfo("Currently, this pmo type should not trigger pgfaults\n");
		kprint_vmr(vmspace);
		BUG_ON(1);
		return -ENOMAPPING;
	}
	}

	unlock(&vmspace->vmspace_lock);

	return ret;
}

#endif
