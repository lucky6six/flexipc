#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <chcore/ipc.h>
#include <chcore/defs.h>
#include <stdlib.h>
#include <stdio.h>
#include <chcore/bug.h>
#include <debug_lock.h>
#include <errno.h>
#include <assert.h>
#include "pthread_impl.h"
#include <chcore/lwip_defs.h>

/*
 * **fs_ipc_struct** is an address that points to the per-thread
 * system_ipc_fs in the pthread_t struct.
 */
ipc_struct_t *__fs_ipc_struct_location(void)
{
	return &__pthread_self()->system_ipc_fs;
}

/*
 * **lwip_ipc_struct** is an address that points to the per-thread
 * system_ipc_net in the pthread_t struct.
 */
ipc_struct_t *__net_ipc_struct_location(void)
{
	return &__pthread_self()->system_ipc_net;
}

static int connect_system_server(ipc_struct_t *ipc_struct);

/* Interfaces for operate the ipc message (begin here) */

/*
 * ipc_msg_t is constructed on the shm pointed by
 * ipc_struct_t->shared_buf.
 * A new ips_msg will override the old one.
 */
ipc_msg_t *ipc_create_msg(ipc_struct_t *icb, u64 data_len, u64 cap_slot_number)
{
	ipc_msg_t *ipc_msg;
	u64 buf_len;

	if (unlikely(icb->conn_cap == 0)) {
		/* Create the IPC connection on demand */
		if (connect_system_server(icb) != 0)
			return NULL;
	}

	/* Grab the ipc lock before setting ipc msg */
	chcore_spin_lock(&(icb->lock));

	/* The ips_msg metadata is at the beginning of the memory */
	buf_len = icb->shared_buf_len - sizeof(ipc_msg_t);

	/*
	 * Check the total length of data and caps.
	 *
	 * The checks at client side is not for security but for preventing
	 * unintended errors made by benign clients.
	 * The server has to validate the ipc msg by itself.
	 */
	if (((data_len + sizeof(u64) * cap_slot_number) > buf_len)
	    || ((data_len + sizeof(u64) * cap_slot_number) < data_len)) {
		printf("%s failed: too long msg (the usable shm size is 0x%lx)\n",
		       __func__, buf_len);
		return NULL;
	}

	ipc_msg = (ipc_msg_t *)icb->shared_buf;
	ipc_msg->icb = icb;

	ipc_msg->data_len = data_len;
	ipc_msg->cap_slot_number = cap_slot_number;
	ipc_msg->data_offset = sizeof(*ipc_msg);
	ipc_msg->cap_slots_offset = ipc_msg->data_offset + data_len;


	/*
	 * Zeroing is not that meaningful for shared memory.
	 * If necessary, the client can explict clear the shm by itself.
	 */
	return ipc_msg;
}

char *ipc_get_msg_data(ipc_msg_t *ipc_msg)
{
	return (char *)ipc_msg + ipc_msg->data_offset;
}

int ipc_set_msg_data(ipc_msg_t *ipc_msg, void *data, u64 offset, u64 len)
{
	if ((offset + len < offset) ||
	    (offset + len > ipc_msg->data_len)) {
		printf("%s failed due to overflow.\n", __func__);
		return -1;
	}

	memcpy(ipc_get_msg_data(ipc_msg) + offset, data, len);
	return 0;
}

/* Each cap takes 8 bytes although its length is 4 bytes in fact */
static u64 *ipc_get_msg_cap_ptr(ipc_msg_t *ipc_msg, u64 cap_id)
{
	return (u64 *)((char *)ipc_msg + ipc_msg->cap_slots_offset) + cap_id;
}

u64 ipc_get_msg_cap(ipc_msg_t *ipc_msg, u64 cap_slot_index)
{
	if (cap_slot_index >= ipc_msg->cap_slot_number) {
		printf("%s failed due to overflow.\n", __func__);
		return -1;
	}
	return *ipc_get_msg_cap_ptr(ipc_msg, cap_slot_index);
}

int ipc_set_msg_cap(ipc_msg_t *ipc_msg, u64 cap_slot_index, u32 cap)
{
	if (cap_slot_index >= ipc_msg->cap_slot_number) {
		printf("%s failed due to overflow.\n", __func__);
		return -1;
	}

	*ipc_get_msg_cap_ptr(ipc_msg, cap_slot_index) = cap;
	return 0;
}

/* XXX: for temporary use of return cap from server to client */
int ipc_set_ret_msg_cap(ipc_msg_t *ipc_msg, u32 cap)
{
	if (ipc_msg->cap_slot_number < 1)
		return -1;
	ipc_msg->cap_slot_number_ret = 1;
	*ipc_get_msg_cap_ptr(ipc_msg, 0) = cap;
	return 0;
}

int ipc_destroy_msg(ipc_msg_t *ipc_msg)
{
	/* Release the ipc lock */
	chcore_spin_unlock(&(ipc_msg->icb->lock));

	return 0;
}


/* Interfaces for operate the ipc message (end here) */

/* g_ipc_shm_idx: per-process ipc_shm index (like a shm slot id) */
static u64 g_ipc_shm_idx = 0;

static u64 get_free_vaddr_for_ipc_shm(void)
{
	int shm_id;

	shm_id = __sync_fetch_and_add(&g_ipc_shm_idx, 1);

	return IPC_SHM_BASE + shm_id * IPC_PER_SHM_SIZE;
}

/* A register_callback thread uses this to finish a registration */
static void ipc_register_cb_return(u64 server_thread_cap,
				   u64 server_shm_addr)
{
	usys_ipc_register_cb_return(server_thread_cap, server_shm_addr);
}

/* A register_callback thread is passive (never proactively run) */
void* register_cb(void *ipc_handler)
{
	int server_thread_cap = 0;
	u64 shm_addr;

	shm_addr = get_free_vaddr_for_ipc_shm();

	// printf("[server]: A new client comes in! ipc_handler: 0x%lx\n", ipc_handler);

	/*
	 * Create a passive thread for serving IPC requests.
	 * Besides, reusing an existing thread is also supported.
	 */
	pthread_t handler_tid;
	server_thread_cap = chcore_pthread_create_shadow(&handler_tid, NULL,
					ipc_handler, (void *)NO_ARG);
	BUG_ON(server_thread_cap < 0);
	/* set affinity */
	ipc_register_cb_return(server_thread_cap, shm_addr);

	return NULL;
}

/*
 * Currently, a server thread can only invoke this interface once.
 * But, a server can use another thread to register a new service.
 */
int ipc_register_server(server_handler server_handler,
			void* (*client_register_handler)(void*))
{
	int register_cb_thread_cap;
	int ret;

	/*
	 * Create a passive thread for handling IPC registration.
	 * - run after a client wants to register
	 * - be responsible for initializing the ipc connection
	 */
	#define ARG_SET_BY_KERNEL 0
	pthread_t handler_tid;
	register_cb_thread_cap = chcore_pthread_create_shadow(&handler_tid, NULL,
					client_register_handler, (void *)ARG_SET_BY_KERNEL);
	BUG_ON(register_cb_thread_cap < 0);
	/*
	 * Kernel will pass server_handler as the argument for the
	 * register_cb_thread.
	 */
	ret = usys_register_server((u64)server_handler,
				    (u64)register_cb_thread_cap);
	if (ret != 0) {
		printf("%s failed (retval is %d)\n", __func__, ret);
	}
	return ret;
}

struct client_shm_config {
	int shm_cap;
	u64 shm_addr;
};

/*
 * A client thread can register itself for multiple times.
 *
 * The returned ipc_struct_t is from heap,
 * so the callee needs to free it.
 */
ipc_struct_t *ipc_register_client(int server_thread_cap)
{
	int conn_cap;
	ipc_struct_t *client_ipc_struct;

	struct client_shm_config shm_config;
	int shm_cap;

	/*
	 * Before registering client on the server,
	 * the client allocates the shm (and shares it with
	 * the server later).
	 *
	 * Now we used PMO_DATA instead of PMO_SHM because:
	 * - SHM (IPC_PER_SHM_SIZE) only contains one page and
	 *   PMO_DATA is thus more efficient.
	 *
	 * If the SHM becomes larger, we can use PMO_SHM instead.
	 * Both types are tested and can work well.
	 */

	// shm_cap = usys_create_pmo(IPC_PER_SHM_SIZE, PMO_SHM);
	shm_cap = usys_create_pmo(IPC_PER_SHM_SIZE, PMO_DATA);
	if (shm_cap < 0) {
		printf("usys_create_pmo ret %d\n", shm_cap);
		usys_exit(-1);
	}

	shm_config.shm_cap = shm_cap;
	shm_config.shm_addr = get_free_vaddr_for_ipc_shm();

	//printf("%s: register_client with shm_addr 0x%lx\n",
	//      __func__, shm_config.shm_addr);

	while (1) {
		conn_cap = usys_register_client((u32)server_thread_cap,
						(u64)&shm_config);

		if (conn_cap == -EAGAIN) {
			// printf("client: Try to connect again ...\n");
			/* The server IPC may be not ready. */
			usys_yield();
		}
		else if (conn_cap < 0) {
			printf("client: %s failed (return %d), server_thread_cap is %d\n", __func__,
			       conn_cap, server_thread_cap);
			return NULL;
		}
		else {
			/* Success */
			break;
		}
	}

	client_ipc_struct = malloc(sizeof(ipc_struct_t));

	client_ipc_struct->lock = 0;
	client_ipc_struct->shared_buf = shm_config.shm_addr;
	client_ipc_struct->shared_buf_len = IPC_PER_SHM_SIZE;
	client_ipc_struct->conn_cap = conn_cap;

	return client_ipc_struct;
}

/* Client uses **ipc_call** to issue an IPC request */
s64 ipc_call(ipc_struct_t *icb, ipc_msg_t *ipc_msg)
{
	s64 ret;

	if (unlikely(icb->conn_cap == 0)) {
		/* Create the IPC connection on demand */
		if ((ret = connect_system_server(icb)) != 0)
			return ret;
	}

	do {
		ret = usys_ipc_call(icb->conn_cap, (u64)ipc_msg,
			    ipc_msg->cap_slot_number);
	} while (ret == -EAGAIN);

	return ret;
}

/* Server uses **ipc_return** to finish an IPC request */
void ipc_return(ipc_msg_t *ipc_msg, int ret)
{
	ipc_msg->cap_slot_number_ret = 0;
	usys_ipc_return((u64)ret, 0);
}

/*
 * IPC return and copy back capabilities.
 * XXX: Use different ipc return interface because cap_slot_number_ret
 * is valid only when we have cap to return. So we need to reset it to
 * 0 in ipc_return which has no cap to return.
 */
void ipc_return_with_cap(ipc_msg_t *ipc_msg, int ret)
{
	usys_ipc_return((u64)ret, ipc_msg->cap_slot_number_ret);
}

int simple_ipc_forward(ipc_struct_t *ipc_struct, void *data, int len)
{
	ipc_msg_t *ipc_msg;
	int ret;

	ipc_msg = ipc_create_msg(ipc_struct, len, 0);
	ipc_set_msg_data(ipc_msg, data, 0, len);
	ret = ipc_call(ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

static void ipc_struct_copy(ipc_struct_t *dst, ipc_struct_t *src)
{
	dst->conn_cap = src->conn_cap;
	dst->shared_buf = src->shared_buf;
	dst->shared_buf_len = src->shared_buf_len;
	dst->lock = src->lock;
}

/* ipc_struct to procmgr is not per-thread. So, we use the following two
 * variables to ensure an application will create at most one ipc_struct to procmgr.
 */
static int connect_procmgr_lock = 0;
static bool connected_procmgr = false;

static int connect_system_server(ipc_struct_t *ipc_struct)
{
	ipc_struct_t *tmp;

	switch (ipc_struct->server_id) {
	case FS_MANAGER: {
		tmp = ipc_register_client(fs_server_cap);
		if (tmp == NULL) {
			printf("%s: failed to connect FS\n", __func__);
			return -1;
		}
		break;
	}
	case NET_MANAGER: {
		tmp = ipc_register_client(lwip_server_cap);
		if (tmp == NULL) {
			printf("%s: failed to connect NET\n", __func__);
			return -1;
		}
		break;
	}
	case PROC_MANAGER: {
		chcore_spin_lock(&connect_procmgr_lock);
		if (connected_procmgr == false) {
			tmp = ipc_register_client(procmgr_server_cap);
			if (tmp == NULL) {
				printf("%s: failed to connect ProgMgr\n", __func__);
				chcore_spin_unlock(&connect_procmgr_lock);
				return -1;
			}
			ipc_struct_copy(ipc_struct, tmp);
			free(tmp);
			connected_procmgr = true;
		}
		chcore_spin_unlock(&connect_procmgr_lock);
		return 0;
	}
	default:
		printf("%s: unsupported system server id %d\n",
		       __func__, ipc_struct->server_id);
		return -1;
	}

	/* Copy the newly allocated ipc_struct to the per_thread ipc_struct */
	ipc_struct_copy(ipc_struct, tmp);
	free(tmp);

	return 0;
}
