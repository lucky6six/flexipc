#include <object/cap_group.h>
#include <object/thread.h>
#include <common/list.h>
#include <common/util.h>
#include <common/bitops.h>
#include <mm/kmalloc.h>
#include <mm/vmspace.h>
#include <mm/uaccess.h>
#include <lib/printk.h>
#include <ipc/notification.h>
#include <irq/irq.h>

extern int trans_uva_to_kva(u64 user_va, u64 *kernel_va);

struct recycle_msg {
	u64 badge;
	int exitcode;
	int padding;
};

struct recycle_msg_node {
	struct list_head node;
	struct recycle_msg msg;
};

struct notification *recycle_notification = NULL;
vaddr_t recycle_msg_buffer = 0;
/* This list is only used when the recycle_msg_buffer is full */
struct list_head recycle_msg_head;
struct lock recycle_buffer_lock;

/*
 * msg_buffer layout (ring buffer)
 * 0-8: consumer_offset (the user-level recycle thread)
 * 9-16: producer_offset (kernel, multiple threads)
 *
 * size: 0x1000 - 16
 */

#define START_OFFSET 16
#define END_OFFSET   4096
/* The node field in a recycle_msg is only used for linking msgs */
#define MSG_SIZE     (sizeof(struct recycle_msg))

static inline vaddr_t get_consumer_offset(void)
{
	return *(vaddr_t *)(recycle_msg_buffer);
}

static inline void set_consumer_offset(vaddr_t val)
{
	*(vaddr_t *)(recycle_msg_buffer) = val;
}

static inline vaddr_t get_producer_offset(void)
{
	return *(vaddr_t *)(recycle_msg_buffer + 8);
}

static inline void set_producer_offset(vaddr_t val)
{
	*(vaddr_t *)(recycle_msg_buffer + 8) = val;
}

static inline void set_msg(vaddr_t off, u64 badge, int exitcode)
{
	struct recycle_msg *msg;

	msg = (struct recycle_msg *)(recycle_msg_buffer + off);
	msg->badge = badge;
	msg->exitcode = exitcode;
}

/* Get a free msg slot in the ring buffer */
static inline vaddr_t get_free_slot_in_msg_buffer(void)
{
	vaddr_t p_off = get_producer_offset();
	vaddr_t c_off = get_consumer_offset();

	/* Case-1 of existing free slots */
	if ((p_off >= c_off) && (p_off < END_OFFSET)) {
		set_producer_offset(p_off + MSG_SIZE);
		return p_off;
	}

	/* Case-2 of existing free slots */
	if ((p_off == END_OFFSET) && (c_off > START_OFFSET)) {
		set_producer_offset(START_OFFSET + MSG_SIZE);
		return START_OFFSET;
	}

	/* Case-3 of existing free slots (can be merged with case-1) */
	if (p_off < c_off) {
		set_producer_offset(p_off + MSG_SIZE);
		return p_off;
	}

	/* No free slot found */
	return 0;
}

int sys_register_recycle(int notifc_cap, vaddr_t msg_buffer)
{
	int ret;

	if (current_cap_group->badge != PROCMGR_BADGE) {
		kinfo("A process (not the procmgr) tries to register recycle.\n");
		return -EPERM;
	}

	recycle_notification = obj_get(current_cap_group, notifc_cap,
				      TYPE_NOTIFICATION);

	BUG_ON(recycle_notification == NULL);

	ret = trans_uva_to_kva(msg_buffer, &recycle_msg_buffer);
	BUG_ON(ret != 0);

	init_list_head(&recycle_msg_head);
	lock_init(&recycle_buffer_lock);

	return 0;
}

/*
 * Kernel uses this function to invoke the recycle thread in procmgr.
 * proc_badge is the badge of the process to recycle.
 */
void notify_user_recycler(u64 proc_badge, int exitcode)
{
	vaddr_t buffer_off;
	/* lock the recyle buffer first */
	lock(&recycle_buffer_lock);

	buffer_off = get_free_slot_in_msg_buffer();
	if (buffer_off == 0) {
		/* Save the msg in the list for now */
		struct recycle_msg_node *msg;

		msg = kmalloc(sizeof(*msg));
		msg->msg.badge = proc_badge;
		msg->msg.exitcode = exitcode;
		list_add(&msg->node, &recycle_msg_head);
	} else {
		set_msg(buffer_off, proc_badge, exitcode);

		/* Put the msg saved in list into the buffer */
		vaddr_t off;
		struct recycle_msg_node *msg;

		for_each_in_list(msg, struct recycle_msg_node, node,
				 &recycle_msg_head) {
			off = get_free_slot_in_msg_buffer();
			if (off != 0) {
				list_del(&msg->node);
				set_msg(off, msg->msg.badge,
					msg->msg.exitcode);
				kfree(msg);
			} else {
				break;
			}
		}
	}

	unlock(&recycle_buffer_lock);

	/* Nofity the recycle thread through the notification*/
	if (buffer_off != 0)
		signal_notific(recycle_notification,
			       0 /* disable direct switch */);
}

void sys_exit_group(int exitcode)
{
	// TODO @ln
	current_thread->thread_ctx->state = TS_EXIT;

	/*
	 * Check if the notification has been sent.
	 * E.g., a faulting process may trigger sys_exit_group for many times.
	 */
	if (__sync_val_compare_and_swap(&current_cap_group->notify_recycler,
					0, 1) == 1) {
		notify_user_recycler(current_cap_group->badge, exitcode);
	}

	current_thread = NULL;
	sched();
	eret_to_thread(switch_context());
}

/*
 * FIXME:
 *
 * 1. Suppose cap_group is exiting, what if a thread in another
 *    cap_group transfer cap to it.
 *
 * 2. Assume there are two threads of a cap_group.
 *    One thread invokes cap_group_exit.
 *    Another thread works in the kernel (may kmalloc something, e.g., IPC).
 *    How to stop the in-kernel thread? Directly stopping it with IPI may
 *    cause memory leakage.
 */


/*
 * If a thread invoke this to recycle the resources, the kernel will run
 * on the thread's kernel stack, which makes things complex.
 *
 * So, only the (user-level) process manager can invoke cap_group_exit on some cap_group.
 *
 * Case-1: a thread invokes exit, it will directly tell the process manager,
 *         and then, the process manager invokes this function.
 * Case-2: if a thread triggers faults (e.g., segfault), the kernel will notify
 *         the process manager to exit the corresponding process (cap_group).
 */
int sys_cap_group_recycle(int cap_group_cap)
{
	struct cap_group *cap_group;

	cap_group = obj_get(current_cap_group, cap_group_cap, TYPE_CAP_GROUP);
	if (!cap_group) {
		BUG_ON("Process Manager gives a invalid cap_group_cap.\n");
	}

#if 0

	struct thread *thread;
	struct slot_table *slot_table;
	int slot_id;


	// TODO: what if another thread is creating a thread
	// use a rwlock (create thread requires rlock, here uses wlock)

	/* Phase-1: Stop all the threads in this cap_group */
	// TODO: is for_each_in_list_safe necessary?
	for_each_in_list(thread, struct thread, node,
			 &(cap_group->thread_list)) {

		// case-1: TS_RUNNING - send resched IPI to
		// the running CPU

		// case-2: TS_RUNNABLE (in run queue?)
		// what if the scheduler choose it to run

		// case-3: TS_XXX
		//
		// set TS_EXITING: cmp_and_swap (TS_XXX, TS_EXITING)

		// sched_dequeue()
	}



	// TODO: revoke cap_group in others? when will the cap_group_obj_id be
	// taken by others? create other processeess?
	// held by the father process? sigchild?
	/* cap_revoke(cap_group, CAP_GROUP_OBJ_ID); */




	/*
	 * Phase-2:
	 * Iterate all the capability table and free the corresponding resources.
	 */
	slot_table = &cap_group->slot_table;
	write_lock(&slot_table->table_guard);

	for_each_set_bit(slot_id, slot_table->slots_bmp,
			 slot_table->slots_size) {
		__cap_free(cap_group, slot_id, true, false);
	}

	write_unlock(&slot_table->table_guard);


	obj_put(cap_group);

	/* TODO: Free the passed cap_group_cap from process manager */

#endif

	return 0;
}

