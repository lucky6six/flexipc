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

struct proc_node *proc_init;
struct id_manager pid_mgr;
const int PID_MAX = 1024 * 1024;

struct radix badge2proc;

typedef int (*req_handler_t)(struct proc_node *, ipc_msg_t *,
			     struct proc_request *);

pid_t handle_newproc(struct proc_node *client_proc, ipc_msg_t *ipc_msg,
		     struct proc_request *pr);
pid_t handle_wait(struct proc_node *client_proc, ipc_msg_t *ipc_msg,
		  struct proc_request *pr);
int handle_get_thread_cap(struct proc_node *client_proc, ipc_msg_t *ipc_msg,
			  struct proc_request *pr);

req_handler_t handlers[] = {
	[PROC_REQ_NEWPROC] = handle_newproc,
	[PROC_REQ_WAIT] = handle_wait,
	[PROC_REQ_GET_MT_CAP] = handle_get_thread_cap,
};

static u64 generate_badge(struct proc_node *proc)
{
	return (rand() << 10) + proc->pid;
}

static struct proc_node *__new_proc_node(struct proc_node *parent, char *name)
{
	struct proc_node *proc = malloc(sizeof(*proc));
	assert(proc);

	proc->name = name;
	proc->parent = parent;
	if (proc->parent) {
		/* Add this proc to parent's child list. */
		list_add(&proc->node, &proc->parent->children);
	}
	init_list_head(&proc->children);

	proc->state = PROC_STATE_INIT;

	return proc;
}

/**
 * The name here should be a newly allocated memory that can be directly stored
 * (and sometime later freed) in the proc_node.
 */
static
struct proc_node *new_proc_node(struct proc_node *parent,
				char *name)
{
	struct proc_node *proc = __new_proc_node(parent, name);

	proc->pid = alloc_id(&pid_mgr);

	proc->badge = generate_badge(proc);

	radix_add(&badge2proc, proc->badge, proc);

	return proc;
}

void del_proc_node(struct proc_node *proc)
{
	if (proc->name)
		free(proc->name);
	if (proc->parent) {
		list_del(&proc->node);
	}
	assert(list_empty(&proc->children));
}


int init_procmgr(void)
{
	init_id_manager(&pid_mgr, PID_MAX);

	init_radix(&badge2proc);

	return 0;
}


struct proc_node *procmgr_find_client_proc(int client_badge)
{

	struct proc_node *proc;
	proc = radix_get(&badge2proc, client_badge);
	debug("Find badge=0x%llx, get proc=%p\n", client_badge, proc);
	assert(proc);
	return proc;
}

static
char *str_join(char *delimiter, char *strings[], int n)
{
	char buf[256];
	size_t size = 256 - 1; /* 1 for the trailing \0. */
	size_t dlen = strlen(delimiter);
	char *dst = buf;
	int i;
	for (i = 0; i < n; ++i) {
		size_t l = strlen(strings[i]);
		if (i > 0)
			l += dlen;
		if (l > size) {
			printf("str_join string buffer overflow\n");
			break;
		}
		if (i > 0) {
			strcpy(dst, delimiter);
			strcpy(dst + dlen, strings[i]);
		} else {
			strcpy(dst, strings[i]);
		}
		dst += l;
		size -= l;
	}
	*dst = 0;
	return strdup(buf);
}

pid_t handle_newproc(struct proc_node *client_proc, ipc_msg_t *ipc_msg,
		     struct proc_request *pr)
{
	int argc = pr->argc;
	char *argv[PROC_REQ_ARGC_MAX];
	struct user_elf user_elf;
	struct proc_node *proc;
	int caps[3];
	int argv_start = 1;
	int ret;
	int new_proc_cap;
	int new_proc_mt_cap;

	struct launch_process_args lp_args;

	argv[0] = CHCORE_LOADER; /* argv[0] is fixed as dyn loader. */

	/* Translate to the argv pointers from argv offsets. */
	for (int i = 0; i < argc; ++i)
		argv[argv_start + i] = &pr->text[pr->argv[i]];

	debug("client: %p, cmd: %s\n", client_proc, argv[1]);

	debug("fsm ipc_struct conn_cap=%d server_type=%d\n",
	      fs_ipc_struct->conn_cap,
	      fs_ipc_struct->server_id);

	ret = readelf_from_fs(argv[argv_start], &user_elf);
	if (ret < 0) {
		printf("[Shell] No such binary: %s\n",
		       argv[argv_start]);
		return ret;
	}
	debug("got elf from fs ret=%d\n", ret);

	caps[0] = fs_server_cap;
	caps[1] = lwip_server_cap;
	caps[2] = procmgr_server_cap;

	if (ret == ET_DYN) {
		argc++;
		argv_start = 0;
	}

	/* Init the proc node to get the pid. */
	proc = new_proc_node(client_proc, str_join(" ", &argv[argv_start],
						   argc));
	assert(proc);

	lp_args.user_elf = &user_elf;
	lp_args.child_process_cap = &new_proc_cap;
	lp_args.child_main_thread_cap = &new_proc_mt_cap;
	lp_args.pmo_map_reqs = NULL;
	lp_args.nr_pmo_map_reqs = 0;
	lp_args.caps = caps;
	lp_args.nr_caps = 3;
	lp_args.cpuid = 0;
	lp_args.argc = argc;
	lp_args.argv = argv + argv_start;
	lp_args.badge = proc->badge;
	lp_args.pid = proc->pid;

	ret = launch_process_with_pmos_caps(&lp_args);

	if (ret != 0) {
		error("launch process failed\n");
		return ret;
	}

	proc->state = PROC_STATE_RUNNING;
	proc->proc_cap = new_proc_cap;
	proc->proc_mt_cap = new_proc_mt_cap;

	debug("new_proc_node: pid=%d cmd=%s parent_pid=%d\n",
	      proc->pid, proc->name, proc->parent ? proc->parent->pid : -1);

	return proc->pid;
}

#define READ_ONCE(t) (*(volatile typeof((t)) *)(&(t)))

pid_t handle_wait(struct proc_node *client_proc, ipc_msg_t *ipc_msg,
		struct proc_request *pr)
{
	assert(client_proc);
	struct proc_node *child = NULL;
	u64 cnt = 0;

	for_each_in_list(child, struct proc_node, node, &client_proc->children) {
		if (child->pid == pr->pid || pr->pid == -1)
			break;
	}
	if (!child || (child->pid != pr->pid && pr->pid != -1))
		return -ENOENT;

	/* Found. */
	debug("Found process with pid=%d proc=%p\n", pr->pid, child);
	while (READ_ONCE(child->state) != PROC_STATE_EXIT) {
		/* STATE EXIT */
		if (cnt++ % 1000000) continue;
		debug("Process (pid=%d, proc=%p) is still running\n",
		      pr->pid, child);
	}
	debug("Process (pid=%d, proc=%p) exits with %d\n",
	      pr->pid, child, child->exitstatus);
	pr->exitstatus = child->exitstatus;

	return child->pid;
}

int handle_get_thread_cap(struct proc_node *client_proc, ipc_msg_t *ipc_msg,
			  struct proc_request *pr)
{
	assert(client_proc);
	struct proc_node *child = NULL;

	for_each_in_list(child, struct proc_node, node, &client_proc->children) {
		if (child->pid == pr->pid || pr->pid == -1)
			break;
	}
	if (!child || (child->pid != pr->pid && pr->pid != -1))
		return -ENOENT;

	/* Found. */
	debug("Found process with pid=%d proc=%p\n", pr->pid, child);

	/*
	 * Set the main-thread cap in the ipc_msg and
	 * the following ipc_return_with_cap will transfer the cap.
	 */
	ipc_set_ret_msg_cap(ipc_msg, child->proc_mt_cap);

	return 0;
}


void procmgr_dispatch(ipc_msg_t *ipc_msg, u64 client_badge)
{
	int ret = 0;
	struct proc_request *pr;
	struct proc_node *client_proc;


	debug("new request from client_badge=0x%llx\n", client_badge);

	if (ipc_msg->data_len < 4) {
		error("FSM: no operation num\n");
		usys_exit(-1);
	}

	pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);

	debug("req: %d\n", pr->req);

	/* FIXME: it is strange that procmgr receives itself cap from INIT. */
	if (pr->req == PROC_REQ_INIT) {
		assert(ipc_msg->cap_slot_number == 1);
		procmgr_server_cap = ipc_get_msg_cap(ipc_msg, 0);

		/* Init the init node. */
		proc_init = new_proc_node(NULL, strdup("init"));
		proc_init->pid = alloc_id(&pid_mgr);
		proc_init->badge = client_badge;
		radix_add(&badge2proc, proc_init->badge, proc_init);

		ipc_return(ipc_msg, 0);
		return;
	}

	// Recognize client
	client_proc = procmgr_find_client_proc(client_badge);
	assert(client_proc >= 0);

	if (pr->req == PROC_REQ_GET_MT_CAP) {
		ret = handlers[pr->req](client_proc, ipc_msg, pr);
		ipc_return_with_cap(ipc_msg, ret);
		return;
	}

	if (pr->req >= 0 && pr->req < PROC_REQ_MAX) {
		ret = handlers[pr->req](client_proc, ipc_msg, pr);
		ipc_return(ipc_msg, ret);
	} else {
		error("A bad request\n");
		// TODO: handle the buggy or malicious client
		assert(0);
	}
}

void *recycle_routine(void *arg);

int main(int argc, char *argv[], char *envp[])
{
	void *token;
	pthread_t recycle_thread;

	/*
	 * A token for starting the procmgr server.
	 * This is just for preventing users run procmgr in the Shell.
	 * It is actually harmless to remove this.
	 */
	token = strstr(argv[0], KERNEL_SERVER);
	if (token == NULL) {
		error("procmgr: I am a system server instead of an (Shell) "
		      "application. Bye!\n");
		usys_exit(-1);
	}

	/*
	 * Prepare the recycle thread.
	 * TODO: what if this becomes a bottleneck?
	 */
	pthread_create(&recycle_thread, NULL, recycle_routine, NULL);

	init_procmgr();
	info("[Process Manager] register server value = %u\n",
	     ipc_register_server(procmgr_dispatch,
				 DEFAULT_CLIENT_REGISTER_HANDLER));


	/*
	 * TODO:
	 *  - use other inferfaces like sleep later.
	 *    This thread is only required when others connect this server.
	 *  - This thread can do anything it wants.
	 *
	 * TODO: When a thread exits, its cap in other cap_group should be
	 * removed (e.g., cap revoke).
	 */
	while (1) {
		usys_yield();
	}

	return 0;
}
