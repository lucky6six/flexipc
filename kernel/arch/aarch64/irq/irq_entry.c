#include <irq/irq.h>
#include <common/types.h>
#include <common/kprint.h>
#include <common/util.h>
#include <sched/sched.h>
#include <sched/fpu.h>
#include <arch/machine/smp.h>
#include "irq_entry.h"
#include "esr.h"

u8 irq_handle_type[MAX_IRQ_NUM];

void arch_enable_irqno(int irq)
{
	plat_enable_irqno(irq);
}

void arch_interrupt_init_per_cpu(void)
{
	/* platform dependent init */
	plat_interrupt_init();
	set_exception_vector();
	disable_irq();
}

void arch_interrupt_init(void)
{
	arch_interrupt_init_per_cpu();
	memset(irq_handle_type, HANDLE_KERNEL, MAX_IRQ_NUM);
}

void handle_entry_c(int type, u64 esr, u64 address)
{
	/* ec: exception class */
	u32 esr_ec = GET_ESR_EL1_EC(esr);

	/* Currently, ChCore only handles a part of IRQs */
	if (type < SYNC_EL0_64) {
		if (esr_ec != ESR_EL1_EC_DABT_CEL) {
			kinfo("%s: irq type is %d\n", __func__, type);
			BUG_ON(1);
		}
	}

	kdebug("Interrupt type: %d, ESR: 0x%lx, Fault address: 0x%lx, EC 0b%b\n",
	       type, esr, address, esr_ec);
	/* Dispatch exception according to EC */
	switch (esr_ec) {
	case ESR_EL1_EC_UNKNOWN:
		kdebug("Unknown\n");
		break;
	case ESR_EL1_EC_WFI_WFE:
		kdebug("Trapped WFI or WFE instruction execution\n");
		/* FIXME: switch to new thread here */
		return;
	case ESR_EL1_EC_ENFP:
#if FPU_SAVING_MODE == LAZY_FPU_MODE
		/*
		 * Disable FPU in EL1: IRQ type is SYNC_EL1h (4).
		 * Disable FPU in EL0: IRQ type is SYNC_EL0_64 (8).
		 */
		change_fpu_owner();
		return;
#else
		kdebug("Access to SVE, Advanced SIMD, or floating-point functionality\n");
		break;
#endif
	case ESR_EL1_EC_ILLEGAL_EXEC:
		kdebug("Illegal Execution state\n");
		break;
	case ESR_EL1_EC_SVC_32:
		kdebug("SVC instruction execution in AArch32 state\n");
		break;
	case ESR_EL1_EC_SVC_64:
		kdebug("SVC instruction execution in AArch64 state\n");
		break;
	case ESR_EL1_EC_MRS_MSR_64:
		kdebug("Using MSR or MRS from a lower Exception level\n");
		break;
	case ESR_EL1_EC_IABT_LEL:
		kdebug("Instruction Abort from a lower Exception level\n");
		/* Page fault handler here:
		 * dynamic loading can trigger faults here.
		 */
		do_page_fault(esr, address);
		return;
	case ESR_EL1_EC_IABT_CEL:
		kinfo("Instruction Abort from current Exception level\n");
		break;
	case ESR_EL1_EC_PC_ALIGN:
		kdebug("PC alignment fault exception\n");
		break;
	case ESR_EL1_EC_DABT_LEL:
		kdebug("Data Abort from a lower Exception level\n");
		/* Handle faults caused by data access.
		 * We only consider page faults for now.
		 */
		do_page_fault(esr, address);
		return;
	case ESR_EL1_EC_DABT_CEL:
		kdebug("Data Abort from a current Exception level\n");
		do_page_fault(esr, address);
		return;
	case ESR_EL1_EC_SP_ALIGN:
		kdebug("SP alignment fault exception\n");
		break;
	case ESR_EL1_EC_FP_32:
		kdebug("Trapped floating-point exception taken from AArch32 state\n");
		break;
	case ESR_EL1_EC_FP_64:
		kdebug("Trapped floating-point exception taken from AArch64 state\n");
		break;
	case ESR_EL1_EC_SError:
		kdebug("SERROR\n");
		break;
	default:
		kdebug("Unsupported Exception ESR %lx\n", esr);
		break;
	}

	BUG_ON(1);
}

void handle_irq(void)
{
	plat_handle_irq();
	sched();
	eret_to_thread(switch_context());
}

void sys_perf_start(void)
{
	kdebug("Disable TIMER\n");
	plat_disable_timer();
}

void sys_perf_end(void)
{
	kdebug("Enable TIMER\n");
	plat_enable_timer();
}

void __eret_to_thread(u64 sp);

void eret_to_thread(u64 sp) {
	__eret_to_thread(sp);
}
