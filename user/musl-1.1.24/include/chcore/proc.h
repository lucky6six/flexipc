#pragma once

#include <chcore/type.h>
#include <chcore/fs_defs.h>


struct pmo_request {
	/* input: args */
	u64 size;
	u64 type;

	/* output: return value */
	u64 ret_cap;
};

struct pmo_map_request {
	/* input: args */
	u64 pmo_cap;
	u64 addr;
	u64 perm;

	/* output: return value */
	u64 ret;
};

/* FIXME(MK): This structure is duplicated in kernel/object/thread.c. */
struct thread_args {
	u64 cap_group_cap;
	u64 stack;
	u64 pc;
	u64 arg;
	u32 prio;
	u64 tls;
	u32 is_shadow;
};

struct launch_process_args {
	struct user_elf *user_elf;
	int *child_process_cap;
	int *child_main_thread_cap;
	struct pmo_map_request *pmo_map_reqs;
	int nr_pmo_map_reqs;
	int *caps;
	int nr_caps;
	s32 cpuid;
	int argc;
	char **argv;
	u64 badge;
	int pid;
};

/* FIXED_BADGE used in init (for system servers): [2, 10) */
#define FIXED_BADGE_DURING_INIT_BASE (2)
#define FIXED_BADGE_DURING_INIT_END (10)

#define FIXED_BADGE_TMPFS (10)

int launch_process(struct user_elf *user_elf,
		   int *child_process_cap,
		   int *child_main_thread_cap,
		   u64 badge);

int launch_process_with_pmos_caps(struct launch_process_args *lp_args);

int launch_process_path(const char *path, int *new_thread_cap,
			struct pmo_map_request *pmo_map_reqs,
			int nr_pmo_map_reqs,
			int caps[], int nr_caps,
			u64 badge);

pid_t chcore_waitpid(pid_t pid, int *status, int options, int d);
