#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/fs_defs.h>
#include <stdio.h>
#include <chcore/proc.h>
#include <chcore/defs.h>
#include <chcore/launcher.h>
#include <errno.h>

#include <string.h>
#include <assert.h>
#include <stdlib.h>

/* An example to launch an (libc) application process */

#define AT_NULL   0	/* end of vector */
#define AT_IGNORE 1	/* entry should be ignored */
#define AT_EXECFD 2	/* file descriptor of program */
#define AT_PHDR   3	/* program headers for program */
#define AT_PHENT  4	/* size of program header entry */
#define AT_PHNUM  5	/* number of program headers */
#define AT_PAGESZ 6	/* system page size */
#define AT_BASE   7	/* base address of interpreter */
#define AT_FLAGS  8	/* flags */
#define AT_ENTRY  9	/* entry point of program */
#define AT_NOTELF 10	/* program is not ELF */
#define AT_UID    11	/* real uid */
#define AT_EUID   12	/* effective uid */
#define AT_GID    13	/* real gid */
#define AT_EGID   14	/* effective gid */
#define AT_PLATFORM 15  /* string identifying CPU for optimizations */
#define AT_HWCAP  16    /* arch dependent hints at CPU capabilities */
#define AT_CLKTCK 17	/* frequency at which times() increments */
/* AT_* values 18 through 22 are reserved */
#define AT_SECURE 23   /* secure mode boolean */
#define AT_BASE_PLATFORM 24	/* string identifying real platform, may
				 * differ from AT_PLATFORM. */
#define AT_RANDOM 25	/* address of 16 random bytes */
#define AT_HWCAP2 26	/* extension of AT_HWCAP */

#define AT_EXECFN  31	/* filename of program */

char init_env[0x1000];
// TODO: ARCH-dependent string
const char PLAT[] = "aarch64";

/* enviroments */
#define SET_LD_LIB_PATH
const char LD_LIB_PATH[] = "LD_LIBRARY_PATH=/";

/**
 * NOTE: The stack format:
 * http://articles.manugarg.com/aboutelfauxiliaryvectors.html
 * The url shows a 32-bit format stack, but we implemented a 64-bit stack below.
 * People as smart as you could understand the difference.
 */
static void construct_init_env(char *env, u64 top_vaddr,
			       struct process_metadata *meta, char *name,
			       struct pmo_map_request *pmo_map_reqs,
			       int nr_pmo_map_reqs,
			       int caps[],
			       int nr_caps,
			       int argc,
			       char **argv,
			       int pid,
			       u64 load_offset)
{
	int i, j;
	u64 *buf;
	char *str;
	size_t l;
	int str_bytes_left;

	/* clear init_env */
	memset(env, 0x1000, 0);

	// TODO: not the the total length of strings
	/* The strings part starts from the middle of the page. */
	str_bytes_left = 0x1000 >> 1;
	str = env + (0x1000 >> 1);
#define __ptr2vaddr(ptr) ((top_vaddr) - 0x1000 + ((ptr) - (env)))

	/* Prepare the stack. */
	buf = (u64 *)env;
	/* argc */
	buf[0] = argc;
	/* argv */
	for (i = 0; i < argc; ++i) {
		l = strlen(argv[i]);
		assert(str_bytes_left > l);
		/* Copy the string. */
		strcpy(str, argv[i]);
		/* Set the pointer. */
		buf[1 + i] = __ptr2vaddr(str);
		str += l + 1;
		str_bytes_left -= l + 1;
	}
	buf[1 + argc] = (u64)NULL;

	/* envp */
	i = 1 + argc + 1;
	/*
	 * See libc_connect_services in
	 * user/musl-1.1.24/src/chcore-port/syscall_dispatcher.c
	 * for more details of the layout of envp.
	 *
	 * Simply speaking, it consists of:
	 *	nr_pmo_map_addr
	 * 	pmo_map_addrs 0, 1, 2, ...
	 * 	nr_caps
	 * 	caps 0, 1, 2, ...
	 * 	traditional environs
	 * 	NULL
	 */

	if (nr_pmo_map_reqs == 0) {
		/* set info_page_vaddr to -1 (means none) */
		*(buf+i) = (u64)ENVP_NONE_PMOS;
		i  = i + 1;
	} else {
		/* set the number of pmo_cap_addr */
		*(buf+i) = (u64)nr_pmo_map_reqs;
		i  = i + 1;

		/* set the addr one by one */
		for (j = 0; j < nr_pmo_map_reqs; ++j) {
			*(buf+i+j) = (u64)pmo_map_reqs[j].addr;
		}
		i = i + j;
	}

	/* set the number of caps */
	if (nr_caps == 0) {
		/* use -1 to mean no caps transferred */
		*(buf+i) = (u64)ENVP_NONE_CAPS;
		i  = i + 1;
	} else {
		*(buf+i) = (u64)nr_caps;
		i  = i + 1;

		for (j = 0; j < nr_caps; ++j) {
			*(buf+i+j) = (u64)caps[j];
		}
		i = i + j;
	}

	/* add more envp here */
	{
#ifdef SET_LD_LIB_PATH
		/* Add the LD_LIBRARY_PATH=xxx here */
		l = strlen(LD_LIB_PATH);
		assert(str_bytes_left > l);
		/* Copy the string. */
		strcpy(str, LD_LIB_PATH);
		/* Set the pointer. */
		*(buf+i) = __ptr2vaddr(str);
		str += l + 1;
		str_bytes_left -= l + 1;
		i += 1;
#endif

		/* Add "PID=xxx" here; PID is smaller than 65536 (see ProcMgr) */
		char pidstr[16];
		snprintf(pidstr, 15, "PID=%d\n", pid);
		l = strlen(pidstr);
		assert(str_bytes_left > l);
		/* Copy the string. */
		strcpy(str, pidstr);
		/* Set the pointer. */
		*(buf+i) = __ptr2vaddr(str);
		str += l + 1;
		str_bytes_left -= l + 1;
		i += 1;

	}
	/* The end of envp */
	*(buf+i) = (u64)NULL;


	/* auxv */
	i  = i + 1;
	*(buf+i+0) = AT_SECURE;
	*(buf+i+1) = 0;

	*(buf+i+2) = AT_PAGESZ;
	*(buf+i+3) = 0x1000;

	*(buf+i+4) = AT_PHDR;
	*(buf+i+5) = meta->phdr_addr + load_offset;

	// printf("phdr_addr is 0x%lx\n", meta->phdr_addr);

	*(buf+i+6) = AT_PHENT;
	*(buf+i+7) = meta->phentsize;

	*(buf+i+8) = AT_PHNUM;
	*(buf+i+9) = meta->phnum;

	*(buf+i+10) = AT_FLAGS;
	*(buf+i+11) = meta->flags;

	*(buf+i+12) = AT_ENTRY;
	*(buf+i+13) = meta->entry + load_offset;


	*(buf+i+14) = AT_UID;
	*(buf+i+15) = 1000;

	*(buf+i+16) = AT_EUID;
	*(buf+i+17) = 1000;

	*(buf+i+18) = AT_GID;
	*(buf+i+19) = 1000;

	*(buf+i+20) = AT_EGID;
	*(buf+i+21) = 1000;

	*(buf+i+22) = AT_CLKTCK;
	*(buf+i+23) = 100;

	*(buf+i+24) = AT_HWCAP;
	*(buf+i+25) = 0;

	*(buf+i+25) = AT_PLATFORM;

	{
		l = strlen(PLAT);
		assert(str_bytes_left > l);
		/* Copy the PLAT string. */
		strcpy(str, PLAT);
		/* Set the PLAT pointer. */
		*(buf+i+26) = __ptr2vaddr(str);

		str += l + 1;
		str_bytes_left -= l + 1;
	}


	*(buf+i+27) = AT_RANDOM;
	*(buf+i+28) = top_vaddr - 64; /* random 16 bytes */

	/* add more auxv here */

	// TODO: libc.so
	*(buf+i+29) = AT_BASE;
	//*(buf+i+30) = 0x400000000000UL;
	*(buf+i+30) = 0;

	*(buf+i+31) = AT_NULL;
	*(buf+i+32) = 0;

	#if 0
	*(buf+i+29) = AT_NULL;
	*(buf+i+30) = 0;
	#endif

}

#ifndef FBINFER
/*
 * user_elf: elf struct
 * child_process_cap: if not NULL, set to child_process_cap that can be
 *                    used in current process.
 *
 * child_main_thread_cap: if not NULL, set to child_main_thread_cap
 *                        that can be used in current process.
 *
 * pmo_map_reqs, nr_pmo_map_reqs: input pmos to map in the new process
 *
 * caps, nr_caps: copy from farther process to child process
 *
 * cpuid: affinity
 *
 * argc/argv: the number of arguments and the arguments.
 *
 * badge: the badge is generated by the invokder (usually procmgr).
 *
 */
int launch_process_with_pmos_caps(struct launch_process_args *lp_args)
{
	int new_process_cap;
	/* The name of process/cap_group: mainly used for debugging */
	char *process_name;
	int main_thread_cap;
	int ret;
	long pc;
	/* for creating pmos */
	struct pmo_request pmo_requests[2];
	int main_stack_cap;
	int forbid_area_cap;
	u64 offset;
	u64 stack_top;
	u64 p_vaddr;
	int i;
	/* for mapping pmos */
	struct pmo_map_request pmo_map_requests[2];
	int *transfer_caps = NULL;
	u64 load_offset = 0;

	struct user_elf *user_elf = lp_args->user_elf;
	int *child_process_cap = lp_args->child_process_cap;
	int *child_main_thread_cap = lp_args->child_main_thread_cap;
	struct pmo_map_request *pmo_map_reqs = lp_args->pmo_map_reqs;
	int nr_pmo_map_reqs = lp_args->nr_pmo_map_reqs;
	int *caps = lp_args->caps;
	int nr_caps = lp_args->nr_caps;
	s32 cpuid = lp_args->cpuid;
	int argc = lp_args->argc;
	char **argv = lp_args->argv;
	u64 badge = lp_args->badge;
	int pid = lp_args->pid;


	/* for usys_creat_thread */
	struct thread_args args;

	process_name = user_elf->path;

	/* Load libc.so first instead of loading the dynamic application
	 * TODO: the load_offset for libc.so
	 */
	if (strstr(user_elf->path, "libc.so") != NULL) {
		printf("CHCORE_LOADER warning: loader itself is loaded at a magic address\n");
		load_offset = 0x400000000000UL;

		for (i = 0; i < 2; ++i) {
			user_elf->user_elf_seg[i].p_vaddr += load_offset;
		}

		if (argc > 1) {
			/* Set the process_name to the dynamic application */
			process_name = argv[1];
		}
	}
	assert(process_name != NULL);


	/* create a new process with an empty vmspace */
	new_process_cap = usys_create_cap_group(badge, process_name,
						strlen(process_name));
	if (new_process_cap < 0) {
		printf("%s: fail to create new_process_cap (ret: %d)\n",
		       __func__, new_process_cap);
		goto fail;
	}

	if (nr_caps > 0) {
		transfer_caps = malloc(sizeof(int) * nr_caps);
		/* usys_transfer_caps is used during process creation */
		ret = usys_transfer_caps(new_process_cap, caps, nr_caps,
					 transfer_caps);
		if (ret != 0) {
			printf("usys_transfer_caps ret %d\n", ret);
			usys_exit(-1);
		}
	}

	/* load each segment in the elf binary */
	// TODO: tricky 2 now
	for (i = 0; i < 2; ++i) {
		p_vaddr = user_elf->user_elf_seg[i].p_vaddr;
		ret = usys_map_pmo(new_process_cap,
			   user_elf->user_elf_seg[i].elf_pmo,
			   ROUND_DOWN(p_vaddr, PAGE_SIZE),
			   user_elf->user_elf_seg[i].flags);

		if (ret < 0) {
			printf("usys_map_pmo ret %d\n", ret);
			usys_exit(-1);
		}
	}

	// debug("meta.pc: 0x%lx\n", user_elf->elf_meta->entry);

	/* L1 icache & dcache have no coherence */
	/* TODO: actually, I am not sure whether this is necessary here*/
	// flush_idcache();

	pc = user_elf->elf_meta.entry;
	// printf("pc is 0x%lx\n", pc);

	/* create pmos in current process */
	pmo_requests[0].size = MAIN_THREAD_STACK_SIZE;
	pmo_requests[0].type = PMO_ANONYM;

	pmo_requests[1].size = PAGE_SIZE;
	pmo_requests[1].type = PMO_FORBID;

	// -------------- merge with prevs
	ret = usys_create_pmos((void *)pmo_requests, 2);
	// --------------

	if (ret != 0) {
		printf("%s: fail to create_pmos (ret: %d)\n",
		       __func__, ret);
		goto fail;
	}

	/* get result caps */
	main_stack_cap = pmo_requests[0].ret_cap;
	if (main_stack_cap < 0) {
		printf("%s: fail to create_pmos (ret cap: %d)\n",
		       __func__, main_stack_cap);
		goto fail;
	}

	/* Map a forbidden pmo in case of stack overflow */
	forbid_area_cap = pmo_requests[1].ret_cap;
	if (forbid_area_cap < 0) {
		printf("%s: fail to create_pmos (ret cap: %d)\n",
		       __func__, forbid_area_cap);
		goto fail;
	}

	/* usys_write_pmo -> prepare the stack */
	stack_top = MAIN_THREAD_STACK_BASE + MAIN_THREAD_STACK_SIZE;
	construct_init_env(init_env, stack_top, &user_elf->elf_meta,
			   user_elf->path, pmo_map_reqs, nr_pmo_map_reqs,
			   transfer_caps, nr_caps, argc, argv, pid,
			   load_offset);
	offset = MAIN_THREAD_STACK_SIZE - 0x1000;
	//printf("init_env argc: %ld\n", (*(u64 *)init_env));
	ret = usys_write_pmo(main_stack_cap, offset, init_env, 0x1000);
	if (ret != 0) {
		printf("%s: fail to write_pmo (ret: %d)\n",
		       __func__, ret);
		goto fail;
	}

	/* map the pmos into the new process */
	pmo_map_requests[0].pmo_cap = main_stack_cap;
	pmo_map_requests[0].addr = MAIN_THREAD_STACK_BASE;
	pmo_map_requests[0].perm = VM_READ | VM_WRITE;

	pmo_map_requests[1].pmo_cap = forbid_area_cap;
	pmo_map_requests[1].addr = MAIN_THREAD_STACK_BASE - PAGE_SIZE;
	pmo_map_requests[1].perm = VM_FORBID;

	// -------------- merge with prevs
	ret = usys_map_pmos(new_process_cap, (void *)pmo_map_requests, 1);
	// -------------- merge with prevs

	if (ret != 0) {
		printf("%s: fail to map_pmos (ret: %d)\n",
		       __func__, ret);
		goto fail;
	}

	// printf("stack: 0x%lx\n", MAIN_THREAD_STACK_BASE + offset);

	if (nr_pmo_map_reqs) {
		ret = usys_map_pmos(new_process_cap, (void *)pmo_map_reqs,
				    nr_pmo_map_reqs);
		if (ret != 0) {
			printf("%s: fail to map_pmos (ret: %d)\n",
			       __func__, ret);
			goto fail;
		}
	}
	/*
	 * create main thread in the new process.
	 * main_thread_cap is the cap can be used in current process.
	 */
	args.cap_group_cap = new_process_cap;
	args.stack = MAIN_THREAD_STACK_BASE + offset;
	args.pc = pc + load_offset;
	args.arg = (u64)NULL;
	args.prio = MAIN_THREAD_PRIO;
	args.tls = cpuid;
	args.is_shadow = 0;
	main_thread_cap = usys_create_thread((u64)&args);

	if (child_process_cap)
		*child_process_cap = new_process_cap;
	if (child_main_thread_cap)
		*child_main_thread_cap = main_thread_cap;
	// printf("[%s succeeds] main_thread (cap: %d) in the new process (cap: %d), pc is 0x%lx\n",
	//        __func__, main_thread_cap, new_process_cap, pc);

	return 0;
fail:
	return -EINVAL;
}


/* TODO: check whether this is out-dated */
int launch_process(struct user_elf *user_elf,
		   int *child_process_cap,
		   int *child_main_thread_cap,
		   u64 badge)
{
	struct launch_process_args lp_args;
	char *argv[2] = {user_elf->path, NULL};

	lp_args.user_elf = user_elf;
	lp_args.child_process_cap = child_process_cap;
	lp_args.child_main_thread_cap = child_main_thread_cap;
	lp_args.pmo_map_reqs = NULL;
	lp_args.nr_pmo_map_reqs = 0;
	lp_args.caps = NULL;
	lp_args.nr_caps = 0;
	lp_args.cpuid = 0;
	lp_args.argc = 1;
	lp_args.argv = argv;
	lp_args.badge = badge;
	/* Note: no PID */
	lp_args.pid = 0;

	return launch_process_with_pmos_caps(&lp_args);
}

/* TODO: check whether this is out-dated */
int launch_process_path(const char *path, int *new_thread_cap,
			struct pmo_map_request *pmo_map_reqs,
			int nr_pmo_map_reqs, int caps[], int nr_caps,
			u64 badge)
{
	struct launch_process_args lp_args;
	struct user_elf user_elf;
	int ret;
	char *argv[2] = {(char *)path, NULL};

	ret = readelf_from_fs(path, &user_elf);
	if (ret < 0) {
		printf("[Client] Cannot create server.\n");
		return ret;
	}

	lp_args.user_elf = &user_elf;
	lp_args.child_process_cap = NULL;
	lp_args.child_main_thread_cap = new_thread_cap;
	lp_args.pmo_map_reqs = pmo_map_reqs;
	lp_args.nr_pmo_map_reqs = nr_pmo_map_reqs;
	lp_args.caps = caps;
	lp_args.nr_caps = nr_caps;
	lp_args.cpuid = NO_AFF;
	lp_args.argc = 1;
	lp_args.argv = argv;
	lp_args.badge = badge;
	/* Note: no PID */
	lp_args.pid = 0;

	return launch_process_with_pmos_caps(&lp_args);
}

#endif
