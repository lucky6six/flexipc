#include <syscall_arch.h>
#include <string.h>
#include <fcntl.h>

#include <chcore/fs_defs.h>
#include <chcore/ipc.h>

#include "fd.h"

#define debug(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define warn(fmt, ...) printf("[WARN] " fmt, ##__VA_ARGS__)
#define warn_once(fmt, ...) do {  \
	static int __warned = 0;  \
	if (__warned) break;      \
	__warned = 1;             \
	warn(fmt, ##__VA_ARGS__);    \
} while (0)

static inline void strncpy_safe(char *dst, const char *src, size_t dst_size)
{
	strncpy(dst, src, dst_size);
	dst[dst_size - 1] = '\0';
}

int chcore_chdir(const char *path)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_CHDIR;
	fr_ptr->dirfd = AT_FDCWD;
	strncpy_safe(fr_ptr->path, path, FS_REQ_PATH_LEN + 1);

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_fchdir(int fd)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	if (fd_dic[fd] == 0) {
		return -EBADF;
	}

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_FCHDIR;
	fr_ptr->fd = fd_dic[fd]->fd;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int chcore_getcwd(char *buf, size_t size)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	long ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_GETCWD;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	if (!ret) {
		strncpy_safe(buf, ipc_get_msg_data(ipc_msg), size);
		ret = (long)buf;
	}
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int chcore_ftruncate(int fd, off_t length)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	if (fd_dic[fd] == 0) {
		return -EBADF;
	}

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_FTRUNCATE;
	fr_ptr->fd = fd_dic[fd]->fd;
	fr_ptr->len = length;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_lseek(int fd, off_t offset, int whence)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	if (fd_dic[fd] == 0) {
		return -EBADF;
	}

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_LSEEK;
	fr_ptr->fd = fd_dic[fd]->fd;
	fr_ptr->offset = offset;
	fr_ptr->whence = whence;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_MKDIRAT;
	fr_ptr->dirfd = dirfd;
	strncpy_safe(fr_ptr->path, pathname, FS_REQ_PATH_LEN);
	fr_ptr->mode = mode;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_unlinkat(int dirfd, const char *pathname, int flags)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_UNLINKAT;
	fr_ptr->dirfd = dirfd;
	strncpy_safe(fr_ptr->path, pathname, FS_REQ_PATH_LEN);
	fr_ptr->flags = flags;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_symlinkat(const char *target, int newdirfd, const char *linkpath)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_SYMLINKAT;
	strncpy_safe(fr_ptr->path, target, FS_REQ_PATH_LEN);
	fr_ptr->path[FS_REQ_PATH_LEN] = '\0';
	fr_ptr->dirfd = newdirfd;
	strncpy_safe(fr_ptr->path2, linkpath, FS_REQ_PATH_LEN);

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_getdents64(int fd, char *buf, int count)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0, remain = count, cnt;

	if (fd_dic[fd] == 0) {
		return -EBADF;
	}

	BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE);
	ipc_msg = ipc_create_msg(fs_ipc_struct, IPC_SHM_AVAILABLE, 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
	while (remain > 0) {
		fr_ptr->req = FS_REQ_GETDENTS64;
		fr_ptr->fd = fd_dic[fd]->fd;
		cnt = MIN(remain, IPC_SHM_AVAILABLE);
		fr_ptr->count = cnt;
		ret = ipc_call(fs_ipc_struct, ipc_msg);
		if (ret < 0)
			goto error;
		memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
		buf += ret;
		remain -= ret;
		if (ret != cnt)
			break;
	}
	ret = count - remain;
error:
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int chcore_fcntl(int fd, int cmd, int arg)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	if (fd == 0 || fd == 1 || fd == 2) {
		/* Why do we make these fds special? */
		warn("fcntl(%d, 0x%x, 0x%x) on fd 0-2, just return.\n",
		     fd, cmd, arg);
		return 0;
	}

	if (fd_dic[fd] == 0) {
		return -EBADF;
	}


	switch (cmd) {
	case F_DUPFD: {
		int new_fd;

		new_fd = alloc_fd_since(arg);
		fd_dic[new_fd]->cap = fd_dic[fd]->cap;
		fd_dic[new_fd]->fd = fd_dic[fd]->fd;
		fd_dic[new_fd]->type = fd_dic[fd]->type;
		fd_dic[new_fd]->fd_op = fd_dic[fd]->fd_op;

		return new_fd;
	}
	case F_DUPFD_CLOEXEC: {
		ret = chcore_fcntl(fd, F_DUPFD, arg);
		if (ret < 0) // on error
			return ret;
		return chcore_fcntl(ret, F_SETFD, FD_CLOEXEC);
	}
	case F_GETFD: // gets fd flag
		return fd_dic[fd]->flags;
	case F_SETFD: // sets fd flag
		// Note: Posix only defines FD_CLOEXEC as file
		// descriptor flags. However it explicitly allows
		// implementation stores other status flags.
		fd_dic[fd]->flags =
			(arg & FD_CLOEXEC) | (fd_dic[fd]->flags & ~FD_CLOEXEC);
		return 0;
	case F_GETFL:
	case F_SETFL:
		if (fd_dic[fd]->type == FD_TYPE_FILE) {
			ipc_msg = ipc_create_msg(fs_ipc_struct,
						 sizeof(struct fs_request), 0);
			fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

			fr_ptr->req = FS_REQ_FCNTL;
			fr_ptr->fcntl_cmd = cmd;
			fr_ptr->fd = fd_dic[fd]->fd;
			fr_ptr->fcntl_arg = arg;

			ret = ipc_call(fs_ipc_struct, ipc_msg);
			ipc_destroy_msg(ipc_msg);

			return ret;
		}
		warn("does not support F_SETFL for non-fs files\n");
		return -1;
	case F_GETOWN:
		warn("fcntl (F_GETOWN, 0x%x, 0x%x) not implemeted, do "
		     "nothing.\n",
		     cmd, arg);
		break;
	case F_SETOWN:
		warn("fcntl (F_SETOWN, 0x%x, 0x%x) not implemeted, do "
		     "nothing.\n",
		     cmd, arg);
		break;
	case F_GETLK:
		warn("fcntl (F_GETLK, 0x%x, 0x%x) not implemeted, do "
		     "nothing.\n",
		     cmd, arg);
		break;
	case F_SETLK:
		warn("fcntl (F_SETLK, 0x%x, 0x%x) not implemeted, do "
		     "nothing.\n",
		     cmd, arg);
		break;
	case F_SETLKW:
		warn("fcntl (F_SETLKW, 0x%x, 0x%x) not implemeted, do "
		     "nothing.\n",
		     cmd, arg);
		break;

	default:
		return -EINVAL;
	}
	return -1;
}

int chcore_readlinkat(int dirfd, const char *pathname, char *buf,
		      size_t bufsiz)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_READLINKAT;
	fr_ptr->dirfd = dirfd;
	strncpy_safe(fr_ptr->path, pathname, FS_REQ_PATH_LEN);
	fr_ptr->bufsiz = bufsiz;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ret = ret > bufsiz ? bufsiz : ret;
	memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int chcore_renameat(int olddirfd, const char *oldpath,
		    int newdirfd, const char *newpath)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_RENAMEAT;
	fr_ptr->dirfd = olddirfd;
	strncpy((void *)fr_ptr->path, oldpath, FS_REQ_PATH_LEN);
	fr_ptr->path[FS_REQ_PATH_LEN] = '\0';
	fr_ptr->dirfd2 = newdirfd;
	strncpy((void *)fr_ptr->path2, newpath, FS_REQ_PATH_LEN);
	fr_ptr->path[FS_REQ_PATH_LEN] = '\0';

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_faccessat(int dirfd, const char *pathname, int mode, int flags)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_FACCESSAT;
	fr_ptr->dirfd = dirfd;
	strncpy_safe(fr_ptr->path, pathname, FS_REQ_PATH_LEN);
	fr_ptr->amode = mode;
	fr_ptr->flags = flags;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_fallocate(int fd, int mode, off_t offset, off_t len)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	if (fd_dic[fd] == 0) {
		return -EBADF;
	}

	fr_ptr->req = FS_REQ_FALLOCATE;
	fr_ptr->fd = fd_dic[fd]->fd;
	fr_ptr->mode = mode;
	fr_ptr->offset = offset;
	fr_ptr->len = len;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_statx(int dirfd, const char *pathname, int flags,
		 unsigned int mask, char *buf)
{
	ipc_msg_t *ipc_msg = 0;
	struct fs_request *fr_ptr;
	int ret = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_STATX;
	fr_ptr->dirfd = dirfd;
	strncpy_safe(fr_ptr->path, pathname, FS_REQ_PATH_LEN);
	fr_ptr->flags = flags;
	fr_ptr->mask = mask;
	fr_ptr->buff = buf;

	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int chcore_fsync(int fd)
{
	warn_once("SYS_fsync is not implemented.\n");
	return 0;
}

int chcore_openat(int dirfd, const char *pathname, int flags, mode_t mode)
{
	struct fs_request *fr_ptr;
	struct fs_fast_path *fp;
	struct fs_request *ret_fr;
	int ret_cap;
	int ret = 0, fd;
	ipc_msg_t *ipc_msg = 0;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 1);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_OPENAT;
	fr_ptr->dirfd = dirfd;
	strncpy_safe(fr_ptr->path, pathname, FS_REQ_PATH_LEN);
	fr_ptr->flags = flags;
	fr_ptr->mode = mode;

	ret = ipc_call(fs_ipc_struct, ipc_msg);

	if (ret >= 0) {
		fd = alloc_fd();
		fd_dic[fd]->cap = fs_server_cap;
		fd_dic[fd]->fd = ret;
		fd_dic[fd]->type = FD_TYPE_FILE;
		fd_dic[fd]->fd_op = &file_ops;
		ret = fd;

		/* set the fast path */
		ret_fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
		fp = malloc(sizeof(*fp));
		fd_dic[fd]->private_data = (void *)fp;
		fp->fid = ret_fr->fid;
		if (ipc_msg->cap_slot_number_ret == 1)
			ret_cap = ipc_get_msg_cap(ipc_msg, 0);
		else
			ret_cap = ret_fr->cap;
		fp->_fs_ipc_struct = cap2struct(ret_fr->cap, ret_cap);
	}

	ipc_destroy_msg(ipc_msg);
	return ret;
}

static int chcore_file_read(int fd, void *buf, size_t count)
{
	ipc_msg_t *ipc_msg;
	struct fs_request *fr_ptr;
	int ret = 0;
	int remain = count, cnt;
	struct fs_fast_path *fp;

	if (fd_dic[fd] == 0)
		return -EBADF;
	BUG_ON(fd_dic[fd]->private_data == NULL);
	fp = fd_dic[fd]->private_data;
	BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE);
	ipc_msg = ipc_create_msg(fp->_fs_ipc_struct, IPC_SHM_AVAILABLE, 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
	while (remain > 0) {
		fr_ptr->req = FS_REQ_READ_FD;
		fr_ptr->fd = fp->fid;
		cnt = MIN(remain, IPC_SHM_AVAILABLE);
		fr_ptr->count = cnt;
		ret = ipc_call(fp->_fs_ipc_struct, ipc_msg);
		memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
		buf = (char *)buf + ret;
		remain -= ret;
		if (ret != cnt)
			break;
	}
	ret = count - remain;
	ipc_destroy_msg(ipc_msg);
	return ret;
}

static int chcore_file_write(int fd, void *buf, size_t count)
{
	ipc_msg_t *ipc_msg;
	struct fs_request *fr_ptr;
	int ret = 0;
	int remain = count, cnt;
	struct fs_fast_path *fp;

	BUG_ON(fd_dic[fd]->private_data == NULL);
	BUG_ON(sizeof(struct fs_request) > IPC_SHM_AVAILABLE);
	fp = fd_dic[fd]->private_data;
	ipc_msg = ipc_create_msg(fp->_fs_ipc_struct, IPC_SHM_AVAILABLE, 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
	while (remain > 0) {
		fr_ptr->req = FS_REQ_WRITE;
		fr_ptr->fd = fp->fid;
		cnt = MIN(remain, FS_BUF_SIZE);
		fr_ptr->count = cnt;
		ipc_set_msg_data(ipc_msg, buf, sizeof(struct fs_request), cnt);
		ret = ipc_call(fp->_fs_ipc_struct, ipc_msg);
		buf = (char *)buf + ret;
		remain -= ret;
		if (ret != cnt)
			break;
	}
	ret = count - remain;
	ipc_destroy_msg(ipc_msg);
	return ret;
}

static int chcore_file_close(int fd)
{
	ipc_msg_t *ipc_msg;
	struct fs_request *fr_ptr;
	int ret;

	ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

	fr_ptr->req = FS_REQ_CLOSE;
	fr_ptr->fd = fd_dic[fd]->fd;
	ret = ipc_call(fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	if (!ret) {
		if (fd_dic[fd]->private_data)
			free(fd_dic[fd]->private_data);
		free_fd(fd);
	}
	return ret;
}

static int chcore_socket_ioctl(int fd, unsigned long request, void *arg)
{
	WARN("FILE not support ioctl!\n");
	return 0;
}

/* FILE */
struct fd_ops file_ops = {
	.read = chcore_file_read,
	.write = chcore_file_write,
	.close = chcore_file_close,
	.poll = NULL,
};
