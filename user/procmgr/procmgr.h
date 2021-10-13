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

#define PREFIX "[procmgr]"
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#if 0
#define debug(fmt, ...) \
	printf(PREFIX "<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) do { } while (0)
#endif
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)

extern int fs_server_cap;
extern int lwip_server_cap;
extern int procmgr_server_cap;
extern ipc_struct_t *fs_ipc_struct;


enum proc_state {
	PROC_STATE_INIT = 1,
	PROC_STATE_RUNNING,
	PROC_STATE_EXIT,
	PROC_STATE_MAX
};

struct proc_node {
	char *name; /* The name of the process. */
	/* The capability of the process owned in procmgr. */
	int proc_cap;
	/* The capability of the process's main thread (MT) owned in procmgr. */
	int proc_mt_cap;
	pid_t pid;
	u64 badge; /* Used for recognizing a process. */
	enum proc_state state;
	int exitstatus;

	/* Connecters */
	struct proc_node *parent;
	struct list_head children; /* A list of child procs. */
	struct list_head node; /* The node in the parent's child list. */
};

struct proc_node *procmgr_find_client_proc(int client_badge);
