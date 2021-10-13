#include "irq_entry.h"
#include <arch/x2apic.h>
#include <seg.h>
#include <machine.h>
#include <common/kprint.h>
#include <common/util.h>
#include <common/macro.h>
#include <arch/sched/arch_sched.h>
#include <arch/io.h>
#include <arch/time.h>
#include <irq/timer.h>
#include <object/thread.h>
#include <object/irq.h>
#include <irq/irq.h>
#include <sched/sched.h>
#include <sched/fpu.h>
#include <mm/vmspace.h>

/* idt and idtr */
struct gate_desc idt[T_NUM] __attribute__((aligned(16)));
struct pseudo_desc idtr = { sizeof(idt) - 1, (u64) idt };

/* record irq is handled by kernel or user */
u8 irq_handle_type[MAX_IRQ_NUM];

void arch_enable_irq(void)
{
	asm volatile("sti");
}

void arch_disable_irq(void)
{
	asm volatile("cli");
}

#define PIC1_BASE 0x20
#define PIC2_BASE 0xa0

void initpic(void)
{
	put8(PIC1_BASE + 1, 0xff);
	put8(PIC2_BASE + 1, 0xff);
}

void arch_enable_irqno(int irq)
{
	BUG("Not impl.");
}

void arch_interrupt_init_per_cpu(void)
{
	u32 eax, ebx, ecx, edx;

	cpuid(1, &eax, &ebx, &ecx, &edx);
	if (ecx & FEATURE_ECX_X2APIC)
		x2apic_init();
	else if (edx & FEATURE_EDX_XAPIC)
		BUG("xapic init not implemented\n");
	else
		BUG("not apic detected\n");
	arch_disable_irq();
	initpic();
	asm volatile("lidt (%0)" : : "r" (&idtr));
}

void arch_interrupt_init(void)
{
        int i = 0;

        /* Set up interrupt gates */
        for (i = 0; i < T_NUM; i ++)
        {
                /* Set all gate to interrupt gate to be effected by eflags.interrupt_enable */
                if (i == T_BP) {
                        set_gate(idt[i], GT_INTR_GATE, KCSEG64, idt_entry[i], 3);
                } else {
                        set_gate(idt[i], GT_INTR_GATE, KCSEG64, idt_entry[i], 0);
                }
        }
        arch_interrupt_init_per_cpu();

	memset(irq_handle_type, HANDLE_KERNEL, MAX_IRQ_NUM);
}

/* Mark the end of an IRQ */
void arch_ack_irq()
{
	x2apic_eoi();
}

/*
 * This interface is only for the common interface across different
 * architectures.
 */
void plat_ack_irq(int irq)
{
	arch_ack_irq();
}

void plat_set_next_timer(u64 tick_delta)
{
	u64 tsc;
	tsc = get_cycles();
	wrmsr(MSR_IA32_TSC_DEADLINE, tsc + tick_delta);
}

void plat_handle_timer_irq(u64 tick_delta)
{
	plat_set_next_timer(tick_delta);
	arch_ack_irq();
}


/*
 * IPI receiver side:
 * Based on IPI_tx interfaces, ChCore uses the following TLB shootdown
 * protocol between different CPU cores.
 */
extern void flush_local_tlb_opt(vaddr_t start_va, u64 page_cnt, u64 pcid);
extern void clear_history_cpu(struct vmspace *vmspace, u32 cpuid);
static void handle_ipi_on_tlb_shootdown(void)
{
	int cpuid;
	u64 start_va;
	u64 page_cnt;
	u64 pcid;
	u64 vmspace;

	cpuid = smp_get_cpu_id();
	start_va = get_ipi_tx_arg(cpuid, 0);
	page_cnt = get_ipi_tx_arg(cpuid, 1);
	pcid     = get_ipi_tx_arg(cpuid, 2);
	vmspace  = get_ipi_tx_arg(cpuid, 3);

	flush_local_tlb_opt(start_va, page_cnt, pcid);

	/*
	 * If the vmspace is running on the current CPU,
	 * we should clear the history_cpu records because
	 * the vmspace will continue to run after this IPI.
	 */
	if ((u64)(current_thread->vmspace) != vmspace)
		clear_history_cpu((struct vmspace *)vmspace, cpuid);

	mark_finish_ipi_tx(cpuid);
}

void handle_irq(int irqno)
{
	int r;

	BUG_ON(irqno >= MAX_IRQ_NUM);
	if (irq_handle_type[irqno] == HANDLE_USER) {
		r = user_handle_irq(irqno);
		BUG_ON(r);
		return;
	}

	switch (irqno) {
	case IRQ_TIMER:
		handle_timer_irq();

		/* Start the scheduler */
		sched();
		eret_to_thread(switch_context());
		return;

	case IRQ_IPI_TLB:
		// kinfo("CPU %d: receive IPI on TLB.\n", smp_get_cpu_id());
		handle_ipi_on_tlb_shootdown();
		arch_ack_irq();
		return;
	case IRQ_IPI_RESCHED:
		kinfo("CPU %d: receive IPI on RESCHED.\n", smp_get_cpu_id());
		arch_ack_irq();

		/* Start the scheduler */
		sched();
		eret_to_thread(switch_context());
		return;
	default:
		kwarn("Unkown Exception\n");
	}
}

void trap_c(struct arch_exec_cont *ec)
{
        int trapno = ec->reg[TRAPNO];
        int errorcode = ec->reg[EC];

	/*
	 * When received IRQ in kernel
	 * When current_thread == TYPE_IDLE
	 * 	We should handle everything like user thread.
	 * Otherwice
	 * 	We should ignore the timer, handle other IRQ as normal.
	 */
	if (ec->reg[CS] == KCSEG64 &&	/* Trigger IRQ in kernel */
		current_thread ) {	/* Has running thread */
		BUG_ON(!current_thread->thread_ctx);
		if (current_thread->thread_ctx->type != TYPE_IDLE) {
			/* And the thread is not the IDLE thread */
			if (trapno == IRQ_TIMER) {
				/* We do not allow kernel preemption */
				/* TODO: dynamic high resulution timer */
				plat_handle_timer_irq(TICK_MS * 1000 * tick_per_us);
				return;
			}
		}
	}

	if (trapno == T_GP) {
		static int cnt = 0;
		if (cnt == 0) {
			cnt += 1;
			kinfo("General Protection Fault\n");
			kinfo("Current thread %p\n", current_thread);
			kinfo("Trap from 0x%lx EC %d Trap No. %d\n", ec->reg[RIP], errorcode, trapno);
			kinfo("DS 0x%x, CS 0x%x, RSP 0x%lx, SS 0x%x\n", ec->reg[DS], ec->reg[CS], ec->reg[RSP], ec->reg[SS]);
			kinfo("rax: 0x%lx, rdx: 0x%lx, rdi: 0x%lx\n", ec->reg[RAX], ec->reg[RDX], ec->reg[RDI]);
			kinfo("rcx: 0x%lx\n", ec->reg[RCX]);

			kprint_vmr(current_thread->vmspace);
			while(1);
		}
		// kinfo("General Protection Fault\n");
		while(1);
	}

	/* Just for kernel tracing and debugging */
	if ((trapno != IRQ_TIMER) &&
	    (trapno != T_PF) &&
	    (trapno != IRQ_IPI_TLB) &&
	    (trapno != T_NM)) {
		kinfo("Trap from 0x%lx EC %d Trap No. %d\n", ec->reg[RIP], errorcode, trapno);
		kinfo("DS 0x%x, CS 0x%x, RSP 0x%lx, SS 0x%x\n", ec->reg[DS], ec->reg[CS], ec->reg[RSP], ec->reg[SS]);
		kinfo("rax: 0x%lx, rdx: 0x%lx, rdi: 0x%lx\n", ec->reg[RAX], ec->reg[RDX], ec->reg[RDI]);
	}

        switch(trapno)
        {
                case T_DE:
                        kinfo("Divide Error\n");
			while(1);
                        break;
                case T_DB:
                        kinfo("Debug Exception\n");
			return;
                case T_NMI:
                        kinfo("Non-maskable Interrupt\n");
                        break;
                case T_BP:
                        kinfo("Break Point\n");
                        break;
                case T_OF:
                        kinfo("Overflow\n");
                        break;
                case T_BR:
                        kinfo("Bounds Range Check\n");
                        break;
                case T_UD:
                        kinfo("Undefined Opcode\n");
                        break;
                case T_NM:
                        kdebug("Device (ChCore considers FPU only) Not Available:\n");
#if FPU_SAVING_MODE == LAZY_FPU_MODE
			change_fpu_owner();
			return;
#else
			break;
#endif
                case T_DF:
                        kinfo("Double Fault\n");
                        break;
                case T_CSO:
                        kinfo("Coprocessor Segment Overrun\n");
                        break;
                case T_TS:
                        kinfo("Invalid Task Switch Segment\n");
                        break;
                case T_NP:
                        kinfo("Segment Not Present\n");
                        break;
                case T_SS:
                        kinfo("Stack Exception\n");
                        break;
                case T_GP: {
			kinfo("General Protection Fault\n");
                        while(1);
                        break;
		}
                case T_PF: {
                        kdebug("Page Fault\n");
                        /* Page Fault Handler Here! */
                        do_page_fault(errorcode, ec->reg[RIP]);
			return;
                        // break;
		}
                case T_MF:
                        kinfo("Floating Point Error\n");
                        break;
                case T_AC:
                        kinfo("Alignment Check\n");
                        break;
                case T_MC:
                        kinfo("Machine Check\n");
                        break;
                case T_XM:
                        kinfo("SIMD Floating Point Error\n");
                        break;
                case T_VE:
                        kinfo("Virtualization Exception\n");
                        break;
                default:
			handle_irq(trapno);
			return;
        }

	/*
	 * After handling the interrupts,
	 * we directly resume the execution.
	 *
	 * Rescheduling only happens after IRQ_TIMER or IRQ_IPI_RESCHED.
	 */
	return;
}

void eret_to_thread(u64 sp)
{
        struct thread_ctx *cur_thread = (struct thread_ctx *)sp;
        arch_exec_cont_t *cur_thread_ctx = &cur_thread->ec;


        switch(cur_thread_ctx->reg[EC]){
                case EC_SYSEXIT:
                        eret_to_thread_through_sys_exit(sp);
                break;
                default:
                        eret_to_thread_through_trap_exit(sp);
                break;
        }
        /* Non-reachable here */
}
