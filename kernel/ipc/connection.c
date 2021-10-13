/*
 * Inter-**Process** Communication.
 *
 * connection: between a client cap_group and a server cap_group (two processes)
 * We store connection cap in a process' cap_group, so each thread in it can
 * use that connection.
 *
 * A connection (cap) can be used by any thread in the client cap_group.
 * A connection will be **only** served by one server thread while
 * one server thread may serve multiple connections.
 *
 * There is one PMO_SHM binding with one connection.
 *
 * Connection can only serve one IPC request at the same time.
 * Both user and kernel should check the "busy_state" of a connection.
 * Besides, register thread can also serve one registration request for one
 * time.
 *
 * Since a connection can only be shared by client threads in the same process,
 * a connection has only-one **badge** to identify the process.
 * During ipc_call, the kernel can set **badge** as an argument in register.
 *
 * Overview:
 * **IPC registration (control path)**
 *  - A server thread (S1) invokes **sys_register_server** with a register_cb_thread (S2)
 *
 *  - A client thread (C) invokes **sys_register_client(S1)**
 *	- invokes (switches) to S2 actually
 *  - S2 invokes **sys_ipc_register_cb_return** with a handler_thread (S3)
 *	- S3 will serve IPC requests later
 *	- switches back to C (finish the IPC registration)
 *
 * **IPC call/reply (data path)**
 *  - C invokes **sys_ipc_call**
 *	- switches to S3
 *  - S3 invokes **sys_ipc_return**
 *	- switches to C
 */

#include <ipc/connection.h>
#include <mm/kmalloc.h>
#include <mm/uaccess.h>
#include <sched/context.h>
#include <irq/irq.h>

/*
 * An extern declaration.
 * Here it is used for mapping ipc_shm.
 */
int map_pmo_in_current_cap_group(u64 pmo_cap, u64 addr, u64 perm);

/*
 * Overall, a server thread that declares a serivce with this interface
 * should specify @ipc_routine (the real ipc service routine entry) and
 * @register_thread_cap (another server thread for handling client
 * registration).
 */
static int register_server(struct thread *server, u64 ipc_routine,
			    u64 register_thread_cap)
{
	struct ipc_server_config *config;
	struct thread *register_cb_thread;
	struct ipc_server_register_cb_config *register_cb_config;

	BUG_ON(server == NULL);
	if (server->general_ipc_config != NULL)
		BUG("A server thread can only invoke **register_server** once!\n");

	/*
	 * TODO (tmac):
	 * - directly using kmalloc is not that **microkernel/cap**
	 * - free this memory when destoring the thread ...
	 * - check kmalloc return value
	 */
	config = kmalloc(sizeof(*config));

	/*
	 * @ipc_routine will be the real ipc_routine_entry.
	 * No need to validate such address because the server just
	 * kill itself if the address is illegal.
	 */
	config->declared_ipc_routine_entry = ipc_routine;

	/*
	 * Record the passive thread in server for handling
	 * client registration.
	 */
	register_cb_thread = obj_get(current_cap_group,
				     register_thread_cap, TYPE_THREAD);
	BUG_ON(!register_cb_thread);
	config->register_cb_thread = register_cb_thread;

	/*
	 * TODO (tmac): same as before.
	 * - free this memory when destoring the thread ...
	 * - check kmalloc return value
	 */
	register_cb_config = kmalloc(sizeof(*register_cb_config));
	register_cb_thread->general_ipc_config = register_cb_config;

	/*
	 * This lock will be used to prevent concurrent client threads
	 * from registering.
	 * In other words, a register_cb_thread can only serve
	 * registration requests one-by-one.
	 */
	lock_init(&register_cb_config->register_lock);


	/*
	 * Record it (PC) as well as the thread's initial stack (SP).
	 */
	register_cb_config->register_cb_entry =
		arch_get_thread_next_ip(register_cb_thread);
	register_cb_config->register_cb_stack =
		arch_get_thread_stack(register_cb_thread);
	obj_put(register_cb_thread);

	/*
	 * The last step: fill the general_ipc_config.
	 * This field is also treated as the whether the server thread
	 * declares an IPC service (or makes the service ready).
	 */
	server->general_ipc_config = config;

	return 0;
}

/* Just used for storing the results of function client_connection */
struct client_connection_result {
	int client_conn_cap;
	int server_conn_cap;
	int server_shm_cap;
	struct ipc_connection *conn;
};

/*
 * The function will create an IPC connection and initialize the client side
 * information. (used in sys_register_client)
 *
 * The server (register_cb_thread) will initialize the server side information
 * later (in sys_ipc_register_cb_return).
 */
static int client_connection(struct thread *client, struct thread *server,
			     int shm_cap_client, u64 shm_addr_client,
			     struct client_connection_result *res)
{
	struct pmobject *pmo;
	int shm_cap_server;
	struct ipc_connection *conn;
	int ret = 0;
	int conn_cap = 0, server_conn_cap = 0;

	BUG_ON((client == NULL) || (server == NULL));

	pmo = obj_get(current_cap_group, shm_cap_client, TYPE_PMO);
	BUG_ON(!pmo);

	/*
	 * Copy the shm_cap to the server.
	 *
	 * It is reasonable to count the shared memory usage on the client.
	 * So, a client should prepare the shm and tell the server.
	 */
	shm_cap_server = cap_copy(current_cap_group, server->cap_group,
				  shm_cap_client, 0, 0);


	/* Create struct ipc_connection */
	conn = obj_alloc(TYPE_CONNECTION, sizeof(*conn));
	if (!conn) {
		ret = -ENOMEM;
		goto out_fail;
	}

	/* Give the ipc_connection (cap) to the client */
	conn_cap = cap_alloc(current_cap_group, conn, 0);
	if (conn_cap < 0) {
		ret = conn_cap;
		goto out_free_obj;
	}

	/* Give the ipc_connection (cap) to the server */
	server_conn_cap = cap_copy(current_cap_group, server->cap_group,
				   conn_cap, 0, 0);
	if (server_conn_cap < 0) {
		ret = server_conn_cap;
		goto out_free_obj;
	}


	/* Initialize the connection (begin) */

	/*
	 * Note that now client is applying to build the connection
	 * instead of issuing an IPC.
	 */
	conn->current_client_thread = client;
	/*
	 * The register_cb_thread in server will assign the
	 * server_handler_thread later.
	 */
	conn->server_handler_thread = NULL;
	/*
	 * The badge is now generated by the process who creates the client
	 * thread. Usually, the process is the procmgr user-space service.
	 * The badge needs to be unique.
	 *
	 * Before a process exits, it needs to close the connection with
	 * servers. Otherwise, a later process may pretend to be it
	 * because the badge is based on PID (if a PID is reused,
	 * the same badge occur).
	 * Or, the kernel should notify the server to close the
	 * connections when some client exits.
	 */
	conn->client_badge = current_cap_group->badge;
	conn->shm.client_shm_uaddr = shm_addr_client;
	conn->shm.shm_size = pmo->size;
	/* Initialize the connection (end) */

	/* Preapre the return results */
	res->client_conn_cap = conn_cap;
	res->server_conn_cap = server_conn_cap;
	res->server_shm_cap = shm_cap_server;
	res->conn = conn;

	return 0;

out_free_obj:
	kinfo("%s fails out_free_obj\n", __func__);
	obj_free(conn);
out_fail:
	kinfo("%s fails out_fail\n", __func__);
	obj_put(pmo);
	return ret;
}

/*
 * Grap the ipc lock before doing any modifications including
 * modifing the conn or sending the caps.
 */
static inline int grab_ipc_lock(struct ipc_connection *conn)
{
	struct thread *target;
	struct ipc_server_handler_config *handler_config;

	target = conn->server_handler_thread;
	handler_config = (struct ipc_server_handler_config *)
		target->general_ipc_config;

	/*
	 * Grabing the ipc_lock can ensure:
	 * First, avoid invoking the same handler thread.
	 * Second, also avoid using the same connection.
	 *
	 * perf in Qemu: lock & unlock (without contention) just takes
	 * about 20 cycles on x86_64.
	 */

	/* Use try-lock, otherwise deadlock may happen
	 * deadlock: T1: ipc-call -> Server -> resched to T2: ipc-call
	 *
	 * Although lock is added in user-ipc-lib, a buggy app may dos
	 * the kernel.
	 */

	if (try_lock(&handler_config->ipc_lock) != 0)
		return -EAGAIN;

	/*
	 * If the target (server) thread may be still executing after
	 * unlocking (a very small time window),
	 * we need to wait until its stack is free.
	 */
	while (target->thread_ctx->kernel_stack_state != KS_FREE) ;

	return 0;
}

/* During an IPC: directly transfer the control flow from client to server */
static void thread_migrate_to_server(struct ipc_connection *conn, u64 arg)
{
	struct thread *target;
	struct ipc_server_handler_config *handler_config;

	target = conn->server_handler_thread;
	handler_config = (struct ipc_server_handler_config *)
		target->general_ipc_config;

	/*
	 * Note that a server ipc handler thread can be assigned to multiple
	 * connections.
	 * So, it is necessary to record which connection is active.
	 */
	handler_config->active_conn = conn;

	/*
	 * Note that multiple client threads may share a same connection.
	 * So, it is necessary to record which client thread is active.
	 * Then, the server can transfer the control back to it after finishing
	 * the IPC.
	 */
	conn->current_client_thread = current_thread;

	/* Mark current_thread as TS_WAITING */
	current_thread->thread_ctx->state = TS_WAITING;
	/* Pass the scheduling context */
	target->thread_ctx->sc = current_thread->thread_ctx->sc;

	/* Set target thread SP/IP/arg */
#ifdef ARCH_X86_64
	/* Remove the simple function calls can save about 20 cycles */
	arch_exec_cont_t *ec;
	ec = &(target->thread_ctx->ec);
	ec->reg[RSP] = handler_config->ipc_routine_stack;
	ec->reg[RIP] = handler_config->ipc_routine_entry;
	ec->reg[RDI] = arg;
	ec->reg[RSI] = conn->client_badge;
#else
	arch_set_thread_stack(target, handler_config->ipc_routine_stack);
	arch_set_thread_next_ip(target, handler_config->ipc_routine_entry);
	/* First argument: ipc_msg */
	arch_set_thread_arg(target, arg);
	/* First argument: client_badge */
	arch_set_thread_badge_arg(target, conn->client_badge);
#endif

	/*
	 * TODO (tmac): is obj_put really necessary?
	 * On one side, any other method to avoid data corruption.
	 * On the other side, kernel directly refer to some objects
	 * somewhere like **sys_ipc_return**.
	 */
	obj_put(conn);

	/* Switch to the target thread */
	switch_to_thread(target);
	eret_to_thread(switch_context());

	/* Function never return */
	BUG_ON(1);
}

/* During an IPC: directly transfer the control flow from server back to client */
static void thread_migrate_to_client(struct thread *client, u64 ret_value)
{
	/* Set return value for the target thread */
	arch_set_thread_return(client, ret_value);

	/* Switch to the target thread */
	switch_to_thread(client);
	eret_to_thread(switch_context());

	/* Function never return */
	BUG_ON(1);
}

struct client_shm_config {
	int shm_cap;
	u64 shm_addr;
};


/* IPC related system calls */

u64 sys_register_server(u64 ipc_routine, u64 register_thread_cap)
{
	return register_server(current_thread, ipc_routine,
			       register_thread_cap);
}

u32 sys_register_client(u32 server_cap, u64 shm_config_ptr)
{
	struct thread *client;
	struct thread *server;

	/*
	 * No need to initialize actually.
	 * However, fbinfer will complain without zeroing because
	 * it cannot tell copy_from_user.
	 */
	struct client_shm_config shm_config = {0};
	int r;
	struct client_connection_result res;

	struct ipc_server_config *server_config;
	struct thread *register_cb_thread;
	struct ipc_server_register_cb_config *register_cb_config;

	client = current_thread;

	server = obj_get(current_cap_group, server_cap, TYPE_THREAD);
	if (!server) {
		r = -ECAPBILITY;
		goto out_fail;
	}

	server_config = (struct ipc_server_config *)
		(server->general_ipc_config);
	if (!server_config) {
		r = -EAGAIN;
		goto out_fail;
	}

	/*
	 * Locate the register_cb_thread first.
	 * And later, directly transfer the control flow to it
	 * for finishing the registration.
	 *
	 * The whole registration procedure:
	 * client thread -> server register_cb_thread -> client threrad
	 */
	register_cb_thread = server_config->register_cb_thread;
	register_cb_config = (struct ipc_server_register_cb_config *)
		(register_cb_thread->general_ipc_config);

	/* Acquiring register_lock: avoid concurrent client registration.
	 *
	 * Use try_lock instead of lock since the unlock operation is done by
	 * another thread and ChCore does not support mutex.
	 * Otherwise, dead lock may happen.
	 */
	if (try_lock(&register_cb_config->register_lock) != 0) {
		r = -EAGAIN;
		goto out_fail;
	}

	/* As before: in case of a small time window */
	while (register_cb_thread->thread_ctx->kernel_stack_state != KS_FREE) ;


	/* copy_from_user/copy_to_user will validate the user addresses */
	r = copy_from_user((char *)&shm_config, (char *)shm_config_ptr,
			   sizeof(shm_config));
	BUG_ON(r != 0);

	/* Map the pmo of the shared memory */
	r = map_pmo_in_current_cap_group(shm_config.shm_cap,
		    shm_config.shm_addr, VMR_READ | VMR_WRITE);
	BUG_ON(r != 0);
	kdebug("client process: %p, map shm at 0x%lx\n", current_cap_group,
	       shm_config.shm_addr);

	/* Create the ipc_connection object */
	r = client_connection(client, server, shm_config.shm_cap,
			      shm_config.shm_addr, &res);
	if (r != 0) {
		kinfo("%s failed (%d)\n", __func__, r);
		BUG_ON(1);
		goto out_fail;
	}

	/* Record the connection cap of the client process */
	register_cb_config->conn_cap_in_client = res.client_conn_cap;
	register_cb_config->conn_cap_in_server = res.server_conn_cap;
	/* Record the server_shm_cap for current connection */
	register_cb_config->shm_cap_in_server = res.server_shm_cap;


	/* Mark current_thread as TS_WAITING */
	current_thread->thread_ctx->state = TS_WAITING;

	/* Set target thread SP/IP/arg */
	arch_set_thread_arg(register_cb_thread,
			    server_config->declared_ipc_routine_entry);
	arch_set_thread_stack(register_cb_thread,
			    register_cb_config->register_cb_stack);
	arch_set_thread_next_ip(register_cb_thread,
			    register_cb_config->register_cb_entry);
	obj_put(server);

	/* Pass the scheduling context */
	register_cb_thread->thread_ctx->sc =
		current_thread->thread_ctx->sc;

	kdebug("In %s: go to regsiter_cb_thread\n", __func__);
	/* On success: switch to the cb_thread of server  */
	switch_to_thread(register_cb_thread);
	eret_to_thread(switch_context());

	/* Never return */
	BUG_ON(1);

out_fail:
	/* Maybe EAGAIN */
	kdebug("%s failed\n", __func__);

	obj_put(server);
	return r;
}

// TODO (tmac): why 8?
#define MAX_CAP_TRANSFER	8
static int ipc_send_cap(struct ipc_connection *conn, ipc_msg_t *ipc_msg,
			u64 cap_num)
{
	int i, r;
	u64 cap_slots_offset;
	u64 *cap_buf;

	if (likely(cap_num == 0)) {
		r = 0;
		goto out;
	} else if (cap_num >= MAX_CAP_TRANSFER) {
		r = -EINVAL;
		goto out;
	}

	r = copy_from_user((char *)&cap_slots_offset,
			   (char *)&ipc_msg->cap_slots_offset,
			   sizeof(cap_slots_offset));
	if (r < 0)
		goto out;

	cap_buf = kmalloc(cap_num * sizeof(*cap_buf));
	if (!cap_buf) {
		r = -ENOMEM;
		goto out;
	}

	r = copy_from_user((char *)cap_buf,
			   (char *)ipc_msg + cap_slots_offset,
			   sizeof(*cap_buf) * cap_num);
	if (r < 0)
		goto out;

	for (i = 0; i < cap_num; i++) {
		u64 dest_cap;

		kdebug("[IPC] send cap:%d\n", cap_buf[i]);
		dest_cap = cap_copy(current_cap_group,
				    conn->server_handler_thread->cap_group,
				    cap_buf[i], false, 0);
		if (dest_cap < 0)
			goto out_free_cap;
		cap_buf[i] = dest_cap;
	}
	/* FIXME: Copied cap id of the dest cap_group is readable from src cap_group. */
	r = copy_to_user((char *)ipc_msg + cap_slots_offset,
			 (char *)cap_buf, sizeof(*cap_buf) * cap_num);
	if (r < 0)
		goto out_free_cap;
	kfree(cap_buf);
	return 0;
out_free_cap:
	for (--i; i >= 0; i--)
		cap_free(conn->server_handler_thread->cap_group, cap_buf[i]);
	kfree(cap_buf);
out:
	return r;
}

/* FIXME: for temporary use of return cap from server to client */
static void ipc_send_cap_to_client(struct ipc_connection *conn, u64 cap_num)
{
	int r, ret_cap;
	struct ipc_msg *server_ipc_msg;

	server_ipc_msg = (struct ipc_msg *)((u64)conn->user_ipc_msg
					    - conn->shm.client_shm_uaddr
					    + conn->shm.server_shm_uaddr);
	if (cap_num == 1) {
		r = copy_from_user((char *)&ret_cap,
				   (char *)server_ipc_msg
				   + server_ipc_msg->cap_slots_offset,
				   sizeof(ret_cap));
		BUG_ON(r < 0);

		ret_cap = cap_copy(current_cap_group,
				   conn->current_client_thread->cap_group,
				   ret_cap, false, 0);
		BUG_ON(ret_cap < 0);
		r = copy_to_user((char *)server_ipc_msg
				   + server_ipc_msg->cap_slots_offset,
				   (char *)&ret_cap,
				   sizeof(ret_cap));
		BUG_ON(r < 0);
	}

}

/* Issue an IPC request */
u64 sys_ipc_call(u32 conn_cap, ipc_msg_t *ipc_msg_in_client, u64 cap_num)
{
	struct ipc_connection *conn;
	u64 ipc_msg_in_server;
	int r = 0;

	/*
	 * Perf on Qemu: obj_get & obj_put takes about 160 cycles
	 * on x86_64.
	 *
	 * Use the following instead:
	 * conn = (struct ipc_connection *)(current_cap_group->slot_table.slots[conn_cap]->object->opaque);
	 */
	conn = obj_get(current_cap_group, conn_cap, TYPE_CONNECTION);

	BUG_ON(!conn);

	/* try_lock may fail and returns egain.
	 * No modifications happen before locking, so the client
	 * can simply try again later.
	 */
	if ((r = grab_ipc_lock(conn)) != 0)
		goto out_obj_put;

	/* Perf: Fast Path */
	if (ipc_msg_in_client == NULL) {
		thread_migrate_to_server(conn, 0);
		BUG("should not reach here\n");
	}

	/*
	 * **ipc_send_cap** only reads server_handler_thread in conn,
	 * so it is not necessary to grab the lock here.
	 */
	if (unlikely(cap_num != 0)) {
		r = ipc_send_cap(conn, ipc_msg_in_client, cap_num);
		if (r < 0)
			goto out_obj_put;
	}

	conn->user_ipc_msg = ipc_msg_in_client;

	/*
	 * A shm is bound to one connection.
	 * But, the client and server can map the shm at different addresses.
	 * So, we re-calculate the ipc_msg (in the shm) address here.
	 */
	ipc_msg_in_server = (u64)ipc_msg_in_client
		- conn->shm.client_shm_uaddr + conn->shm.server_shm_uaddr;
	/* Call server (handler thread) */
	thread_migrate_to_server(conn, ipc_msg_in_server);

	BUG("should not reach here\n");
out_obj_put:
	obj_put(conn);
	return r;
}

void sys_ipc_return(u64 ret, u64 cap_num)
{
	struct ipc_server_handler_config *handler_config;
	struct ipc_connection *conn;
	struct thread *client;

	/* Get the currently active connection */
	handler_config = (struct ipc_server_handler_config *)
		current_thread->general_ipc_config;
	conn = handler_config->active_conn;
	BUG_ON(conn == NULL);
	/* Set active_conn to NULL since the IPC will finish sooner */
	handler_config->active_conn = NULL;

	/*
	 * Get the client thread that issues this IPC.
	 *
	 * Note that it is **unnecessary** to set the field to NULL
	 * i.e., conn->current_client_thread = NULL.
	 */
	client = conn->current_client_thread;

	if (unlikely(cap_num != 0))
		ipc_send_cap_to_client(conn, cap_num);

	/*
	 * Release the ipc_lock to mark the server_handler_thread can
	 * serve other requests now.
	 */
	unlock(&handler_config->ipc_lock);

	/* Return to client */
	thread_migrate_to_client(client, ret);
	BUG("should not reach here\n");
}

void sys_ipc_register_cb_return(u64 server_handler_thread_cap,
				u64 server_shm_addr)
{
	struct ipc_server_register_cb_config *config;
	struct ipc_connection *conn;
	struct thread *client_thread;

	struct thread *ipc_server_handler_thread;
	struct ipc_server_handler_config *handler_config;
	int r;

	config = (struct ipc_server_register_cb_config *)
		current_thread->general_ipc_config;

	/* Get the connection currently building */
	conn = obj_get(current_cap_group, config->conn_cap_in_server,
		       TYPE_CONNECTION);
	if (!conn) {
		kinfo("conn_cap_in_server is %d\n",
		      config->conn_cap_in_server);
	}

	BUG_ON(!conn);

	/* Get the client_thread that issues this registration */
	client_thread = conn->current_client_thread;
	/*
	 * Set the return value (conn_cap) for the client here
	 * because the server has approved the registration.
	 */
	arch_set_thread_return(client_thread, config->conn_cap_in_client);

	/*
	 * @server_handler_thread_cap from server.
	 * Server uses this handler_thread to serve ipc requests.
	 */
	ipc_server_handler_thread =
		(struct thread *)obj_get(current_cap_group,
					 server_handler_thread_cap,
					 TYPE_THREAD);
	if (!ipc_server_handler_thread) {
		kinfo("server_handler_thread_cap is %d\n",
		      server_handler_thread_cap);
	}
	BUG_ON(!ipc_server_handler_thread);

	/* Initialize the ipc configuration for the handler_thread (begin) */
	handler_config = (struct ipc_server_handler_config *)
		kmalloc(sizeof(*handler_config));
	ipc_server_handler_thread->general_ipc_config = handler_config;
	lock_init(&handler_config->ipc_lock);


	/*
	 * Record the initial PC & SP for the handler_thread.
	 * For serving each IPC, the handler_thread starts from the
	 * same PC and SP.
	 */
	handler_config->ipc_routine_entry =
		arch_get_thread_next_ip(ipc_server_handler_thread);
	handler_config->ipc_routine_stack =
		arch_get_thread_stack(ipc_server_handler_thread);
	obj_put(ipc_server_handler_thread);
	/* Initialize the ipc configuration for the handler_thread (end) */


	/* Map the shm of the connection in server */
	r = map_pmo_in_current_cap_group(config->shm_cap_in_server,
		    server_shm_addr, VMR_READ | VMR_WRITE);
	BUG_ON(r != 0);
	kdebug("server process: %p, map shm at 0x%lx\n", current_cap_group,
	      server_shm_addr);

	/* Fill the server information in the IPC connection. */
	conn->shm.server_shm_uaddr = server_shm_addr;
	conn->server_handler_thread = ipc_server_handler_thread;
	conn->current_client_thread = NULL;
	obj_put(conn);

	/* Finish the registration: switch to the original client_thread */
	unlock(&config->register_lock);
	switch_to_thread(client_thread);
	eret_to_thread(switch_context());
}

/* Send cap through IPC */

static int transfer_cap(struct ipc_connection *conn, u32 send_cap)
{
	int cap;
	struct thread *target;

	target = conn->server_handler_thread;
	if ((cap = cap_copy(current_cap_group, target->cap_group,
			    send_cap, false, 0)) < 0)
		return cap;

	thread_migrate_to_server(conn, cap);
	return 0;
}

u64 sys_ipc_send_cap(u32 conn_cap, u32 send_cap)
{
	struct ipc_connection *conn;
	int r;

	kdebug("In %s\n", __func__);

	/* obj_put will be done in later thread_migrate_to_server */
	conn = obj_get(current_cap_group, conn_cap, TYPE_CONNECTION);
	if (!conn) {
		r = -ECAPBILITY;
		goto out_fail;
	}

	if ((r = grab_ipc_lock(conn)) != 0)
		goto out_fail;

	r = transfer_cap(conn, send_cap);

out_fail:
	return r;
}
