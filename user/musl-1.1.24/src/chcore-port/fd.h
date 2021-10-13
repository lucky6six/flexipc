#pragma once

#include "chcore/container/hashtable.h"
#include "chcore/ipc.h"
#include <termios.h>
#include <chcore/lwip_defs.h>

#include "poll.h"

#define MAX_FD  40960
#define MIN_FD  0

/* Type of fd */
enum fd_type {
	FD_TYPE_FILE = 0,
	FD_TYPE_PIPE,
	FD_TYPE_SOCK,
	FD_TYPE_STDIN,
	FD_TYPE_STDOUT,
	FD_TYPE_STDERR,
	FD_TYPE_EVENT,
	FD_TYPE_TIMER,
	FD_TYPE_EPOLL,
};

struct fd_ops {
	int (*read) (int fd, void *buf, size_t count);
	int (*write) (int fd, void *buf, size_t count);
	int (*close) (int fd);
	int (*poll) (int fd, struct pollarg *arg);
	int (*ioctl) (int fd, unsigned long request, void *arg);
};

extern struct fd_ops epoll_ops;
extern struct fd_ops socket_ops;
extern struct fd_ops file_ops;
extern struct fd_ops event_op;
extern struct fd_ops timer_op;
extern struct fd_ops pipe_op;
extern struct fd_ops stdin_ops;
extern struct fd_ops stdout_ops;
extern struct fd_ops stderr_ops;

/*
 * Each fd will have a fd structure `fd_desc` which can be found from
 * the `fd_dic`. `fd_desc` structure contains the basic information of
 * the fd.
 */
struct fd_desc {
	/* Identification used by corresponding service */
	union {
		int conn_id;
		int fd;
	};
	/* Baisc informantion of fd */
	int flags;		/* Flags of the file */
	int cap;		/* Service's cap of fd, 0 if no service */
	enum fd_type type;	/* Type for debug use */
	struct fd_ops *fd_op;
	/* Private data of fd */

	/* stored termios */
	struct termios termios;

	void *private_data;
};

extern struct fd_desc *fd_dic[MAX_FD];
extern struct htable fs_cap_struct_htable;

struct fs_fast_path {
	ipc_struct_t *_fs_ipc_struct;
	int fid;
};

struct fs_cap_struct {
	int fsm_cap;
	ipc_struct_t *_fs_ipc_struct;
	struct hlist_node node;
};

/* fd */
int alloc_fd(void);
int alloc_fd_since(int min);
void free_fd(int fd);

/* fd operation */
int chcore_read(int fd, void *buf, size_t count);
int chcore_write(int fd, void *buf, size_t count);
int chcore_close(int fd);
int chcore_ioctl(int fd, unsigned long request, void *arg);
int chcore_readv(int fd, const struct iovec *iov, int iovcnt);
int chcore_writev(int fd, const struct iovec *iov, int iovcnt);

ipc_struct_t *cap2struct(int key, int cap);
