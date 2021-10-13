#pragma once

#include <sys/types.h>

#define TMPFS_MAP_BUF 1
#define TMPFS_SCAN 2
#define TMPFS_MKDIR 3
#define TMPFS_RMDIR 4
#define TMPFS_CREAT 5
#define TMPFS_UNLINK 6
#define TMPFS_OPEN 7
#define TMPFS_CLOSE 8
#define TMPFS_WRITE 9
#define TMPFS_READ 10

#define TMPFS_GET_SIZE 999

#define AT_FDROOT (-101)

enum FS_REQ {
	FS_REQ_OPEN = 0,
	FS_REQ_CLOSE,

	FS_REQ_CREAT,
	FS_REQ_MKDIRAT,
	FS_REQ_RMDIR,
	FS_REQ_RMDIRAT,
	FS_REQ_SYMLINKAT,
	FS_REQ_UNLINK,
	FS_REQ_UNLINKAT,
	FS_REQ_RENAMEAT,
	FS_REQ_READLINKAT,

	FS_REQ_SCAN,
	FS_REQ_READ,
	FS_REQ_READ_FD,
	FS_REQ_WRITE,
	FS_REQ_WRITEV,

	FS_REQ_OPENAT,
	FS_REQ_STATX,
	FS_REQ_FSTAT,
	FS_REQ_FSTATAT,
	FS_REQ_STATFS,
	FS_REQ_FSTATFS,

	FS_REQ_LSEEK,

	FS_REQ_CHDIR,
	FS_REQ_FCHDIR,

	FS_REQ_GETCWD,
	FS_REQ_GETDENTS64,

	FS_REQ_FTRUNCATE,
	FS_REQ_FALLOCATE,

	FS_REQ_GET_SIZE,

	FS_REQ_FACCESSAT,

	FS_REQ_FCNTL,

	FS_REQ_FMAP, /* The first phase of mmap. */

	FS_REQ_MAX
};

#define FS_REQ_PATH_LEN (255)
struct fs_request {
	enum FS_REQ req;

	char *buff;
	int flags;
	int dirfd;
	int dirfd2; // for rename
	int fd;
	int whence;
	unsigned int mask;
	union { // access() requires int for `amode`
		mode_t mode;
		int amode;
	};
	off_t offset;
	ssize_t count;
	size_t bufsiz;
	off_t len;
	int fcntl_cmd, fcntl_arg;

	/* For mmap(addr, count, prot, flags, fd, offset). */
	void *addr;
	int prot;

	/* == Response == */
	int fid;
	int cap;
	/* Used to walk a path in the filesystem instance. */
	int path_advanced;
	int next_fs_idx;
	/* == End of Response == */

	char path[FS_REQ_PATH_LEN + 1];
	char path2[FS_REQ_PATH_LEN + 1]; // for symlink, rename
};

#define FS_BUF_SIZE (IPC_SHM_AVAILABLE - sizeof(struct fs_request))
