/* Scheduler related functions are implemented here */
#include <sched/sched.h>
#include <arch/machine/smp.h>
#include <common/kprint.h>
#include <machine.h>
#include <mm/kmalloc.h>
#include <common/list.h>
#include <common/util.h>
#include <object/thread.h>
#include <common/macro.h>
#include <common/errno.h>
#include <common/lock.h>
#include <object/thread.h>
#include <irq/irq.h>
#include <sched/context.h>
#include <sched/fpu.h>

struct thread *current_threads[PLAT_CPU_NUM];

/* For TLB maintenence */
extern void record_history_cpu(struct vmspace *vmspcae, u32 cpuid);

/* Chosen Scheduling Policies */
struct sched_ops *cur_sched_ops;

char thread_type[][TYPE_STR_LEN] = {
	"IDLE  ",
	"KERNEL",
	"USER  ",
	"SHADOW  ",
	"TESTS"
};

char thread_state[][STATE_STR_LEN] = {
	"TS_INIT      ",
	"TS_READY     ",
	"TS_INTER     ",
	"TS_RUNNING   ",
	"TS_EXIT      ",
	"TS_WAITING   ",
	"TS_EXITING   "
};

void print_thread(struct thread *thread)
{
	printk("Thread %p\tType: %s\tState: %s\tCPU %d\tAFF %d\tBudget %d\tPrio: %d\tCMD: %s\n",
		thread,
		thread_type[thread->thread_ctx->type],
		thread_state[thread->thread_ctx->state],
		thread->thread_ctx->cpuid,
		thread->thread_ctx->affinity,
		thread->thread_ctx->sc->budget,
		thread->thread_ctx->prio,
		thread->cap_group->cap_group_name);
}

int sched_is_running(struct thread *target)
{
	BUG_ON(!target);
	BUG_ON(!target->thread_ctx);

	if (target->thread_ctx->state == TS_RUNNING)
		return 1;
	return 0;
}

/*
 * Switch Thread to the specified one.
 * Set the correct thread state to running and
 * the per_cpu varible `current_thread`.
 *
 * Note: the switch is between current_thread and target.
 */

int switch_to_thread(struct thread *target)
{
	BUG_ON(!target);
	BUG_ON(!target->thread_ctx);
	BUG_ON((target->thread_ctx->state == TS_READY));

	/* No thread switch happens actually */
	if (target == current_thread) {
		target->thread_ctx->state = TS_RUNNING;

		/* The previous thread is the thread itself */
		target->prev_thread = THREAD_ITSELF;
		return 0;
	}

	target->thread_ctx->cpuid = smp_get_cpu_id();
	target->thread_ctx->state = TS_RUNNING;

	/* Record the thread transferring the CPU */
	target->prev_thread = current_thread;

#if FPU_SAVING_MODE == EAGER_FPU_MODE
	save_fpu_state(current_thread);
	restore_fpu_state(target);
#else
	/* FPU_SAVING_MODE == LAZY_FPU_MODE */
	if (target->thread_ctx->type > TYPE_KERNEL)
		disable_fpu_usage();
#endif

	/*
	 * Switch the TLS information:
	 * Save the tls info for current_thread,
	 * and restore the tls info for target.
	 */
	switch_tls_info(current_thread, target);

	target->thread_ctx->kernel_stack_state = KS_LOCKED;

	/*
	 * An important assumption: Per-CPU variable current_thread
	 * is only accessed by its owner CPU.
	 *
	 * TODO: Otherwise, consider about using barrier.
	 */
	// smp_wmb();
	current_thread = target;

	return 0;
}

/*
 * Switch vmspace and arch-related stuff
 * Return the context pointer which should be set to stack pointer register
 */
u64 switch_context(void)
{
	/* TODO: with IRQ disabled.
	 * tmac: what if IRQ is not disabled? But directly resumes the execution
	 * after interrupts.
	 */
	struct thread *target_thread;
	struct thread_ctx *target_ctx;

	target_thread = current_thread;
	BUG_ON(!target_thread);
	BUG_ON(!target_thread->thread_ctx);

	target_ctx = target_thread->thread_ctx;

	if (target_thread->prev_thread == THREAD_ITSELF)
		return (u64) target_ctx;

	/* These 3 types of thread do not have vmspace */
	if (target_thread->thread_ctx->type != TYPE_IDLE &&
		target_thread->thread_ctx->type != TYPE_KERNEL &&
		target_thread->thread_ctx->type != TYPE_TESTS) {

		BUG_ON(!target_thread->vmspace);
		/*
		* Recording the CPU the thread runs on: for TLB maintainence.
		* switch_context is always required for running a (new) thread.
		* So, we invoke record_running_cpu here.
		*/
		record_history_cpu(target_thread->vmspace, smp_get_cpu_id());
		BUG_ON(!target_thread->vmspace);
		switch_thread_vmspace_to(target_thread);
	}
	arch_switch_context(target_thread);

	return (u64) target_ctx;
}

/*
 * Only threads whose kernel_stack_state is free can be choosed.
 * Also, the thread should not be TS_EXITING state.
 *
 * TODO (LN): && thread->thread_state != TS_EXITING
 */
struct thread *find_next_runnable_thread_in_list(struct list_head *thread_list)
{
	struct thread *thread;

	for_each_in_list(thread, struct thread, ready_queue_node, thread_list) {
		if (thread->thread_ctx->kernel_stack_state == KS_FREE
			|| thread == current_thread) {
			return thread;
		}
	}
	return NULL;
}

void finish_switch()
{
	struct thread *prev_thread;

	prev_thread = current_thread->prev_thread;
	// BUG_ON(prev_thread == current_thread);
	if ((prev_thread == THREAD_ITSELF) || (prev_thread == NULL))
		return;

	/* This flag is checked during IPC.
	 * TODO: do we need any fence to ensure everthing is done
	 * before setting the stack_state.
	 */
	prev_thread->thread_ctx->kernel_stack_state = KS_FREE;
}

/* SYSCALL functions */

void sys_yield(void)
{
	struct thread *thread = current_thread;
	BUG_ON(!thread);

	thread->thread_ctx->sc->budget = 0;
	sched();
	eret_to_thread(switch_context());
}

void sys_top(void){
	cur_sched_ops->sched_top();
}

int sched_init(struct sched_ops *sched_ops)
{
	BUG_ON(sched_ops == NULL);

	cur_sched_ops = sched_ops;
	cur_sched_ops->sched_init();
	return 0;
}

/* Performance syscall */
void sys_perf_null(void)
{
#ifdef NO_VMSWITCH
	current_thread->thread_ctx->sc->budget = DEFAULT_BUDGET;
	// sched_enqueue(target);
	// sched();
	// eret_to_thread(switch_context());
	eret_to_thread((u64)target->thread_ctx);
#elif defined(NORMAL)
	/* Set thread state */
	current_thread->thread_ctx->sc->budget = DEFAULT_BUDGET;
	/* Set thread state */
	current_thread = NULL;
	BUG_ON(sched_enqueue(target));
	/* Reschedule */
	sched();
	eret_to_thread(switch_context());
#endif
}
