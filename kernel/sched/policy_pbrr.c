/* Scheduler related functions are implemented here */
#include <arch/machine/smp.h>
#include <common/errno.h>
#include <common/kprint.h>
#include <common/list.h>
#include <common/lock.h>
#include <common/macro.h>
#include <common/util.h>
#include <irq/irq.h>
#include <machine.h>
#include <mm/kmalloc.h>
#include <object/thread.h>
#include <sched/context.h>
#include <sched/sched.h>
#include <sched/fpu.h>

/* in arch/sched/idle.S */
void idle_thread_routine(void);

// TODO(tcz): Avoid cache thrashing, this requires knowing cacheline size at
// compile time for different plats
/*
 * pbrr_ready_queue
 * Per-CPU, per-priority queue for ready tasks.
 */
struct list_head pbrr_ready_queue[PLAT_CPU_NUM][PRIO_NUM];
struct lock pbrr_ready_queue_lock[PLAT_CPU_NUM][PRIO_NUM];

struct thread idle_threads[PLAT_CPU_NUM];

/*
 * Sched_enqueue w/o lock
 */
int __pbrr_sched_enqueue(struct thread *thread, int cpuid)
{
	int prio = thread->thread_ctx->prio;

	/* Already in a ready queue */
	if (thread->thread_ctx->state == TS_READY) {
		return -EINVAL;
	}
	thread->thread_ctx->cpuid = cpuid;
	thread->thread_ctx->state = TS_READY;
	list_append(&(thread->ready_queue_node),
		    &(pbrr_ready_queue[cpuid][prio]));
	return 0;
}

/*
 * Sched_enqueue
 * Put `thread` at the end of ready queue of assigned `affinity` and `prio`.
 * If affinity = NO_AFF, assign the core to the current cpu.
 */
int pbrr_sched_enqueue(struct thread *thread)
{
	BUG_ON(thread == NULL);
	BUG_ON(thread->thread_ctx == NULL);

	u32 cpuid = 0, prio = 0, ret = 0;
	u32 local_cpu_id = 0;

	local_cpu_id = smp_get_cpu_id();

	/* Check AFF and set cpuid */
	if (thread->thread_ctx->affinity == NO_AFF)
		cpuid = local_cpu_id;
	else
		cpuid = thread->thread_ctx->affinity;

	/* Check Prio and set prio */
	prio = thread->thread_ctx->prio;
	if (prio > MAX_PRIO)
		return -EINVAL;

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	if (cpuid != local_cpu_id) {
		save_and_release_fpu(thread);
	}
#endif

	lock(&pbrr_ready_queue_lock[cpuid][prio]);
	ret = __pbrr_sched_enqueue(thread, cpuid);
	unlock(&pbrr_ready_queue_lock[cpuid][prio]);
	return ret;
}

int __pbrr_sched_dequeue(struct thread *thread)
{
	/* Not in a ready queue */
	if (thread->thread_ctx->state != TS_READY) {
		return -EINVAL;
	}
	list_del(&(thread->ready_queue_node));
	thread->thread_ctx->state = TS_INTER;
	return 0;
}

/*
 * remove `thread` from its current residual ready queue
 */
int pbrr_sched_dequeue(struct thread *thread)
{
	BUG_ON(thread == NULL);
	BUG_ON(!thread->thread_ctx);

	u32 cpuid = 0, prio = 0, ret = 0;

	cpuid = thread->thread_ctx->cpuid;
	prio = thread->thread_ctx->prio;

	lock(&pbrr_ready_queue_lock[cpuid][prio]);
	ret = __pbrr_sched_dequeue(thread);
	unlock(&pbrr_ready_queue_lock[cpuid][prio]);
	return ret;
}

/*
 * Choose an appropriate thread.
 * A simple policy is adapted here: choose the thread with the highest priority.
 */
// TODO(TCZ): this policy will always choose root task for cpu 0
// And this policy will pick idle thread when there is task at MIN_PRIO
struct thread *pbrr_sched_choose_thread(void)
{
	u32 cpuid = smp_get_cpu_id();
	int i = 0;
	struct thread *thread = 0;

	for (i = MAX_PRIO; i >= MIN_PRIO; i--) {
		if (!list_empty(&pbrr_ready_queue[cpuid][i])) {
			lock(&pbrr_ready_queue_lock[cpuid][i]);
			if (list_empty(&pbrr_ready_queue[cpuid][i])) {
				unlock(&pbrr_ready_queue_lock[cpuid][i]);
				continue;
			}
			/*
			 * When the thread is just moved from another cpu and
			 * the kernel stack is used by the origina core, try
			 * to find another thread. At least idle thead is capable
			 * to run with lowest priority.
			 */
			thread = find_next_runnable_thread_in_list(
					&pbrr_ready_queue[cpuid][i]);
			if (!thread) {
				unlock(&pbrr_ready_queue_lock[cpuid][i]);
				continue;
			}

			BUG_ON(__pbrr_sched_dequeue(thread));
			unlock(&pbrr_ready_queue_lock[cpuid][i]);
			return thread;
		}
	}

	BUG_ON(1);
	return NULL;
}

void pbrr_top(void)
{
	u32 cpuid = 0;
	int i;
	struct thread *thread;

	for (cpuid = 0; cpuid < 4; cpuid++) {
		thread = current_threads[cpuid];
		if (thread != NULL)
			print_thread(thread);
		for (i = MAX_PRIO; i >= MIN_PRIO; i--) {
			if (!list_empty(&pbrr_ready_queue[cpuid][i])) {
				for_each_in_list(thread, struct thread,
						 ready_queue_node,
						 &pbrr_ready_queue[cpuid][i]) {
					print_thread(thread);
				}
			}
		}
	}
}

static inline void pbrr_sched_refill_budget(struct thread *target, u32 budget)
{
	target->thread_ctx->sc->budget = budget;
}

/*
 * Schedule a thread to execute.
 * current_thread can be NULL, or the state is TS_RUNNING or TS_WAITING.
 * This function will suspend current running thread, if any, and schedule
 * another thread from `pbrr_ready_queue[cpu_id]`.
 * ***the following text might be outdated***
 * 1. Choose an appropriate thread through calling *chooseThread* (Simple
 * Priority-Based Policy)
 * 2. Update current running thread and left the caller to restore the executing
 * context
 */
int pbrr_sched(void)
{
	/* WITH IRQ Disabled */
	struct thread *old = current_thread;
	struct thread *new = 0;

	if (old) {
		BUG_ON(old->thread_ctx->sc == NULL);
		BUG_ON(old->thread_ctx == NULL);
		BUG_ON(old->thread_ctx->state != TS_RUNNING
		       && old->thread_ctx->state != TS_WAITING);

		/*
		 * If previous thread is running, equeue it.
		 * If it is warting, do nothing.
		 */
		if (current_thread->thread_ctx->state == TS_RUNNING) {
			if (old->thread_ctx->sc->budget != 0) {
				kdebug("Sched: No schedule needed!\n");
				return 0;
			}

			kdebug("Sched: Refill the budget and enqueue the RQ\n");
			pbrr_sched_refill_budget(old, DEFAULT_BUDGET);
			old->thread_ctx->state = TS_INTER;
			BUG_ON(pbrr_sched_enqueue(old) != 0);
		}
	}

	BUG_ON(!(new = pbrr_sched_choose_thread()));
	switch_to_thread(new);
	return 0;
}

int pbrr_sched_init(void)
{
	int i = 0, j = 0;
	char idle_name[] = "KNL-IDEL-PBRR";
	int name_len = strlen(idle_name);

	/* Initialize global variables */
	for (i = 0; i < PLAT_CPU_NUM; i++) {
		current_threads[i] = NULL;
		for (j = 0; j < PRIO_NUM; j++) {
			init_list_head(&pbrr_ready_queue[i][j]);
			lock_init(&pbrr_ready_queue_lock[i][j]);
		}
	}

	kdebug("Scheduler create idle threads for each core\n");
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
		sched_enqueue(&idle_threads[i]);
	}
	kdebug("Scheduler initialized. Create %d idle threads.\n", i);

	return 0;
}

struct sched_ops pbrr = {.sched_init = pbrr_sched_init,
	.sched = pbrr_sched,
	.sched_enqueue = pbrr_sched_enqueue,
	.sched_dequeue = pbrr_sched_dequeue,
	.sched_top = pbrr_top
};
