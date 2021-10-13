#include <ipc/connection.h>
#include <ipc/notification.h>
#include <common/list.h>
#include <common/errno.h>
#include <object/thread.h>
#include <sched/sched.h>
#include <sched/context.h>
#include <irq/irq.h>
#include <posix/time.h>
#include <mm/uaccess.h>

void init_notific(struct notification *notifc)
{
	/* FIXME: We should check whether this process can bind a notification
	 * to a interrup */
	/* FIXME: We should check whether this interrupt has been binded to a
	 * notification */
	notifc->not_delivered_notifc_count = 0;
	notifc->waiting_threads_count = 0;
	init_list_head(&notifc->waiting_threads);
	lock_init(&notifc->notifc_lock);
}

/*
 * A waiting thread can be awoken by timeout and signal, which leads to racing.
 * We guarantee that a thread is not awoken for twice by 1. removing a thread from
 * notification waiting_threads when timeout and 2. removing a thread from
 * sleep_list when get signaled.
 * When signaled:
 *	lock(notification)
 *	remove from waiting_threads
 *      thread state = TS_READY
 *	unlock(notification)
 *
 *	if (sleep_state.cb != NULL) {
 *		lock(sleep_list)
 *		if (sleep_state.cb != NULL)
 *			remove from sleep_list
 *		unlock(sleep_list)
 *	}
 *
 * When timeout:
 *	lock(sleep_list)
 *	remove from sleep_list
 *	unlock(notification)
 *	if (thread state == TS_WAITING)
 *		remove from waiting_threads
 *	lock(notification)
 *	sleep_state.cb = NULL
 *	unlock(sleep_list)
 */

static void notific_timer_cb(struct thread *thread)
{
	struct notification *notifc;

	notifc = thread->sleep_state.pending_notific;
	thread->sleep_state.pending_notific = NULL;

	lock(&notifc->notifc_lock);
	if (thread->thread_ctx->state != TS_WAITING) {
		unlock(&notifc->notifc_lock);
		return;
	}

	list_del(&thread->notification_queue_node);
	BUG_ON(notifc->waiting_threads_count <= 0);
	notifc->waiting_threads_count--;

	arch_set_thread_return(thread, -EAGAIN);
	thread->thread_ctx->state = TS_INTER;
	BUG_ON(sched_enqueue(thread));

	unlock(&notifc->notifc_lock);
}

/* Return 0 if wait successfully, -EAGAIN otherwise */
s32 wait_notific(struct notification *notifc, bool is_block,
		 struct timespec *timeout)
{
	s32 ret = 0;

	lock(&notifc->notifc_lock);
	if (notifc->not_delivered_notifc_count > 0) {
		notifc->not_delivered_notifc_count--;
		ret = 0;
	} else {
		if (is_block) {
			/* Add this thread to waiting list */
			list_append(&current_thread->notification_queue_node,
				    &notifc->waiting_threads);
			current_thread->thread_ctx->state = TS_WAITING;
			notifc->waiting_threads_count++;
			arch_set_thread_return(current_thread, 0);

			if (timeout) {
				current_thread->sleep_state
					.pending_notific = notifc;
				enqueue_sleeper(current_thread, timeout,
						notific_timer_cb);
			}

			sched();

			/*
			 * Can only unlock if there is not any access to
			 * the old thread afterward.
			 */
			unlock(&notifc->notifc_lock);

			/*
			 * this thread will be directly waked up by
			 * a notifying thread and never return here
			 */
			eret_to_thread(switch_context());
		} else {
			ret = -EAGAIN;
		}
	}
	unlock(&notifc->notifc_lock);
	return ret;
}

static void try_remove_timeout(struct thread *target, struct notification *notifc)
{
	if (target == NULL)
		return;
	if (target->sleep_state.cb == NULL)
		return;

	try_dequeue_sleeper(target);
	BUG_ON(target->sleep_state.pending_notific
	       && notifc != target->sleep_state.pending_notific);
	target->sleep_state.pending_notific = NULL;
}

s32 signal_notific(struct notification *notifc, int enable_direct_switch)
{
	struct thread *target = NULL;

	lock(&notifc->notifc_lock);
	if (notifc->not_delivered_notifc_count > 0
	   || notifc->waiting_threads_count == 0) {
		notifc->not_delivered_notifc_count++;
	} else {
		/*
		 * Some threads have been blocked and waiting for notifc.
		 * Wake up one waiting thread
		 */
		target = list_entry(notifc->waiting_threads.next,
				    struct thread,
				    notification_queue_node);
		list_del(&target->notification_queue_node);
		notifc->waiting_threads_count--;
	}

	/* FIXME: common out */
	if (target) {
		/*
		 * If target can be scheduled on the same core, directly switch
		 * to it. Otherwise enqueue target to another core's queue.
		 */
		if (enable_direct_switch) {
			if (target->thread_ctx->affinity == NO_AFF ||
			    target->thread_ctx->affinity == smp_get_cpu_id()) {
				arch_set_thread_return(current_thread, 0);
				BUG_ON(sched_enqueue(current_thread));
				unlock(&notifc->notifc_lock);
				try_remove_timeout(target, notifc);

				BUG_ON(switch_to_thread(target));
				eret_to_thread(switch_context());

				/* Note that the control flow will not go through */
			}
		}

		target->thread_ctx->state = TS_INTER;
		BUG_ON(sched_enqueue(target));
	}

	unlock(&notifc->notifc_lock);
	try_remove_timeout(target, notifc);
	return 0;
}

s32 sys_create_notifc()
{
	struct notification *notifc = NULL;
	int notifc_cap = 0;
	int ret = 0;

	notifc = obj_alloc(TYPE_NOTIFICATION, sizeof(*notifc));
	if (!notifc) {
		ret = -ENOMEM;
		goto out_fail;
	}
	init_notific(notifc);

	notifc_cap = cap_alloc(current_cap_group, notifc, 0);
	if (notifc_cap < 0) {
		ret = notifc_cap;
		goto out_free_obj;
	}

	return notifc_cap;
out_free_obj:
	obj_free(notifc);
out_fail:
	return ret;
}


s32 sys_wait(u32 notifc_cap, bool is_block, struct timespec *timeout)
{
	struct notification *notifc = NULL;
	struct timespec timeout_k;
	int ret;

	notifc = obj_get(current_thread->cap_group, notifc_cap,
			 TYPE_NOTIFICATION);
	if (!notifc) {
		ret = -ECAPBILITY;
		goto out;
	}

	if (timeout) {
		ret = copy_from_user((char *)&timeout_k, (char *)timeout,
				     sizeof(timeout_k));
		if (ret != 0)
			goto out_obj_put;
	}

	ret = wait_notific(notifc, is_block, timeout ? &timeout_k : NULL);
out_obj_put:
	obj_put(notifc);
out:
	return ret;
}

s32 sys_notify(u32 notifc_cap)
{
	struct notification *notifc = NULL;
	int ret;
	notifc = obj_get(current_thread->cap_group, notifc_cap,
			 TYPE_NOTIFICATION);
	if (!notifc) {
		ret = -ECAPBILITY;
		goto out;
	}
	ret = signal_notific(notifc, 1 /* Enable direct switch */);
	obj_put(notifc);
out:
	return ret;
}
