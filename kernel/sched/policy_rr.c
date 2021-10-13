/* Scheduler related functions are implemented here */
#include <sched/sched.h>
#include <sched/fpu.h>
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

/* in arch/sched/idle.S */
void idle_thread_routine(void);

/*
 * rr_ready_queue
 * Per-CPU ready queue for ready tasks.
 */
struct list_head rr_ready_queue[PLAT_CPU_NUM];
struct lock rr_ready_queue_lock[PLAT_CPU_NUM];

/*
 * RR policy also has idle threads.
 * When no active user threads in ready queue,
 * we will choose the idle thread to execute.
 * Idle thread will **NOT** be in the RQ.
 */
struct thread idle_threads[PLAT_CPU_NUM];

int __rr_sched_enqueue(struct thread *thread, int cpuid)
{
	/* Already in the ready queue */
	if (thread->thread_ctx->state == TS_READY) {
		return -EINVAL;
	}
	thread->thread_ctx->cpuid = cpuid;
	thread->thread_ctx->state = TS_READY;
	list_append(&(thread->ready_queue_node), &(rr_ready_queue[cpuid]));

	return 0;
}

/*
 * Sched_enqueue
 * Put `thread` at the end of ready queue of assigned `affinity` and `prio`.
 * If affinity = NO_AFF, assign the core to the current cpu.
 * If the thread is IDEL thread, do nothing!
 */
int rr_sched_enqueue(struct thread *thread)
{
	BUG_ON(!thread);
	BUG_ON(!thread->thread_ctx);

	u32 cpuid = 0;
	int ret = 0;
	u32 local_cpuid = 0;

	if (thread->thread_ctx->type == TYPE_IDLE)
		return 0;

	local_cpuid = smp_get_cpu_id();

	/* Check AFF and set cpuid */
	if (thread->thread_ctx->affinity == NO_AFF)
		cpuid = local_cpuid;
	else
		cpuid = thread->thread_ctx->affinity;

	/* Check Prio */
	if (thread->thread_ctx->prio > MAX_PRIO)
		return -EINVAL;

	if (cpuid >= PLAT_CPU_NUM) {
		return -EINVAL;
	}

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	if (cpuid != local_cpuid) {
		/*
		 * @thread will be migrated to other CPUs.
		 * So, we have to save its FPU states
		 * if it is the owner of current CPU.
		 */
		//kinfo("local: %d, remote: %d, invoke save_and_release\n",
		  //    cpuid, local_cpuid);
		save_and_release_fpu(thread);
	}
#endif

	lock(&rr_ready_queue_lock[cpuid]);
	ret = __rr_sched_enqueue(thread, cpuid);
	unlock(&rr_ready_queue_lock[cpuid]);
	return ret;
}

/* dequeue w/o lock */
int __rr_sched_dequeue(struct thread *thread)
{
	if (thread->thread_ctx->state != TS_READY)
		return -EINVAL;
	list_del(&(thread->ready_queue_node));
	thread->thread_ctx->state = TS_INTER;
	return 0;
}

/*
 * remove `thread` from its current residual ready queue
 */
int rr_sched_dequeue(struct thread *thread)
{
	BUG_ON(!thread);
	BUG_ON(!thread->thread_ctx);
	/* IDLE thread will **not** be in any ready queue */
	BUG_ON(thread->thread_ctx->type == TYPE_IDLE);

	u32 cpuid = 0;
	int ret = 0;

	cpuid = thread->thread_ctx->cpuid;
	lock(&rr_ready_queue_lock[cpuid]);
	ret = __rr_sched_dequeue(thread);
	unlock(&rr_ready_queue_lock[cpuid]);
	return ret;
}

/*
 * Choose an appropriate thread and dequeue from ready queue
 */
struct thread *rr_sched_choose_thread(void)
{
	u32 cpuid = smp_get_cpu_id();
	struct thread *thread = 0;

find_again:
	if (!list_empty(&rr_ready_queue[cpuid])) {
		lock(&rr_ready_queue_lock[cpuid]);
		if (list_empty(&rr_ready_queue[cpuid])) {
			unlock(&rr_ready_queue_lock[cpuid]);
			/* Find again the ready queue */
			goto find_again;
		}
		/*
		 * When the thread is just moved from another cpu and
		 * the kernel stack is used by the origina core, try
		 * to find another thread.
		 */
		thread = find_next_runnable_thread_in_list(
				&rr_ready_queue[cpuid]);
		if (!thread) {
			unlock(&rr_ready_queue_lock[cpuid]);
			return &idle_threads[cpuid];
		}

		BUG_ON(__rr_sched_dequeue(thread));
		unlock(&rr_ready_queue_lock[cpuid]);
		return thread;
	}
	return &idle_threads[cpuid];
}

static inline void rr_sched_refill_budget(struct thread *target, u32 budget)
{
	target->thread_ctx->sc->budget = budget;
}

/*
 * Schedule a thread to execute.
 * current_thread can be NULL, or the state is TS_RUNNING or TS_WAITING.
 * This function will suspend current running thread, if any, and schedule
 * another thread from `rr_ready_queue[cpu_id]`.
 * ***the following text might be outdated***
 * 1. Choose an appropriate thread through calling *chooseThread* (Simple
 * Priority-Based Policy)
 * 2. Update current running thread and left the caller to restore the executing
 * context
 */
int rr_sched(void)
{
	/* WITH IRQ Disabled */
	struct thread *old = current_thread;
	struct thread *new = 0;

	if (old) {
		BUG_ON(!old->thread_ctx);
		BUG_ON(!old->thread_ctx->sc);
		BUG_ON(old->thread_ctx->state != TS_RUNNING
		       && old->thread_ctx->state != TS_WAITING);
		/*
		 * If previous thread is running, equeue it.
		 * If it is waiting, do nothing.
		 */
		if (old->thread_ctx->state == TS_RUNNING) {
			if (old->thread_ctx->sc->budget > 0) {
				kdebug("Sched: No schedule needed!\n");
				return 0;
			}
			rr_sched_refill_budget(old, DEFAULT_BUDGET);
			old->thread_ctx->state = TS_INTER;
			BUG_ON(rr_sched_enqueue(old));
		}
	}

	BUG_ON(!(new = rr_sched_choose_thread()));
	switch_to_thread(new);
	return 0;
}

int rr_sched_init(void)
{
	int i = 0;
	char idle_name[] = "KNL-IDEL-RR";
	int name_len = strlen(idle_name);

	/* Initialize global variables */
	for (i = 0; i < PLAT_CPU_NUM; i++) {
		current_threads[i] = NULL;
		init_list_head(&rr_ready_queue[i]);
		lock_init(&rr_ready_queue_lock[i]);
	}

	/* Create a fake idle cap group to store the name */
	struct cap_group *idle_cap_group = kzalloc(sizeof(struct cap_group));
	memset(idle_cap_group->cap_group_name, 0, MAX_GROUP_NAME_LEN);
	if (name_len > MAX_GROUP_NAME_LEN)
		name_len = MAX_GROUP_NAME_LEN;
	memcpy(idle_cap_group->cap_group_name, idle_name, name_len);

	/* Initialize one idle thread for each core and insert into the RQ */
	for (i = 0; i < PLAT_CPU_NUM; i++) {
		/* Set the thread context of the idle threads */
		BUG_ON(!(idle_threads[i].thread_ctx = create_thread_ctx()));
		/* We will set the stack and func ptr in arch_idle_ctx_init */
		init_thread_ctx(&idle_threads[i], 0, 0, MIN_PRIO, TYPE_IDLE, i);
		/* Call arch-dependent function to fill the context of the idle
		 * threads */
		arch_idle_ctx_init(idle_threads[i].thread_ctx,
				   idle_thread_routine);
		/* Idle thread is kernel thread which do not have vmspace */
		idle_threads[i].vmspace = NULL;
		idle_threads[i].cap_group = idle_cap_group;
	}
	kdebug("Scheduler initialized. Create %d idle threads.\n", i);

	return 0;
}

void rr_top(void)
{
	u32 cpuid = smp_get_cpu_id();
	struct thread *thread;

	printk("Current CPU %d\n", cpuid);
	for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
		lock(&rr_ready_queue_lock[cpuid]);
	}
	for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
		printk("===== CPU %d =====\n", cpuid);
		thread = current_threads[cpuid];
		if (thread != NULL) {
			print_thread(thread);
		}
		if (!list_empty(&rr_ready_queue[cpuid])) {
			for_each_in_list(thread, struct thread,
					 ready_queue_node,
					 &rr_ready_queue[cpuid]) {
				print_thread(thread);
			}
		}
	}
	for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
		unlock(&rr_ready_queue_lock[cpuid]);
	}
}

struct sched_ops rr = {
	.sched_init = rr_sched_init,
	.sched = rr_sched,
	.sched_enqueue = rr_sched_enqueue,
	.sched_dequeue = rr_sched_dequeue,
	.sched_top = rr_top
};
