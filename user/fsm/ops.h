#pragma once

#include <assert.h>
#include <chcore/cpio.h>
#include <chcore/defs.h>
#include <chcore/error.h>
#include <chcore/fs_defs.h>
#include <chcore/ipc.h>
#include <chcore/launch_kern.h>
#include <chcore/launcher.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "common.h"
#include "defs.h"

struct fstatxxat_cb_args {
	void *statbuf;
	size_t bufsize;
};

int fsm_scan(int client_id, const char *path, int scan_pmo, int start);
int fsm_read_file(int client_id, const char *path, int read_pmo_cap,
		  off_t offset, size_t count);
int fsm_get_size(int client_id, const char *path);

int __fsm_openat(struct FSRecord *fs, const char *path, int *path_start_p,
		 int flags, mode_t mode, int *next_fs_idx_p, int *fid);

int fsm_openat(int client_id, struct fs_request *ret_fr);
int fsm_close(int client_id, int fd);
int fsm_statx(int dirfd, const char *pathname, int flags, unsigned int mask,
	      void *statxbuf);

int fsm_fstatxx(int client_idx, int req, int fd, void *statbuf, size_t bufsize);
int fsm_fstatxxat_cb(int retcode, struct walk_path *walk, struct fs_request *fr,
		     void *__args);
int fsm_fstatxxat(int client_id, int req, int dirfd, const char *pathname,
		  int flags, void *statbuf, size_t bufsize);

int fsm_lseek(int fd, off_t offset, int whence, ipc_struct_t *ipc_struct);
int fsm_symlinkat(int client_id, const char *path1, int dirfd,
		  const char *path2);
int fsm_readlinkat(int client_id, int dirfd, const char *pathname,
		   size_t bufsiz, ipc_msg_t *libc_ipc_msg);
int fsm_read(int fd, size_t count, ipc_struct_t *ipc_struct,
	     ipc_msg_t *libc_ipc_msg);
int fsm_write(int fd, size_t count, ipc_struct_t *ipc_struct,
	      ipc_msg_t *libc_ipc_msg);

int fsm_mkdirat(int client_id, int dirfd, const char *pathname, mode_t mode);
int fsm_unlinkat(int client_id, int dirfd, const char *pathname, int flags);

int fsm_faccessat(int client_id, int dirfd, const char *pathname, int amode,
		  int flags);
int fsm_chdir(int clinet_id, const char *path);
int fsm_fchdir(struct ClientRecord *client, int fd, ipc_struct_t *ipc_struct,
	       ipc_msg_t *libc_ipc_msg);
int fsm_getcwd(struct ClientRecord *client, ipc_msg_t *libc_ipc_msg);
int fsm_getdents(struct ClientRecord *client, ipc_msg_t *libc_ipc_msg, int fd,
		 unsigned int count);
int fsm_rename(int client_id, int oldfd, const char *old, int newfd,
	       const char *new);
int fsm_ftruncate(int client_id, int fd, off_t length);
int fsm_fallocate(int client_id, int fd, int mode, off_t offset, off_t len);
int fsm_fcntl(int client_id, int fd, int fcntl_cmd, int fcntl_arg);
int fsm_fmap(int client_idx, int fd, size_t count, off_t offset, ipc_msg_t *,
	     bool *ret_with_cap);
