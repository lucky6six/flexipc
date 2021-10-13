#include <object/thread.h>
#include <sched/sched.h>
#include <machine.h>
#include <arch/machine/registers.h>
#include <arch/machine/smp.h>
#include <mm/kmalloc.h>
#include <seg.h>

void init_thread_ctx(struct thread *thread, u64 stack, u64 func, u32 prio, u32 type, s32 aff)
{
	/* Init thread registers */
        thread->thread_ctx->ec.reg[RSP] = stack;
        thread->thread_ctx->ec.reg[RIP] = func;
	/* Set thread segment (Lower 2 bits are CPL) */
	thread->thread_ctx->ec.reg[DS] = UDSEG | 3;
	thread->thread_ctx->ec.reg[CS] = UCSEG | 3;
	thread->thread_ctx->ec.reg[SS] = UDSEG | 3;

	/* Set thread flag according to type*/
	if (type == TYPE_KERNEL)
		thread->thread_ctx->ec.reg[RFLAGS] = EFLAGS_1;
	else
		thread->thread_ctx->ec.reg[RFLAGS] = EFLAGS_DEFAULT;

        /* Set the priority and state of the thread */
	thread->thread_ctx->prio = prio;
	thread->thread_ctx->state = TS_INIT;

	/* Set the affinity */
	thread->thread_ctx->cpuid = 0;
	thread->thread_ctx->affinity = aff;

	/* Set the thread type */
	thread->thread_ctx->type = type;

	/* Set the budget of the thread */
	thread->thread_ctx->sc = kmalloc(sizeof(sched_cont_t));
	thread->thread_ctx->sc->budget = DEFAULT_BUDGET;

	/* Set special state */
	thread->thread_ctx->kernel_stack_state = KS_FREE;
}

u64 arch_get_thread_stack(struct thread *thread)
{
	return thread->thread_ctx->ec.reg[RSP];
}

void arch_set_thread_stack(struct thread *thread, u64 stack)
{
        thread->thread_ctx->ec.reg[RSP] = stack;
}

void arch_set_thread_return(struct thread *thread, u64 ret)
{
        thread->thread_ctx->ec.reg[RAX] = ret;
}

void arch_set_thread_next_ip(struct thread *thread, u64 ip)
{
        thread->thread_ctx->ec.reg[RIP] = ip;
}

u64 arch_get_thread_next_ip(struct thread *thread)
{
        return thread->thread_ctx->ec.reg[RIP];
}

void arch_set_thread_info_page(struct thread *thread, u64 info_page_addr)
{
        /* Maybe we don't need this */
	thread->thread_ctx->ec.reg[RDI] = info_page_addr;
}

/* The first argument */
void arch_set_thread_arg(struct thread *thread, u64 arg)
{
	thread->thread_ctx->ec.reg[RDI] = arg;
}

void arch_set_thread_tls(struct thread *thread, u64 tls)
{
	thread->thread_ctx->tls_base_reg[TLS_FS] = tls;	
}

/* Set client_badge for IPC (as the second argument) */
void arch_set_thread_badge_arg(struct thread *thread, u64 badge)
{
	thread->thread_ctx->ec.reg[RSI] = badge;
}

void arch_enable_interrupt(struct thread *thread)
{
	kinfo("Enable interrupt\n");
	thread->thread_ctx->ec.reg[RFLAGS] |= EFLAGS_IF ;
}

void arch_disable_interrupt(struct thread *thread)
{
	kinfo("Disable interrupt\n");
	thread->thread_ctx->ec.reg[RFLAGS] &= ~EFLAGS_IF ;
}

/*
 * Saving registers related to thread local storage.
 * On x86_64, FS/GS is used by convention.
 */
void switch_tls_info(struct thread *from, struct thread *to)
{
	if (likely ((from) &&
		    (from->thread_ctx->type > TYPE_KERNEL))) {
		/* Save FS for thread from */
		from->thread_ctx->tls_base_reg[TLS_FS] = __builtin_ia32_rdfsbase64();
		/* Save GS for thread from */
		// TODO: rdmsr triggers VMExit, so we disable it for now
		// from->thread_ctx->tls_base_reg[TLS_GS] = rdmsr(MSR_GSBASE);
	}

	if (likely ((to) &&
		    (to->thread_ctx->type > TYPE_KERNEL))) {
		/* Restore FS for thread to */
		__builtin_ia32_wrfsbase64(to->thread_ctx->tls_base_reg[TLS_FS]);
		/* Restore GS for thread to */
		// TODO: rdmsr triggers VMExit (if running in QEMU), so we disable it for now
		// wrmsr(MSR_GSBASE, to->thread_ctx->tls_base_reg[TLS_GS]);
	}
}

