#include <common/types.h>
#include <object/thread.h>
#include <mm/vmspace.h>

#include "esr.h"

// declarations of fault handlers
int handle_trans_fault(struct vmspace *vmspace, vaddr_t fault_addr);

static inline vaddr_t get_fault_addr()
{
	vaddr_t addr;
	asm volatile ("mrs %0, far_el1\n\t" :"=r" (addr));
	return addr;
}

// EC: Instruction Abort or Data Abort
void do_page_fault(u64 esr, u64 fault_ins_addr)
{
	vaddr_t fault_addr;
	int fsc; // fault status code

	fault_addr = get_fault_addr();
	fsc = GET_ESR_EL1_FSC(esr);
	switch (fsc) {
	case DFSC_TRANS_FAULT_L0:
	case DFSC_TRANS_FAULT_L1:
	case DFSC_TRANS_FAULT_L2:
	case DFSC_TRANS_FAULT_L3: {
		int ret;

		ret = handle_trans_fault(current_thread->vmspace, fault_addr);
		if (ret != 0) {
			kinfo("do_page_fault: faulting ip is 0x%lx,"
			      "faulting address is 0x%lx,"
			      "fsc is trans_fault (0b%b)\n",
			      fault_ins_addr, fault_addr, fsc);
			kprint_vmr(current_thread->vmspace);

			kinfo("current_cap_group is %s\n",
			      current_cap_group->cap_group_name);

			// TODO: kill the process
			BUG_ON(ret != 0);
		}
		break;
	}
	case DFSC_PERM_FAULT_L1:
	case DFSC_PERM_FAULT_L2:
	case DFSC_PERM_FAULT_L3:
		// TODO: supprt COW here?

		kinfo("do_page_fault: faulting ip is 0x%lx,"
			      "faulting address is 0x%lx,"
			      "fsc is perm_fault (0b%b)\n",
			      fault_ins_addr, fault_addr, fsc);
		BUG_ON(1);
		break;
	case DFSC_ACCESS_FAULT_L1:
	case DFSC_ACCESS_FAULT_L2:
	case DFSC_ACCESS_FAULT_L3:
		kinfo("do_page_fault: fsc is access_fault (0x%b)\n", fsc);
		BUG_ON(1);
		break;
	default:
		kinfo("do_page_fault: fsc is unsupported (0x%b) now\n", fsc);
		BUG_ON(1);
		break;
	}
	// TODO: copy on write
}