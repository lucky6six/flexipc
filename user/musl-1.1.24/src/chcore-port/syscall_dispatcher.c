#define _GNU_SOURCE

#include <assert.h>
#include <bits/alltypes.h>
#include <bits/syscall.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <sys/resource.h>
#include <syscall_arch.h>
#include <termios.h>
#include <stdlib.h>

/* ChCore Port header */
#include "fd.h"
#include "poll.h"
#include <chcore/bug.h>
#include <chcore/defs.h>
#include <chcore/fs_defs.h>
#include <chcore/ipc.h>
#include <chcore/lwip_defs.h>
#include <chcore/procmgr_defs.h>
#include "fd.h"
#include "poll.h"
#include "futex.h"
#include "eventfd.h"
#include "pipe.h"
#include "timerfd.h"
#include "socket.h"
#include "file.h"

#define debug(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define warn(fmt, ...) printf("[WARN] " fmt, ##__VA_ARGS__)
#define warn_once(fmt, ...) do {  \
	static int __warned = 0;  \
	if (__warned) break;      \
	__warned = 1;             \
	warn(fmt, ##__VA_ARGS__);    \
} while (0)

// #define TRACE_SYSCALL
#ifdef TRACE_SYSCALL
#define syscall_trace(n) \
	printf("[dispatcher] %s: syscall_num %ld\n", __func__, n)
#else
#define syscall_trace(n) \
	do { \
	} while (0)
#endif

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

/*
 * Used to issue ipc to user-space services
 * Register to server before executing the program
 */
int fs_server_cap;
int lwip_server_cap;

int procmgr_server_cap;
/* An application will not send IPCs to procmgr frequently,
 * so it uses a global ipc_struct_t (shared by different threads).
 */
static ipc_struct_t __procmgr_ipc_struct = {0};
ipc_struct_t *procmgr_ipc_struct;

/* TODO: how to handle these fake ones? */
static long fake_tid_val = 10086;

int chcore_pid = 0;

/*
 * Helper function.
 */
static int __xstatxx(int req, int fd, const char *path, int flags,
		     void *statbuf, size_t bufsize);
void dead(long n);

static inline void strncpy_safe(char *dst, const char *src, size_t dst_size)
{
	strncpy(dst, src, dst_size);
	dst[dst_size - 1] = '\0';
}


/*
 * ChCore `envp` layout convention:
 *
 * -----------------
 *  nr_map_pmo_addrs
 * -----------------
 *  empty if nr_map_pmo_addrs == -1
 *  otherwise, nr addrs (a father process maps nr pmos into the child)
 * -----------------
 *  nr_caps
 * -----------------
 *  empty if nr_caps == -1
 *  otherwise, nr caps and the default order is as follows:
 *  cap[0]: FS
 *  cap[1]: NET
 *  cap[2]: ProcMgr
 * -----------------
 *  traditional environs
 * -----------------
 *  NULL (0) is used for marking the end
 * -----------------
 *
 *  For easing programming, application developers need not to know
 *  this although an application can override this function.
 */

void __libc_connect_services(char *envp[])
{
	long *envp_ptr;
	long nr_pmo_map_addr;
	long nr_caps;
	long *caps;

	envp_ptr = (long *)envp;
	nr_pmo_map_addr = envp_ptr[0];
	if (nr_pmo_map_addr == ENVP_NONE_PMOS)
		envp_ptr += 1;
	else
		envp_ptr += (nr_pmo_map_addr + 1);

	nr_caps = envp_ptr[0];
	if (nr_caps == ENVP_NONE_CAPS) {
		//printf("%s: connect to zero system services.\n", __func__);
		return;
	}

	caps = envp_ptr + 1;

	fs_server_cap = (int)(caps[0]);
	fs_ipc_struct->conn_cap = 0;
	fs_ipc_struct->server_id = FS_MANAGER;

	/* Connect to FS only */
	if ((--nr_caps) == 0) return;

	lwip_server_cap = (int)(caps[1]);
	lwip_ipc_struct->conn_cap = 0;
	lwip_ipc_struct->server_id = NET_MANAGER;

	/* Connect to FS and NET */
	if ((--nr_caps) == 0) return;

	procmgr_server_cap = (int)(caps[2]);
	procmgr_ipc_struct = &__procmgr_ipc_struct;
	procmgr_ipc_struct->conn_cap = 0;
	procmgr_ipc_struct->server_id = PROC_MANAGER;

	/* Connect to FS, NET, and PROCMGR */
	//printf("%s: connect FS, NET and PROCMGR\n", __func__);
}
weak_alias(__libc_connect_services, libc_connect_services);

/*
 * This function is local to libc and it will
 * only be executed once during the libc init time.
 *
 * It will be executed in the dynamic loader (for dynamic-apps) or
 * just before calling user main (for static-apps).
 * Nevertheless, when loading a dynamic application, it will be invoked twice.
 * This is why the variable `initialized` is required.
 */
void __libc_chcore_init(char *envp[])
{
	static int initialized = 0;

	if (initialized == 1) return;
	initialized = 1;

	/* Open stdin/stdout/stderr */
	/* STDIN */
	int fd0 = alloc_fd();
	assert(fd0 == STDIN_FILENO);
	fd_dic[fd0]->type = FD_TYPE_STDIN;
	fd_dic[fd0]->fd = fd0;
	fd_dic[fd0]->fd_op = &stdin_ops;

	/* STDOUT */
	int fd1 = alloc_fd();
	assert(fd1 == STDOUT_FILENO);
	fd_dic[fd1]->type = FD_TYPE_STDOUT;
	fd_dic[fd1]->fd = fd1;
	fd_dic[fd1]->fd_op = &stdout_ops;

	/* STDERR */
	int fd2 = alloc_fd();
	assert(fd2 == STDERR_FILENO);
	fd_dic[fd2]->type = FD_TYPE_STDERR;
	fd_dic[fd2]->fd = fd2;
	fd_dic[fd2]->fd_op = &stderr_ops;

	/* ProcMgr passes PID through env variable */
	char *pidstr = getenv("PID");
	if (pidstr) {
		chcore_pid = atoi(pidstr);
	}

	libc_connect_services(envp);
}
/*
 * LibC may invoke syscall directly through assembly code like:
 * src/thread/x86_64/__set_thread_area.s
 *
 * So, we also need to rewrite those function.
 */

/*
 * TODO (tmac): some SYS_num are specific to architectures.
 * We can ag "x86_64"/"aarch64" in src directory to see that.
 *
 * Maybe we can use another file for hooking those specific SYS_num.
 */

long __syscall0(long n)
{
	if (n != SYS_sched_yield)
		syscall_trace(n);

	switch (n) {
	case SYS_getppid:
		warn_once("faked ppid\n");
		return chcore_pid;
	case SYS_getpid:
		return chcore_pid;
	case SYS_geteuid:
		warn_once("faked euid\n");
		return 0; /* root. */
	case SYS_sched_yield:
		chcore_syscall0(CHCORE_SYS_yield);
		return 0;
	default:
		dead(n);
		return chcore_syscall0(n);
	}
}

long __syscall1(long n, long a)
{
	syscall_trace(n);

	switch (n) {
#ifdef SYS_unlink
	case SYS_unlink:
		return __syscall3(SYS_unlinkat, AT_FDCWD, a, 0);
#endif
#ifdef SYS_rmdir
	case SYS_rmdir:
		return __syscall3(SYS_unlinkat, AT_FDCWD, a, AT_REMOVEDIR);
#endif
	case SYS_close:
		return chcore_close(a);
	case SYS_umask:
		debug("ignore SYS_uname\n");
		return 0;
	case SYS_uname:
		debug("ignore SYS_uname\n");
		return -1;
	case SYS_exit:
		/* Thread exit: a is exitcode */
		chcore_syscall1(CHCORE_SYS_thread_exit, a);
		printf("[libc] error: thread_exit should never return.\n");
		return 0;
	case SYS_exit_group:
		/* Group exit: a is exitcode */
		chcore_syscall1(CHCORE_SYS_process_exit, a);
		printf("[libc] error: process_exit should never return.\n");
		return 0;
	case SYS_set_tid_address:
		// debug("ignore SYS_set_tid_address\n");
		// TODO: maybe we need to obey the convention
		return fake_tid_val++;
	case SYS_brk:
		return chcore_syscall1(CHCORE_SYS_handle_brk, a);
	case SYS_chdir: {
		return chcore_chdir((const char *)a);
	}
	case SYS_fchdir: {
		return chcore_fchdir(a);
	} break;
	case SYS_fsync: {
		return chcore_fsync(a);
	}
	case SYS_dup: {
		return __syscall3(SYS_fcntl, a, F_DUPFD, 0);
	}
	case SYS_shmdt: {
		return chcore_syscall1(CHCORE_SYS_shmdt, a);
	}
#ifdef SYS_pipe
	case SYS_pipe:
		return __syscall2(SYS_pipe2, a, 0);
#endif
	default:
		dead(n);
		return chcore_syscall1(n, a);
	}
}

long __syscall2(long n, long a, long b)
{
	syscall_trace(n);

	switch (n) {
#ifdef SYS_stat
	case SYS_stat:
		return __syscall4(SYS_newfstatat, AT_FDCWD, a, b, 0);
#endif
#ifdef SYS_lstat
	case SYS_lstat:
		return __syscall4(SYS_newfstatat, AT_FDCWD, a, b,
				  AT_SYMLINK_NOFOLLOW);
#endif
#ifdef SYS_access
	case SYS_access:
		return __syscall4(SYS_faccessat, AT_FDCWD, a, b, 0);
#endif
#ifdef SYS_rename
	case SYS_rename:
		return __syscall4(SYS_renameat, AT_FDCWD, a, AT_FDCWD, b);
#endif
#ifdef SYS_symlink
	case SYS_symlink:
		return __syscall3(SYS_symlinkat, a, AT_FDCWD, b);
#endif
#ifdef SYS_mkdir
	case SYS_mkdir:
		return __syscall3(SYS_mkdirat, AT_FDCWD, a, b);
#endif
#ifdef SYS_open
	case SYS_open:
		return __syscall6(SYS_open, a, b, 0, 0, 0, 0);
#endif
	case SYS_fstat: {
		if (fd_dic[a] == 0) {
			return -EBADF;
		}

		return __xstatxx(FS_REQ_FSTAT, fd_dic[a]->fd, /* fd */
				 NULL,			      /* path  */
				 0,			      /* flags */
				 (struct stat *)b,	    /* statbuf */
				 sizeof(struct stat)	  /* bufsize */
		);
	}
	case SYS_statfs: {
		return __xstatxx(FS_REQ_STATFS, AT_FDCWD, /* fd */
				 (const char *)a,	 /* path  */
				 0,			  /* flags */
				 (struct statfs *)b,      /* statbuf */
				 sizeof(struct statfs)    /* bufsize */
		);
	}
	case SYS_fstatfs: {
		if (fd_dic[a] == 0) {
			return -EBADF;
		}

		return __xstatxx(FS_REQ_FSTATFS, fd_dic[a]->fd, /* fd */
				 NULL,				/* path  */
				 0,				/* flags */
				 (struct statfs *)b,		/* statbuf */
				 sizeof(struct statfs)		/* bufsize */
		);
	}
	case SYS_getcwd: {
		return chcore_getcwd((char *)a, b);
	}
	case SYS_ftruncate: {
		return chcore_ftruncate(a, b);
	}
	case SYS_clock_gettime: {
		return chcore_syscall2(CHCORE_SYS_clock_gettime, a, b);
	}
	case SYS_set_robust_list: {
		/* TODO Futex not implemented! */
		return 0;
	}
	case SYS_munmap: {
		/* munmap: @a is addr, @b is len */
		return chcore_syscall2(CHCORE_SYS_handle_munmap, a, b);
	}
	case SYS_setpgid: {
		warn_once("setpgid is not implemented.\n");
		return 0;
	}
	case SYS_pipe2: {
		return chcore_pipe2((int *)a, b);
	}
	case SYS_membarrier: {
		warn_once("SYS_membarrier is not implmeneted.\n");
		return 0;
	}
	case SYS_getrusage:
		warn_once("getrusage is not implemented.\n");
		memset((void *)b, 0, sizeof(struct rusage));
		return 0;
	case SYS_fcntl: {
		return __syscall3(n, a, b, 0);
	}
	case SYS_eventfd2: {
		return chcore_eventfd(a, b);
	}
	case SYS_timerfd_create:
		return chcore_timerfd_create(a, b);
#ifdef SYS_timerfd_gettime64
	case SYS_timerfd_gettime64:
#endif
	case SYS_timerfd_gettime:
		return chcore_timerfd_gettime(a, (struct itimerspec *)b);
	default:
		dead(n);
		return chcore_syscall2(n, a, b);
	}
}


long __syscall3(long n, long a, long b, long c)
{
	/* avoid recurrent invocation */
	if (n != SYS_writev && n != SYS_ioctl && n != SYS_read)
		syscall_trace(n);

	switch (n) {
#ifdef SYS_readlink
	case SYS_readlink:
		return __syscall4(SYS_readlinkat, AT_FDCWD, a, b, c);
#endif
#ifdef SYS_open
	case SYS_open:
		return __syscall6(SYS_open, a, b, c, 0, 0, 0);
#endif
	case SYS_readv:
		return __syscall6(SYS_readv, a, b, c, 0, 0, 0);
	case SYS_ioctl: {
		return chcore_ioctl(a, b, (void *)c);
	}
	case SYS_writev: {
		return __syscall6(SYS_writev, a, b, c, 0, 0, 0);
	}
	case SYS_read: {
		return chcore_read((int)a, (void *)b, (size_t)c);
	}
	case SYS_sched_getaffinity: {
		/* get affinity of tid */
		/* Convert special tid to cap */
		if (a == 0) {
			/* XXX: currently, we use -1 to represent the current
			 * thread */
			a = -1;
		}
		int ret = chcore_syscall1(CHCORE_SYS_get_affinity, a);
		if (ret >= 0) {
			CPU_SET(ret, c);
			return 0;
		} else {
			/* Fail to get the affinity */
			return ret;
		}
	}
	case SYS_sched_setaffinity: {
		/* set affinity of tid */
		/* Convert special tid to cap */
		if (a == 0) {
			/* XXX: currently, we use -1 to represent the current
			 * thread */
			a = -1;
		}

		/* Find the cpu (According to sched.h max is 128 */
		/* XXX: Currently we only support set one cpu */
		for (int i = 0; i < 128; i++) {
			if (CPU_ISSET(i, c)) {
				/* XXX tid to cap */
				return chcore_syscall2(CHCORE_SYS_set_affinity,
						       a, i);
			}
		}
		// /* No cpu aff has been set */
		return -1;
	}
	case SYS_lseek: {
		return chcore_lseek(a, b, c);
	}
	case SYS_mkdirat: {
		return chcore_mkdirat(a, (const char *)b, c);
	}
	case SYS_unlinkat: {
		return chcore_unlinkat(a, (const char *)b, c);
	}
	case SYS_symlinkat: {
		return chcore_symlinkat((const char *)a, b, (const char *)c);
	}
	case SYS_getdents64: {
		return chcore_getdents64(a, (char *)b, c);
	}
	case SYS_openat: {
		return __syscall6(SYS_openat, a, b, c, /* mode */ 0, 0, 0);
	}
	case SYS_futex: {
		/*
		 * XXX: Multiple sys_futex entries as parameter number varies in
		 *	different futex ops.
		 */
		return chcore_futex((int *)a, b, c, NULL, NULL, 0);
	}
	case SYS_fcntl: {
		return chcore_fcntl(a, b, c);
	}
	case SYS_fchown: {
		warn("SYS_fchown is not implemented.\n");
		return 0;
	}
	case SYS_madvise: {
		// TODO: no support now (just ingore the advise)
		warn("SYS_madvise is not implemented.\n");
		return 0;
	}
	case SYS_dup3: {
		int err;
		int oldfd = a, newfd = b;

		/* TODO: There is no SYS_dup2 in musl-libc, dup2 use dup3 and
		 * set flags to zero. flags is not handled yet. If we want to
		 * support dup3, we should take flags into consideration */
		BUG_ON(fd_dic[oldfd] == 0);
		/* If the fd newfd was previously open, it is silently colosed
		 * before being reused. */
		if (fd_dic[newfd])
			err = chcore_close(newfd);
		if (err)
			return err;
		fd_dic[newfd]->cap = fd_dic[oldfd]->cap;
		fd_dic[newfd]->fd = fd_dic[oldfd]->fd;
		fd_dic[newfd]->type = fd_dic[oldfd]->type;
		fd_dic[newfd]->fd_op = fd_dic[oldfd]->fd_op;
		return newfd;
	}
	case SYS_mprotect: {
		/* mproctect: @a addr, @b len, @c prot */
		return chcore_syscall3(CHCORE_SYS_mprotect, a, b, c);
	}
	case SYS_mincore: {
		// TODO: no support now (just tell the caller: every
		// page is in memory)
		warn("SYS_mincore is not implemented.\n");

		/* mincore: @a addr, @b length, @c vec */
		size_t size;
		size_t cnt;
		size_t i;
		unsigned char *vec;

		size = ROUND_UP(b, PAGE_SIZE);
		cnt = size / PAGE_SIZE;
		vec = (unsigned char *)c;

		for (i = 0; i < cnt; ++i) {
			vec[i] = (char)1;
		}
		return 0;
	}
	case SYS_shmget: {
		return chcore_syscall3(CHCORE_SYS_shmget, a, b, c);
	}
	case SYS_shmat: {
		return chcore_syscall3(CHCORE_SYS_shmat, a, b, c);
	}
	case SYS_shmctl: {
		return chcore_syscall3(CHCORE_SYS_shmctl, a, b, c);
	}
	default:
		dead(n);
		return chcore_syscall3(n, a, b, c);
	}
}

pid_t chcore_waitpid(pid_t pid, int *status, int options, int d)
{
	ipc_msg_t *ipc_msg;
	int ret;
	struct proc_request pr;
	struct proc_request *reply_pr;

	/* register IPC client */
	procmgr_ipc_struct = ipc_register_client(procmgr_server_cap);
	assert(procmgr_ipc_struct);

	ipc_msg = ipc_create_msg(procmgr_ipc_struct,
				 sizeof(struct proc_request), 0);
	pr.req = PROC_REQ_WAIT;
	pr.pid = pid;

	ipc_set_msg_data(ipc_msg, &pr, 0, sizeof(pr));
	/* This ipc_call returns the pid. */
	ret = ipc_call(procmgr_ipc_struct, ipc_msg);
	if (ret != -1) {
		/* Get the actual exit status. */
		reply_pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);
		*status = reply_pr->exitstatus;
	}
	// debug("pid=%d => exitstatus: %d\n", pid, pr.exitstatus);

	ipc_destroy_msg(ipc_msg);
	return ret;

}

long __syscall4(long n, long a, long b, long c, long d)
{
	syscall_trace(n);

	switch (n) {
	case SYS_wait4: {
		return chcore_waitpid((pid_t)a, (int *)b, (int)c, (int)d);
	}
	case SYS_newfstatat: {
		return __xstatxx(FS_REQ_FSTATAT, a,  /* dirfd */
				 (const char *)b,    /* path  */
				 d,		     /* flags */
				 (struct stat *)c,   /* statbuf */
				 sizeof(struct stat) /* bufsize */
		);
	}
	case SYS_readlinkat: {
		return chcore_readlinkat(a, (const char *)b, (char *)c, d);
	}
	case SYS_renameat: {
		return chcore_renameat(a, (const char *)b, c, (const char *)d);
	}
	case SYS_faccessat: {
		return chcore_faccessat(a, (const char *)b, c, d);
	}
	case SYS_fallocate: {
		return chcore_fallocate(a, b, c, d);
	}
	case SYS_futex: {
		return chcore_futex((int *)a, b, c, (struct timespec *)d, NULL, 0);
	}
	case SYS_rt_sigprocmask: {
		warn_once("SYS_rt_sigprocmask is not implemented.\n");
		return 0;
	}
	case SYS_rt_sigaction: {
		warn_once("SYS_rt_sigaction is not implemented.\n");
		return 0;
	}
	case SYS_openat: {
		return __syscall6(SYS_openat, a, b, c, d, 0, 0);
	}
#ifdef SYS_timerfd_settime64
	case SYS_timerfd_settime64:
#endif
	case SYS_timerfd_settime:
		return chcore_timerfd_settime(a, b, (struct itimerspec *)c,
					      (struct itimerspec *)d);
	default:
		dead(n);
		// chcore_syscall4(n, a, b, c, d);
	}
}

long __syscall5(long n, long a, long b, long c, long d, long e)
{
	syscall_trace(n);

	switch (n) {
	case SYS_statx: {
		return chcore_statx(a, (const char *)b, c, d, (char *)e);
	}
	case SYS_futex:
		return chcore_futex((int *)a, b, c, (struct timespec *)d,
				    (int *)e, 0);
	case SYS_mremap: {
		warn("SYS_mremap is not implemented.\n");
		/*
		 * At least, our libc can work without mremap
		 * since it will just try to invoke it and
		 * have backup solution.
		 */
		errno = -EINVAL;
		return -1;
	}
	default:
		dead(n);
		return chcore_syscall5(n, a, b, c, d, e);
	}
}

long __syscall6(long n, long a, long b, long c, long d, long e, long f)
{

	ipc_msg_t *ipc_msg = 0;
	struct lwip_request lr = {0};
	struct lwip_request *lr_ptr = 0;
	int ret = 0, fd = 0, cap = 0, len = 0;
	int shared_pmo = 0;

	if (n != SYS_io_submit && n != SYS_read)
		syscall_trace(n);

	switch (n) {

	case SYS_mmap: {
		/* TODO */
		if (e != -1) {
			ipc_msg_t *ipc_msg;
			struct fs_request fr;
			u64 translated_aarray_cap;

			int ret;


			BUG_ON(fd_dic[e] == 0);

			/**
			 * One cap slot number to receive translated_aarray_cap.
			 */
			ipc_msg = ipc_create_msg(fs_ipc_struct,
						 sizeof(struct fs_request), 1);

			fr.req = FS_REQ_FMAP;
			fr.addr = (void *)a;
			fr.count = (size_t)b;
			fr.prot = (int)c;
			fr.flags = (int)d;
			fr.fd = fd_dic[e]->fd;
			fr.offset = (off_t)f;

			ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(fr));

			ret = ipc_call(fs_ipc_struct, ipc_msg);

			BUG_ON(ipc_msg->cap_slot_number_ret <= 0);

			translated_aarray_cap = ipc_get_msg_cap(ipc_msg, 0);
			ipc_destroy_msg(ipc_msg);

			return chcore_syscall6(CHCORE_SYS_handle_fmap, a, b,
					       c, d, translated_aarray_cap, f);
		}
		return chcore_syscall6(CHCORE_SYS_handle_mmap, a, b, c, d, e,
				       f);
	}
	case SYS_fsync: {
		return chcore_fsync(a);
	}
	case SYS_close: {
		return chcore_close(a);
	}
	case SYS_ioctl: {
		return chcore_ioctl(a, b, (void *)c);
	}
	/* Network syscalls */
	case SYS_socket: {
		return chcore_socket(a, b, c);
	}
	case SYS_setsockopt: {
		return chcore_setsocketopt(a, b, c, (const void *)d, e);
	}
	case SYS_getsockopt: {
		return chcore_getsocketopt(a, b, c, (void *)d, (socklen_t *restrict)e);
	}
	case SYS_getsockname: {
		return chcore_getsockname(a, (struct sockaddr *restrict)b, (socklen_t *restrict)c);
	}
	case SYS_getpeername: {
		return chcore_getpeername(a, (struct sockaddr *restrict)b, (socklen_t *restrict)c);
	}
	case SYS_bind: {
		return chcore_bind(a, (const struct sockaddr *)b, (socklen_t)c);
	}
	case SYS_listen: {
		return chcore_listen(a, b);
	}
	case SYS_accept: {
		return chcore_accept(a, (struct sockaddr *restrict)b, (socklen_t *restrict)c);
	}
	case SYS_connect: {
		return chcore_connect(a, (const struct sockaddr *)b, (socklen_t)c);
	}
	case SYS_sendto: {
		return chcore_sendto(a, (const void *)b, c, d, (const struct sockaddr *)e, f);
	}
	case SYS_sendmsg: {
		return chcore_sendmsg(a, (const struct msghdr *)b, c);
	}

	case SYS_recvfrom: {
		return chcore_recvfrom(a, (void *restrict)b, c, d, (struct sockaddr *restrict)e, (socklen_t *restrict)f);
	}
	case SYS_recvmsg: {
		return chcore_recvmsg(a, (struct msghdr *)b, c);
	}
	case SYS_shutdown: {
		return chcore_shutdown(a, b);
	}
#ifdef SYS_open
	case SYS_open:
		return __syscall6(SYS_openat, AT_FDCWD, a, b, c, 0, 0);
#endif
	case SYS_openat: {
		return chcore_openat(a, (const char *)b, c, d);
	}
	case SYS_write: {
		return chcore_write((int)a, (void *)b, (size_t)c);
	}
	case SYS_read: {
		return chcore_read((int)a, (void *)b, (size_t)c);
	}
	case SYS_epoll_create1:
		return chcore_epoll_create1(a);
	case SYS_epoll_ctl:
		return chcore_epoll_ctl(a, b, c, (struct epoll_event *)d);
	case SYS_epoll_pwait:
		return chcore_epoll_pwait(a, (struct epoll_event *)b, c, d,
					  (sigset_t *)e);
#ifdef SYS_poll
	case SYS_poll:
		return chcore_poll((struct pollfd *)a, b, c);
#endif
	case SYS_ppoll:
		return chcore_ppoll((struct pollfd *)a, b, (struct timespec *)c,
				    (sigset_t *)d);
	case SYS_futex:
		return chcore_futex((int *)a, b, c, (struct timespec *)d,
				    (int *)e, f);

	case SYS_readv: {
		return chcore_readv(a, (const struct iovec *)b, c);
	}
	case SYS_writev: {
		return chcore_writev(a, (const struct iovec *)b, c);
	}
#ifdef SYS_clock_nanosleep_time64
	case SYS_clock_nanosleep_time64:
#else
	case SYS_clock_nanosleep:
#endif
		/* Currently ChCore only support CLOCK_MONOTONIC */
		BUG_ON(a != CLOCK_MONOTONIC);
		BUG_ON(b != 0);
		return chcore_syscall4(CHCORE_SYS_clock_nanosleep, a, b, c, d);
	case SYS_nanosleep:
		/*
		 * Though nanosleep uses REALTIME, convert it to MONOTOMIC is
		 * OK since discontinuous changes in should not affect
		 * nanosleep.
		 */
		return chcore_syscall4(CHCORE_SYS_clock_nanosleep,
				       CLOCK_MONOTONIC, 0, a, b);
	default:
		dead(n);
		return chcore_syscall6(n, a, b, c, d, e, f);
	}
}

// TODO (tmac): should we set unimplemented syscall to "dead"?
void dead(long n)
{
	printf("Unsupported syscall %d, bye.\n", n);
	volatile int *addr = (int *)n;
	*addr = 0;
}

/**
 * Generic function to request stat information.
 * `req` controls which input parameters are used.
 * `statbuf` is always the output (struct stat / struct statfs).
 * Requests using this function include SYS_fstat, SYS_fstatat,
 * SYS_statfs, SYS_fstatfs.
 */
static int __xstatxx(int req, int fd, const char *path, int flags,
		     void *statbuf, size_t bufsize)
{
	ipc_msg_t *ipc_msg;
	struct fs_request fr;
	int ret;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr.req = req;
	/* fd could be fd or dirfd, depending on req. */
	fr.fd = fr.dirfd = fd;
	if (path)
		strncpy_safe(fr.path, path, FS_REQ_PATH_LEN);
	fr.flags = flags;
	/* XXX: len should be optimized */
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(fr));
	ret = ipc_call(fs_ipc_struct, ipc_msg);
	if (ret == 0) {
		/* No error */
		memcpy(statbuf, ipc_get_msg_data(ipc_msg), bufsize);
	}
	ipc_destroy_msg(ipc_msg);
	return ret;
}
