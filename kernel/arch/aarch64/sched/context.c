#include <object/thread.h>
#include <sched/sched.h>
#include <arch/machine/registers.h>
#include <arch/machine/smp.h>
#include <mm/kmalloc.h>

void init_thread_ctx(struct thread *thread, u64 stack, u64 func, u32 prio, u32 type, s32 aff)
{
	/* Fill the context of the thread */
	thread->thread_ctx->ec.reg[SP_EL0] = stack;
	thread->thread_ctx->ec.reg[ELR_EL1] = func;
	/* TODO pmode need to check later */
	thread->thread_ctx->ec.reg[SPSR_EL1] = SPSR_EL1_EL0t;

	/* Set the priority and state of the thread */
	thread->thread_ctx->prio = prio;
	thread->thread_ctx->state = TS_INIT;

	/* Set thread type */
	thread->thread_ctx->type = type;

	/* Set the cpuid and affinity */
	thread->thread_ctx->affinity = aff;

	/* Set the budget of the thread */
	thread->thread_ctx->sc = kmalloc(sizeof(sched_cont_t));
	thread->thread_ctx->sc->budget = DEFAULT_BUDGET;

	thread->thread_ctx->kernel_stack_state = KS_FREE;
}

u64 arch_get_thread_stack(struct thread *thread)
{
	return thread->thread_ctx->ec.reg[SP_EL0];
}

void arch_set_thread_stack(struct thread *thread, u64 stack)
{
	thread->thread_ctx->ec.reg[SP_EL0] = stack;
}

void arch_set_thread_return(struct thread *thread, u64 ret)
{
	thread->thread_ctx->ec.reg[X0] = ret;
}

void arch_set_thread_next_ip(struct thread *thread, u64 ip)
{
	/* Currently, we use fault PC to store the next ip */
	// thread->thread_ctx->ec.reg[FaultPC] = ip;
	/* Only required when we need to change PC */
	/* Maybe update ELR_EL1 directly */
	thread->thread_ctx->ec.reg[ELR_EL1] = ip;
}

u64 arch_get_thread_next_ip(struct thread *thread)
{
	return thread->thread_ctx->ec.reg[ELR_EL1];
}

void arch_set_thread_info_page(struct thread *thread, u64 info_page_addr)
{
	thread->thread_ctx->ec.reg[X0] = info_page_addr;
}

/* First argument in X0 */
void arch_set_thread_arg(struct thread *thread, u64 arg)
{
	thread->thread_ctx->ec.reg[X0] = arg;
}

void arch_set_thread_tls(struct thread *thread, u64 tls)
{
	thread->thread_ctx->tls_base_reg[0] = tls;
}

/* Second argument in X1 (pass badge here) */
void arch_set_thread_badge_arg(struct thread *thread, u64 badge)
{
	thread->thread_ctx->ec.reg[X1] = badge;
}

void arch_enable_interrupt(struct thread *thread)
{
	/* TODO */
	thread->thread_ctx->ec.reg[SPSR_EL1] &= ~SPSR_EL1_IRQ;
}

void arch_disable_interrupt(struct thread *thread)
{
	/* TODO */
	thread->thread_ctx->ec.reg[SPSR_EL1] |= SPSR_EL1_IRQ;
}

/*
 * Saving registers related to thread local storage.
 * On aarch64, TPIDR_EL0 is used by convention.
 */
void switch_tls_info(struct thread *from, struct thread *to)
{
	u64 tpidr_el0;

	if (likely ((from) &&
		    (from->thread_ctx->type > TYPE_KERNEL))) {

		/* Save TPIDR_EL0 for thread from */
		asm volatile("mrs %0, tpidr_el0\n" : "=r"(tpidr_el0));
		from->thread_ctx->tls_base_reg[0] = tpidr_el0;
	}

	if (likely ((to) &&
		    (to->thread_ctx->type > TYPE_KERNEL))) {
		/* Restore TPIDR_EL0 for thread to */
		tpidr_el0 = to->thread_ctx->tls_base_reg[0];
		asm volatile("msr tpidr_el0, %0\n" :: "r"(tpidr_el0));
	}
}

