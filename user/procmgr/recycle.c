#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <chcore/idman.h>
#include <chcore/ipc.h>
#include <chcore/container/list.h>
#include <chcore/launcher.h>
#include <chcore/proc.h>
#include <chcore/procmgr_defs.h>
#include <chcore/syscall.h>
#include <chcore/container/radix.h>
#include <pthread.h>

#include "procmgr.h"

/* A recycle_msg is set by the kernel when one process needs to
 * be recycled.
 */
struct recycle_msg {
	u64 badge;
	int exitcode;
	int padding;
};

/*
 * msg_buffer layout (ring buffer)
 * 0-8: consumer_offset (the user-level recycle thread)
 * 9-16: producer_offset (kernel, multiple threads)
 *
 * size: 0x1000 - 16
 */
vaddr_t recycle_msg_buffer;

#define START_OFFSET 16
#define END_OFFSET   4096
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

static inline struct recycle_msg *retrieve_one_msg(void)
{
	vaddr_t p_off = get_producer_offset();
	vaddr_t c_off = get_consumer_offset();

	if (p_off != c_off) {
		return (struct recycle_msg *)(c_off + recycle_msg_buffer);
	}

	return NULL;
}

static inline void increase_consumer_offset(struct recycle_msg *msg)
{
	u64 c_off;

	c_off = (u64)msg - recycle_msg_buffer;

	if (c_off < END_OFFSET)
		set_consumer_offset(c_off + MSG_SIZE);
	else {
		assert(c_off == END_OFFSET);
		c_off = START_OFFSET;
	}
}

int usys_cap_group_recycle(int);

void *recycle_routine(void *arg)
{
	int notific_cap;
	u64 msg_buffer;
	int ret;

	struct recycle_msg *msg;
	struct proc_node *proc_to_recycle;

	/* Recycle_thread will wait on this notification */
	notific_cap = usys_create_notifc();

	/* The msg on recycling which process is in msg_buffer */
	msg_buffer = (u64)mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
			  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	assert(msg_buffer != 0);
	recycle_msg_buffer = msg_buffer;
	memset((void *)msg_buffer, 0x1000, 0);

	set_consumer_offset(START_OFFSET);
	set_producer_offset(START_OFFSET);

	/* TODO: This system can only be invoked by Procmgr */
	ret = usys_register_recycle_thread(notific_cap, msg_buffer);
	assert(ret == 0);

	while (1) {
		usys_wait(notific_cap, 1 /* Block */, NULL /* No timeout */);

		while ((msg = retrieve_one_msg()) != NULL) {
			proc_to_recycle = procmgr_find_client_proc(msg->badge);
			assert(proc_to_recycle != 0);

			info("[recycle thread]: recycle process (pid: %d)\n",
			     proc_to_recycle->pid);

			proc_to_recycle->exitstatus = msg->exitcode;
			proc_to_recycle->state = PROC_STATE_EXIT;

			do {
				ret = usys_cap_group_recycle
					(proc_to_recycle->proc_cap);
			} while (ret == -EAGAIN);

			increase_consumer_offset(msg);
		}
	}
}
