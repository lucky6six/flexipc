#pragma once

#include <common/list.h>
#include <mm/vmspace.h>
#include <sched/sched.h>
#include <object/cap_group.h>
#include <arch/machine/smp.h>
#include <ipc/connection.h>
#include <irq/timer.h>

extern struct thread *current_threads[PLAT_CPU_NUM];
#define current_thread (current_threads[smp_get_cpu_id()])
#define DEFAULT_KERNEL_STACK_SZ		(0x1000)

#define THREAD_ITSELF	((void*)(-1))

struct thread {
	struct list_head	node;	                // link threads in a same cap_group
	struct list_head	ready_queue_node;	// link threads in a ready queue
	struct list_head	notification_queue_node; // link threads in a notification waiting queue
	struct thread_ctx	*thread_ctx;	        // thread control block

	/*
	 * prev_thread switch CPU to this_thread
	 *
	 * When previous thread is the thread itself,
	 * prev_thread will be set to THREAD_ITSELF.
	 */
	struct thread	        *prev_thread;

	/*
	 * vmspace: virtual memory address space.
	 * vmspace is also stored in the 2nd slot of capability
	 */
	struct vmspace    *vmspace;

	struct cap_group *cap_group;

	// struct ipc_connection *active_conn;
	// struct server_ipc_config *server_ipc_config;
	// struct lock register_lock;
	// struct ipc_connection *tmac_current_conn;

	/*
	 * Only exists for threads in a server process.
	 * If not NULL, it points to one of the three config types.
	 */
	void *general_ipc_config;
#if 0
	union general_ipc_config {
		/* If the thread declares an IPC service by invoking "register_server" */
		struct ipc_server_config server_config;
		/* If the thread is TYPE_SHADOW and is used as ipc_server_register_cb_thread */
		struct ipc_server_register_cb_config server_register_cb_config;
		/* If the thread is TYPE_SHADOW and is used as ipc_server_handler_thread */
		struct ipc_server_handler_config server_handler_config;
	};
#endif

	struct sleep_state sleep_state;
};

void create_root_thread(void);
void switch_thread_vmspace_to(struct thread *);
void thread_deinit(void *thread_ptr);
void handle_pending_exit();
