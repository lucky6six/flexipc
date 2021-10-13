#include <chcore/cpio.h>
#include <chcore/defs.h>
#include <chcore/falloc.h>
#include <chcore/fs_defs.h>
#include <chcore/ipc.h>
#include <chcore/launch_kern.h>
#include <chcore/syscall.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mman.h>
#include <time.h>

#include "tmpfs.h"

#define DIV_ROUND_UP(n, d)    (((n) + (d) - 1) / (d))
#define ROUND_UP(x, n)        (((x) + (n) - 1) & ~((n) - 1))

struct inode *tmpfs_root = NULL;
struct dentry *tmpfs_root_dent = NULL;
struct id_manager fidman;
struct fid_record fid_records[MAX_NR_FID_RECORDS];

int tfs_load_image(const char *start)
{
	struct cpio_file *f;
	struct inode *dirat;
	struct dentry *dent;
	const char *leaf;
	size_t len;
	int err;
	ssize_t write_count;
	struct path path;

	BUG_ON(start == NULL);

	cpio_init_g_files();
	cpio_extract(start, "/");

	// TODO:
	// error("mode is not handled in tfs_load_image\n");

	for (f = g_files.head.next; f; f = f->next) {
		if (!(f->header.c_filesize))
			continue;

		leaf = f->name;

		path_init(&path);
		path_append(&path, tmpfs_root_dent);

		err = tfs_namex(&leaf, &path,
				/* mkdir_p */ 1,
				/* follow_symlink */ 1);
		if (err) {
			goto error;
		}

		dirat = path_last_inode(&path);
		assert(dirat->type == FS_DIR);
		/* cpio will include the path in the end, so we should skip the
		 * directories that have been created */
		len = strlen(leaf);
		dent = tfs_lookup(dirat, leaf, len);
		if (dent) {
			path_fini(&path);
			continue;
		}

		err = tfs_creat(dirat, leaf, len, /* mode */ 0, NULL);
		if (err)
			goto error;

		dent = tfs_lookup(dirat, leaf, len);
		BUG_ON(dent == NULL);

		len = f->header.c_filesize;
		write_count = tfs_file_write(dent->inode, 0, f->data, len);
		if (write_count != len) {
			err = -ENOSPC;
			goto error;
		}
	}

	return 0;

error:
	path_fini(&path);
	return err;
}

static int dirent_filler(void **dirpp, void *end, char *name, off_t off,
			 unsigned char type, ino_t ino)
{
	struct dirent *dirp = *(struct dirent **)dirpp;
	void *p = dirp;
	unsigned short len = strlen(name) + 1 + sizeof(dirp->d_ino) +
			     sizeof(dirp->d_off) + sizeof(dirp->d_reclen) +
			     sizeof(dirp->d_type);
	p += len;
	if (p > end)
		return -EAGAIN;
	dirp->d_ino = ino;
	dirp->d_off = off;
	dirp->d_reclen = len;
	dirp->d_type = type;
	strcpy(dirp->d_name, name);
	*dirpp = p;
	return len;
}

static int tfs_scan(struct inode *dir, unsigned int start, void *buf, void *end,
		    int *read_bytes)
{
	s64 cnt = 0;
	int b;
	int ret;
	ino_t ino;
	void *p = buf;
	unsigned char type;
	struct dentry *iter;

	for_each_in_htable(iter, b, node, &dir->dentries)
	{
		if (cnt >= start) {
			/* TODO(YJF): use the correct ino and type */
			type = iter->inode->type;
			ino = iter->inode->size;
			ret = dirent_filler(&p, end, iter->name.str, cnt, type,
					    ino);
			if (ret <= 0) {
				if (read_bytes)
					*read_bytes = (int)(p - buf);
				return cnt - start;
			}
		}
		cnt++;
	}
	if (read_bytes)
		*read_bytes = (int)(p - buf);
	return cnt - start;
}

int fs_creat(const char *path, mode_t mode)
{
	struct inode *dirat = NULL;
	struct path path_record;
	const char *leaf = path;
	int err;

	BUG_ON(!path);
	BUG_ON(*path != '/');
	WARN_ON(mode, "mode is ignored by fs_creat");

	path_init(&path_record);
	path_append(&path_record, tmpfs_root_dent);

	err = tfs_namex(&leaf, &path_record,
			/* mkdir_p */ 0,
			/* follow_symlink */ 1);
	if (err)
		goto error;

	dirat = path_last_inode(&path_record);
	err = tfs_creat(dirat, leaf, strlen(leaf),
			mode, NULL);
	if (err)
		goto error;

	// FIXME(TCZ): add fd entry to a process' open file table and return fd
	// TODO(TCZ): change to the following call as per POSIX
	// open (filename, O_WRONLY | O_CREAT | O_TRUNC, mode)
error:
	path_fini(&path_record);
	return err;
}

int fs_unlink(const char *path)
{
	struct inode *dirat = NULL;
	struct path path_record;
	const char *leaf = path;
	int err;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	path_init(&path_record);
	path_append(&path_record, tmpfs_root_dent);

	err = tfs_namex(&leaf, &path_record,
			/* mkdir_p */ 0,
			/* follow_symlink */ 1);
	if (err)
		goto error;

	dirat = path_last_inode(&path_record);
	err = tfs_remove(dirat, leaf, strlen(leaf));

error:
	path_fini(&path_record);
	return err;
}

int fs_rmdir(const char *path)
{
	struct inode *dirat = NULL;
	struct path path_record;
	const char *leaf = path;
	int err;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	path_init(&path_record);
	path_append(&path_record, tmpfs_root_dent);

	err = tfs_namex(&leaf, &path_record,
			/* mkdir_p */ 0,
			/* follow_symlink */ 1);
	if (err)
		goto error;

	dirat = path_last_inode(&path_record);
	err = tfs_remove(dirat, leaf, strlen(leaf));

error:
	path_fini(&path_record);
	return err;
}

/* path[0] must be '/' */
static struct inode *open_path(const char *path)
{
	struct inode *dirat = NULL;
	struct path path_record;
	const char *leaf = path;
	struct dentry *dent;
	int err;

	// printk("path = %s\n", path);
	if (*path == '/' && !*(path + 1))
		return get_inode(tmpfs_root);

	path_init(&path_record);
	path_append(&path_record, tmpfs_root_dent);

	assert(*path == '/');
	leaf++;

	err = tfs_namex(&leaf, &path_record,
			/* mkdir_p */ 0,
			/* follow_symlink */ 1);
	if (err) {
		path_fini(&path_record);
		return NULL;
	}

	dirat = path_last_inode(&path_record);
	dent = tfs_lookup(dirat, leaf, strlen(leaf));

	path_fini(&path_record);
	return dent ? get_inode(dent->inode) : NULL;
}

/* use absolute path, offset and count to read directly
 * FIXME(YJF): check permission
 */
int fs_read(const char *path, off_t offset, void *buf, size_t count)
{
	struct inode *inode;
	int ret = -ENOENT;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	inode = open_path(path);
	if (inode)
		ret = tfs_file_read(inode, offset, buf, count);
	return ret;
}

int tmpfs_read(int fd, size_t count, ipc_msg_t *ipc_msg)
{
	struct inode *inode = fid_records[fd].inode;
	int ret = 0;
	if (inode)
		ret = tfs_file_read(inode, fid_records[fd].offset,
				    ipc_get_msg_data(ipc_msg), count);
	fid_records[fd].offset += ret;
	return ret;
}

int tmpfs_write(int fd, char *buf, size_t count)
{
	struct inode *inode = fid_records[fd].inode;
	int ret = 0;
	if (inode)
		ret = tfs_file_write(inode, fid_records[fd].offset, buf, count);
	fid_records[fd].offset += ret;
	return ret;
}

off_t tmpfs_lseek(int fd, off_t offset, int whence)
{
	off_t ret = 0;
	switch (whence) {
	case SEEK_SET: {
		fid_records[fd].offset = offset;
		break;
	}
	case SEEK_CUR: {
		fid_records[fd].offset += offset;
		break;
	}
	case SEEK_END: {
		fid_records[fd].offset = fid_records[fd].inode->size + offset;
		break;
	}
	default: {
		error("%s: %d Not impelemented yet\n", __func__, whence);
		usys_exit(-1);
		break;
	}
	}

	if (!ret)
		ret = fid_records[fd].offset;
	return ret;
}

int fs_chdir(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int dirfd = fr->dirfd;
	const char *path = fr->path;
	struct tmpfs_walk_path walk;
	int err;

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = NULL;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	/* Found the target */
	/* walk->parent is the parent, walk->target is the target file inode. */
	if (!walk.target)
		err = -ENOENT;
	else if (walk.target->type != FS_DIR)
		err = -ENOTDIR;
	else {
		/* Succeed. */
		int fid;

		path_append(&walk.path_record, walk.target_dent);

		/* Build a fileid and record it. */
		fid = new_fid_record(walk.target);
		fid_records[fid].flags = 0;
		fid_records[fid].offset = 0;
		fid_records[fid].path = walk.path_record;

		return fid;
	}

error:
	path_fini(&walk.path_record);
	return err;
}

int fs_fchdir(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	if (fid_records[fr->fd].inode->type == FS_DIR)
		return 0;
	else
		return -ENOTDIR;
}

#define MAX_PATH_LEN 255
int fs_getcwd(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int err;
	char buf[MAX_PATH_LEN + 1];

	debug("getcwd fid=%d\n", fr->fd);

	if (fid_records[fr->fd].inode->type != FS_DIR)
		return -ENOTDIR;

	err = path_to_str(&fid_records[fr->fd].path, buf,
			  MAX_PATH_LEN, /* expand_symlink? */ false);
	buf[MAX_PATH_LEN] = '\0';
	debug("cwd=%s\n", buf);
	if (err)
		return err;

	err = ipc_set_msg_data(ipc_msg, buf, 0, strlen(buf) + 1);
	return err;
}

int fs_unlinkat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int dirfd = fr->fd;
	const char *path = fr->path;
	int flags = fr->flags;
	struct tmpfs_walk_path walk;
	int err;

	if (flags & (~AT_SYMLINK_NOFOLLOW)) {
		/* flags contain invalid flag bits. */
		return -EINVAL;
	}

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = NULL;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	/* Found the target */
	/* walk->parent is the parent, walk->target is the target file inode. */
	if (fr->req == FS_REQ_UNLINKAT && walk.target->type == FS_DIR) {
		err = -EISDIR;
		goto error;
	} else if (fr->req == FS_REQ_RMDIRAT && walk.target->type != FS_DIR) {
		err = -ENOTDIR;
		goto error;
	}

	err = tfs_remove(path_last_inode(&walk.path_record),
			 walk.leaf, strlen(walk.leaf));
	if (err)
		goto error;

	err = 0;

error:

	path_fini(&walk.path_record);
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

int fs_mkdirat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int dirfd = fr->fd;
	const char *path = fr->path;
	int mode = fr->mode;
	struct tmpfs_walk_path walk;
	struct inode *parent;
	int err;

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = NULL;

	debug("path=%s\n", path);

	err = handle_xxxat(dirfd, &walk);
	if (err && err != -ENOENT)
		goto error;

	parent = path_last_inode(&walk.path_record);
	debug("parent=%p leaf=%s\n", parent, walk.leaf);
	/**
	 * Found the target.
	 * The last component in path is the parent,
	 * walk->target is the target file inode.
	 */

	err = tfs_mkdir(parent, walk.leaf, strlen(walk.leaf), mode, NULL);
	if (err)
		goto error;

	err = 0;

error:
	path_fini(&walk.path_record);

	debug("err=%d\n", err);
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

static void fill_statbuf(struct stat *statbuf, struct inode *inode)
{
	assert(inode);
	statbuf->st_dev = 0;
	/* TODO(MK): We should use a UUID for inode. */
	statbuf->st_ino = (ino_t)inode;

	if (inode->type == FS_REG)
		statbuf->st_mode = S_IFREG;
	else if (inode->type == FS_DIR)
		statbuf->st_mode = S_IFDIR;
	else {
		assert(0);
	}
	/* TODO(MK): Hard link. */
	statbuf->st_nlink = 1;
	/* TODO(MK): uid/gid. */
	statbuf->st_uid = 0;
	statbuf->st_gid = 0;

	statbuf->st_rdev = 0;
	statbuf->st_size = inode->size;

	/* TODO(MK): Use real xtime. */
	statbuf->st_atime = 0;
	statbuf->st_mtime = 0;
	statbuf->st_ctime = 0;

	/* TODO(MK): Handle the following two. */
	statbuf->st_blksize = 0;
	statbuf->st_blocks = 0;
}

int fs_fstatat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	// (int dirfd, const char *path, struct stat *stat, int flags)
	int dirfd = fr->fd;
	const char *path = fr->path;
	struct stat stat;
	int flags = fr->flags;
	struct tmpfs_walk_path walk;
	int err;

	if (flags & (~AT_SYMLINK_NOFOLLOW)) {
		/* flags contain invalid flag bits. */
		return -EINVAL;
	}

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = NULL;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	/* Found the target */
	/* walk->parent is the parent, walk->target is the target file inode. */
	fill_statbuf(&stat, walk.target);

	err = ipc_set_msg_data(ipc_msg, &stat, 0, sizeof(stat));


	/* FIXME(MK): We should use new error codes for IPC. */
	if (err) {
		err = -ENOSPC;
		goto error;
	}

	err = 0;

error:

	path_fini(&walk.path_record);
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

int fs_fstat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	struct fid_record *fid_record;
	struct stat stat;
	int err;

	fid_record = get_fid_record(fr->fd);
	assert(fid_record);
	fill_statbuf(&stat, fid_record->inode);

	err = ipc_set_msg_data(ipc_msg, &stat, 0, sizeof(stat));
	/* FIXME(MK): We should use new error codes for IPC. */
	if (err)
		return -ENOSPC;
	return 0;
}

static void fill_statfsbuf(struct statfs *statbuf)
{
	const int faked_large_value = 1024 * 1024 * 512;
	/* Type of filesystem (see below): ChCorE7mpf5 */
	statbuf->f_type = 0xCCE7A9F5;
	/* Optimal transfer block size */
	statbuf->f_bsize = PAGE_SIZE;
	/* Total data blocks in filesystem */
	statbuf->f_blocks = faked_large_value;
	/* Free blocks in filesystem */
	statbuf->f_bfree = faked_large_value;
	/* Free blocks available to unprivileged user */
	statbuf->f_bavail = faked_large_value;
	/* Total file nodes in filesystem */
	statbuf->f_files = faked_large_value;
	/* Free file nodes in filesystem */
	statbuf->f_ffree = faked_large_value;
	/* Filesystem ID (See man page) */
	memset(&statbuf->f_fsid, 0, sizeof(statbuf->f_fsid));
	/* Maximum length of filenames */
	statbuf->f_namelen = MAX_FILENAME_LEN;
	/* Fragment size (since Linux 2.6) */
	statbuf->f_frsize = 512; /* XXX(MK): the value? */
	/* Mount flags of filesystem (since Linux 2.6.36) */
	statbuf->f_flags = 0;
}

int fs_fstatfs(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	struct fid_record *fid_record;
	struct statfs stat;
	int err;

	/* TODO(MK): Check fd validity. */
	fid_record = get_fid_record(fr->fd);
	assert(fid_record);
	fill_statfsbuf(&stat);

	err = ipc_set_msg_data(ipc_msg, &stat, 0, sizeof(stat));
	/* FIXME(MK): We should use new error codes for IPC. */
	if (err)
		return -ENOSPC;
	return 0;
}

int fs_fstatfsat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	// (int dirfd, const char *path, struct stat *stat)
	int dirfd = fr->fd;
	const char *path = fr->path;
	struct statfs stat;
	struct tmpfs_walk_path walk;
	int err;

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = NULL;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	/* Found the target */
	/* walk->parent is the parent, walk->target is the target file inode. */
	fill_statfsbuf(&stat);

	err = ipc_set_msg_data(ipc_msg, &stat, 0, sizeof(stat));
	/* FIXME(MK): We should use new error codes for IPC. */
	if (err)
		return -ENOSPC;
	return 0;
error:
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

int fs_faccessat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	// (int dirfd, const char *path, struct stat *stat)
	int dirfd = fr->fd;
	const char *path = fr->path;
	int amode = fr->amode;
	int flags = fr->flags;
	struct tmpfs_walk_path walk;
	int err;

	if (flags & (~AT_EACCESS)) {
		/* flags contain invalid flag bits. */
		return -EINVAL;
	}

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = NULL;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	/* Found the target */
	/* walk->parent is the parent, walk->target is the target file inode. */
	// TODO(TCZ): check walk->target->mode with uid & gid
	// Note: F_OK requires no additional check, since handle_xxxat can
	// return -ENOENT when file is not found
	if (amode & R_OK) {
	}
	if (amode & W_OK) {
	}
	if (amode & X_OK) {
	}
	error("fs_faccessat always return 0 when the file exists\n");

	return 0;
error:
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

int fs_symlinkat_callback(int retcode, struct tmpfs_walk_path *walk, void *data)
{
	return tfs_mknod(path_last_inode(&walk->path_record),
			 walk->leaf, strlen(walk->leaf), FS_SYM,
			 data, &(walk->target_dent));
}

int fs_symlinkat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int dirfd = fr->fd;
	const char *path = fr->path;
	struct tmpfs_walk_path walk;
	int err;

	walk.path = path;
	walk.path_len = strlen(path);
	walk.not_found_callback = fs_symlinkat_callback;
	walk.callback_data = fr->path2;

	err = handle_xxxat(dirfd, &walk);

	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

int fs_readlinkat(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int dirfd = fr->fd;
	const char *path = fr->path;
	struct tmpfs_walk_path walk;
	int size;
	int err;

	walk.path = path;
	walk.path_len = strlen(path);
	walk.follow_symlink = 0;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	assert(walk.target->type == FS_SYM);
	size = tfs_file_read(walk.target, 0, fr->path2, FS_REQ_PATH_LEN);
	if (size < 0)
		goto error;

	err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
	/* FIXME(MK): We should use new error codes for IPC. */
	if (err)
		goto error;

	return size;

error:
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

ssize_t fs_get_size(const char *path)
{
	struct inode *inode;
	int ret = -ENOENT;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	inode = open_path(path);
	if (inode)
		ret = inode->size;
	return ret;
}

/* use absolute path, offset and count to write directly
 * FIXME(YJF): check permission*/
int fs_write(const char *path, off_t offset, const void *buf, size_t count)
{
	struct inode *inode;
	int ret = -ENOENT;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	inode = open_path(path);
	if (inode)
		ret = tfs_file_write(inode, offset, buf, count);
	return ret;
}

int fs_openat_callback(int retcode, struct tmpfs_walk_path *walk, void *data)
{
	int flags = ((struct openat_callback_args *)data)->flags;
	mode_t mode = ((struct openat_callback_args *)data)->mode;
	if (flags & O_CREAT) {
		return tfs_creat(path_last_inode(&walk->path_record),
				 walk->leaf, strlen(walk->leaf),
				 mode, &(walk->target_dent));
	}
	return retcode;
}

int fs_open(ipc_msg_t *ipc_msg, struct fs_request *fr)
{
	int dirfd = fr->fd;
	const char *path = fr->path;
	int flags = fr->flags;
	mode_t mode = fr->mode;
	struct tmpfs_walk_path walk;
	struct openat_callback_args callback_data;
	int fid;
	int err;

	debug("open path=%s mode=0x%x flags=0x%x\n", path, mode, flags);

	walk.path = path;
	walk.path_len = strlen(path);

	callback_data.flags = flags;
	callback_data.mode = mode;
	walk.not_found_callback = fs_openat_callback;
	walk.callback_data = &callback_data;
	walk.follow_symlink = flags & O_NOFOLLOW ? 0 : 1;

	err = handle_xxxat(dirfd, &walk);
	if (err)
		goto error;

	if (walk.target && (flags & O_CREAT) && (flags & O_EXCL)) {
		err = -EEXIST;
		goto error;
	}

	if ((flags & O_DIRECTORY) && walk.target->type != FS_DIR) {
		err = -ENOTDIR;
		goto error;
	}

	if ((flags & (O_RDWR | O_WRONLY)) && walk.target->type == FS_DIR) {
		err = -EISDIR;
		goto error;
	}

	if (flags & O_NOCTTY) {
		// TODO(TCZ): O_NOCTTY
		error("O_NOCTTY at open is not supported\n");
	}

	if (!(flags & (O_RDWR | O_WRONLY)) && (flags & (O_TRUNC | O_APPEND))) {
		err = -EACCES;
		goto error;
	}

	// TODO(TCZ): check file permission with flag, return EACCES on mismatch
	error("open does not check file permission\n");

	/* Build a fileid and record it. */
	fid = new_fid_record(walk.target);
	fid_records[fid].flags = flags;
	fid_records[fid].offset = 0;

	fr->fid = fid;

	if (flags & O_TRUNC && walk.target->type == FS_REG) {
		tfs_truncate(walk.target, 0);
	}

	if (flags & O_APPEND && walk.target->type == FS_REG) {
		fid_records[fid].offset = walk.target->size;
		error("atomicity is not provide, when O_APPEND is set\n");
	}

	return 0;
error:
	if (err == -EAGAIN) {
		fr->next_fs_idx = walk.next_fs_idx;
		fr->path_advanced = walk.path_advanced;
		err = ipc_set_msg_data(ipc_msg, fr, 0, sizeof(*fr));
		/* FIXME(MK): We should use new error codes for IPC. */
		if (err)
			return -ENOSPC;
		return -EAGAIN;
	}
	return err;
}

int fs_close(int fd)
{
	int err;
	/* Del fd_record. */
	err = del_fid_record(fd);
	return err;
}

/* Scan several dirent structures from the directory referred to by the path
 * into the buffer pointed by dirp. The argument count specifies the size of
 * that buffer.
 *
 * RETURN VALUE
 * On success, the number of items is returned. On end of directory, 0 is
 * returned. On error, -errno is returned.
 *
 * The caller should call this function over and over again until it returns 0
 * */
int fs_scan(const char *path, unsigned int start, void *buf, unsigned int count)
{
	struct inode *inode;

	BUG_ON(!path);
	BUG_ON(*path != '/');

	inode = open_path(path);
	if (inode) {
		if (inode->type == FS_DIR)
			return tfs_scan(inode, start, buf, buf + count, NULL);
		return -ENOTDIR;
	}
	return -ENOENT;
}

int fs_getdents(int fd, unsigned int count, ipc_msg_t *ipc_msg)
{
	struct inode *inode = fid_records[fd].inode;
	int ret = 0, read_bytes;
	if (inode) {
		if (inode->type == FS_DIR) {
			ret = tfs_scan(inode, fid_records[fd].offset,
				       ipc_get_msg_data(ipc_msg),
				       ipc_get_msg_data(ipc_msg) + count,
				       &read_bytes);
			fid_records[fd].offset += ret;
			ret = read_bytes;
		} else
			ret = -ENOTDIR;
	} else
		ret = -ENOENT;
	return ret;
}

// TODO: check . and .. in the final component
// TODO: check old is not a ancestor of new
int fs_rename(struct fs_request *fr)
{
	struct tmpfs_walk_path walk_old, walk_new;
	struct inode *inode_to_remove = NULL;
	struct dentry *dent_old, *dent_new /* of new path */, *tmp_dent;
	int new_exists, err;

	walk_old.path = fr->path;
	walk_old.path_len = strlen(fr->path);
	walk_old.not_found_callback = NULL;
	walk_old.follow_symlink = 0;
	if ((err = handle_xxxat(fr->dirfd, &walk_old)))
		return err;
	dent_old = *walk_old.leaf == '\0' ?
		path_last_dent(&walk_old.path_record) : walk_old.target_dent;

	/* FIXME(MK): Shall we move this forward? */
	path_fini(&walk_old.path_record);

	walk_new.path = fr->path2;
	walk_new.path_len = strlen(fr->path2);
	walk_new.not_found_callback = NULL;
	walk_new.follow_symlink = 0;
	err = handle_xxxat(fr->dirfd2, &walk_new);
	if (err != -ENOENT) {
		path_fini(&walk_new.path_record);
		return err;
	}
	new_exists = err != -ENOENT;

	if (new_exists) {
		dent_new = *walk_new.leaf == '\0' ?
			path_last_dent(&walk_new.path_record)
			: walk_new.target_dent;

		/* FIXME(MK): Shall we move this forward? */
		path_fini(&walk_new.path_record);

		// same dentry or inode, then do nothing
		if (dent_old == dent_new ||
		    dent_old->inode == dent_new->inode) {
			return 0;
		}

		// if old is dir, new should be dir;
		// if old is not a dir, new should not be a dir
		if ((dent_old->inode->type == FS_DIR &&
		     dent_new->inode->type != FS_DIR) ||
		    (dent_old->inode->type != FS_DIR &&
		     dent_new->inode->type == FS_DIR)) {
			return -ENOTDIR;
		}

		// if new is dir, it should be empty
		if (dent_new->inode->type == FS_DIR &&
		    !htable_empty(&dent_new->inode->dentries))
			return -ENOTEMPTY;

		inode_to_remove = dent_new->inode;
		dent_new->inode = dent_old->inode;
	} else {
		tmp_dent = new_dent(dent_old->inode, walk_new.leaf,
				    strlen(walk_new.leaf));
		if (CHCORE_IS_ERR(tmp_dent)) {
			return CHCORE_PTR_ERR(tmp_dent);
		}

		htable_add(&path_last_inode(&walk_new.path_record)->dentries,
			   (u32)(tmp_dent->name.hash), &tmp_dent->node);
		path_fini(&walk_new.path_record);
	}

	if (inode_to_remove) {
		if ((err = put_inode(inode_to_remove))) {
			return err;
		}
	}

	if ((err = put_inode(dent_old->inode))) {
		return err;
	}
	// remove dentry from parent
	htable_del(&dent_old->node);
	// free dentry
	free(dent_old);

	return 0;
}


struct __fmap_radix_scan_cb_args {
	vaddr_t *buffer;
	size_t capacity; /* Buffer capacity in entries. */
	size_t nr_filled;
};

int __fmap_radix_scan_cb(void *data, void *privdata)
{
	struct __fmap_radix_scan_cb_args *args = privdata;

	if (args->nr_filled >= args->capacity)
		return -ENOMEM;

	args->buffer[args->nr_filled++] = (vaddr_t)data;

	return 0;
}

int fs_ftruncate(struct fs_request *fr)
{
	int fd = fr->fd;
	off_t len = fr->len;
	struct inode *inode = fid_records[fd].inode;

	/* The argument len is negative or larger than the maximum file size */
	if (len < 0)
		return -EINVAL;
	/* The named file is a directory */
	if (inode->type == FS_DIR)
		return -EISDIR;
	/* fd does not reference a regular file */
	if (inode->type != FS_REG)
		return -EINVAL;

	return tfs_truncate(inode, len);
}

static int fs_punch_hole(struct inode *inode, off_t offset, off_t len)
{
	u64 page_no, page_off;
	u64 cur_off = offset;
	size_t to_remove;
	void *page;
	int err;

	while (len > 0) {
		page_no = cur_off / PAGE_SIZE;
		page_off = cur_off % PAGE_SIZE;

		to_remove = MIN(len, PAGE_SIZE - page_off);
		cur_off += to_remove;
		len -= to_remove;
		page = radix_get(&inode->data, page_no);
		if (page) {
			if (to_remove == PAGE_SIZE || cur_off == inode->size)
				err = radix_del(&inode->data, page_no, 1);
			else
				memset(page + page_off, 0, to_remove);

			if (err)
				return err;
		}
	}
	return 0;
}

static int fs_collapse_range(struct inode *inode, off_t offset, off_t len)
{
	u64 page_no1, page_no2;
	u64 cur_off = offset;
	void *page1;
	void *page2;
	u64 remain;
	int err, dist;

	/* To ensure efficient implementation, offset and len must be a mutiple
	 * of the filesystem logical block size */
	if (offset % PAGE_SIZE || len % PAGE_SIZE)
		return -EINVAL;
	if (offset + len >= inode->size)
		return -EINVAL;

	remain = ((inode->size + PAGE_SIZE - 1) - (offset + len)) / PAGE_SIZE;
	dist = len / PAGE_SIZE;
	while (remain-- > 0) {
		page_no1 = cur_off / PAGE_SIZE;
		page_no2 = page_no1 + dist;

		cur_off += PAGE_SIZE;
		page1 = radix_get(&inode->data, page_no1);
		page2 = radix_get(&inode->data, page_no2);
		if (page1) {
			err = radix_del(&inode->data, page_no1, 1);
			if (err)
				goto error;
		}
		if (page2) {
			err = radix_add(&inode->data, page_no1, page2);
			if (err)
				goto error;
			err = radix_del(&inode->data, page_no2, 0);
			if (err)
				goto error;
		}
	}

	inode->size -= len;
	return 0;

error:
	error("Error in collapse range!\n");
	return err;
}

static int fs_zero_range(struct inode *inode, off_t offset, off_t len, int mode)
{
	u64 page_no, page_off;
	u64 cur_off = offset;
	size_t to_zero;
	void *page;

	while (len > 0) {
		page_no = cur_off / PAGE_SIZE;
		page_off = cur_off % PAGE_SIZE;

		to_zero = MIN(len, PAGE_SIZE - page_off);
		cur_off += to_zero;
		len -= to_zero;
		if (!len)
			to_zero = PAGE_SIZE;
		page = radix_get(&inode->data, page_no);
		if (!page) {
			page = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
			if (!page)
				return -ENOSPC;
			radix_add(&inode->data, page_no, page);
		}
		BUG_ON(!page);
		memset(page + page_off, 0, to_zero);
	}

	if ((!(mode & FALLOC_FL_KEEP_SIZE)) && (offset + len > inode->size))
		inode->size = offset + len;

	return 0;
}

static int fs_insert_range(struct inode *inode, off_t offset, off_t len)
{
	u64 page_no1, page_no2;
	void *page;
	int err, dist;

	/* To ensure efficient implementation, this mode has the same
	 * limitations as FALLOC_FL_COLLAPSE_RANGE regarding the granularity of
	 * the operation. (offset and len must be a mutiple of the filesystem
	 * logical block size) */
	if (offset % PAGE_SIZE || len % PAGE_SIZE)
		return -EINVAL;
	/* If the offset is equal to or greater than the EOF, an error is
	 * returned. For such operations, ftruncate should be used. */
	if (offset >= inode->size)
		return -EINVAL;

	page_no1 = (inode->size + PAGE_SIZE - 1) / PAGE_SIZE;
	dist = len / PAGE_SIZE;
	while (page_no1 >= offset / PAGE_SIZE) {
		page_no2 = page_no1 + dist;
		BUG_ON(radix_get(&inode->data, page_no2));
		page = radix_get(&inode->data, page_no1);
		if (page) {
			err = radix_del(&inode->data, page_no1, 0);
			if (err)
				goto error;
			err = radix_add(&inode->data, page_no2, page);
			if (err)
				goto error;
		}
		page_no1--;
	}

	inode->size += len;
	return 0;

error:
	error("Error in insert range!\n");
	return err;
}

int fs_fallocate(struct fs_request *fr)
{
	int fd = fr->fd;
	int mode = fr->mode;
	off_t offset = fr->offset;
	off_t len = fr->len;
	int keep_size;
	struct inode *inode = fid_records[fd].inode;

	if (offset < 0 || len <= 0)
		return -EINVAL;

	/* return error if mode is not supported */
	if (mode & ~(FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE |
		     FALLOC_FL_COLLAPSE_RANGE | FALLOC_FL_ZERO_RANGE |
		     FALLOC_FL_INSERT_RANGE))
		return -EOPNOTSUPP;

	if (mode & FALLOC_FL_PUNCH_HOLE)
		return fs_punch_hole(inode, offset, len);
	if (mode & FALLOC_FL_COLLAPSE_RANGE)
		return fs_collapse_range(inode, offset, len);
	if (mode & FALLOC_FL_ZERO_RANGE)
		return fs_zero_range(inode, offset, len, mode);
	if (mode & FALLOC_FL_INSERT_RANGE)
		return fs_insert_range(inode, offset, len);

	keep_size = mode & FALLOC_FL_KEEP_SIZE ? 1 : 0;
	return tfs_allocate(inode, offset, len, keep_size);
}

int fs_fcntl(struct fs_request *fr)
{
	struct fid_record *fid;
	int ret = 0;

	if ((fid = get_fid_record(fr->fd)) == NULL)
		return -EBADF;

	switch (fr->fcntl_cmd) {
	case F_GETFL:
		ret = fid->flags;
		break;
	case F_SETFL: {
		// file access mode and the file creation flags
		// should be ingored, per POSIX
		int effective_bits = (~O_ACCMODE & ~O_CLOEXEC & ~O_CREAT &
				      ~O_DIRECTORY & ~O_EXCL & ~O_NOCTTY &
				      ~O_NOFOLLOW & ~O_TRUNC & ~O_TTY_INIT);

		fid->flags = (fr->fcntl_arg & effective_bits) |
			     (fid->flags & ~effective_bits);
		break;
	}
	case F_DUPFD:
	case F_GETOWN:
	case F_SETOWN:
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
	default:
		error("unsopported fcntl cmd\n");
		ret = -1;
		break;
	}

	return ret;
}

int fs_fmap(ipc_msg_t *ipc_msg, struct fs_request *fr, bool *ret_with_cap)
{
	struct __fmap_radix_scan_cb_args args;
	struct fid_record *fid_record;
	struct inode *inode;
	size_t nr_file_pages;
	int err;

	/* TODO(MK): Check fd validity. */
	fid_record = get_fid_record(fr->fd);
	assert(fid_record);

	inode = fid_record->inode;
	assert(inode);

	if (inode->type != FS_REG)
		return -EINVAL;

	nr_file_pages = DIV_ROUND_UP(inode->size, PAGE_SIZE);


	if (!inode->aarray.valid) {

		size_t size;
		vaddr_t *array;

		size = ROUND_UP(nr_file_pages * sizeof(array[0]), PAGE_SIZE);

		array = malloc(size);
		assert(array);

		args.buffer = array;
		args.capacity = size / sizeof(array[0]);
		args.nr_filled = 0;

		/* FIXME(MK): do not fill all pages. */
		err = radix_scan(&inode->data, 0, __fmap_radix_scan_cb, &args);
		assert(!err);

		inode->aarray.size = size;
		inode->aarray.array = array;
		inode->aarray.nr_used = args.nr_filled;


		/**
		 * The following syscall translates addresses in aarray to a
		 * newly created pmo memory. The returned pmo should not be
		 * accessible in user space. We should pass this to the
		 * application who will use it to finish the fmap syscall.
		 */
		inode->aarray.translated_pmo_cap = 0;
		err = usys_translate_aarray(array, size, 0, args.nr_filled,
					    &inode->aarray.translated_pmo_cap);

		if (err)
			return err;

		inode->aarray.valid = true;

	} else
	/* array is valid */
	if (nr_file_pages < inode->aarray.nr_used) {


		/* FIXME(MK): handle file size changes. */
		warn("%s: file shrinked after first mmap,"
		     " something may go wrong\n",
		     __func__);
	} else if (nr_file_pages > inode->aarray.nr_used) {

		size_t nr_added;

		/* Prepare args for radix scan. */
		args.buffer = inode->aarray.array;
		args.capacity = inode->aarray.size / sizeof(args.buffer[0]);
		args.nr_filled = inode->aarray.nr_used;

		/**
		 * Call radix scan, which fills in the inode->aaray.array
		 * buffer.
		 */
		err = radix_scan(&inode->data, args.nr_filled,
				 __fmap_radix_scan_cb, &args);
		assert(!err);

		/* Reflect the new entries in kernel aarray pmo. */
		nr_added = args.nr_filled - inode->aarray.nr_used;

		/* Update number of valid entries in aarray. */
		inode->aarray.nr_used = args.nr_filled;

		err = usys_translate_aarray(inode->aarray.array,
					    inode->aarray.size,
					    inode->aarray.nr_used,
					    nr_added,
					    &inode->aarray.translated_pmo_cap);
		assert(!err);

		debug("aarray increased from %d to %d\n",
		      inode->aarray.nr_used - nr_added,
		      inode->aarray.nr_used);

	}

	/**
	 * NOTE(MK): It's okay if fr->offset + fr->count > file size.
	 * In that case, the mapping is created, but access bytes beyond file
	 * size will cause SIGBUS.
	 */
	// assert(fr->offset + fr->count <= inode->aarray.nr_used * PAGE_SIZE);

	/* Returns the pmo of the address array. */
	ipc_set_ret_msg_cap(ipc_msg, inode->aarray.translated_pmo_cap);
	*ret_with_cap = true;

	/* FIXME(MK): We should use new error codes for IPC. */
	if (err)
		return -ENOSPC;
	return 0;
}


/*
 * Types in the following two functions would conflict with existing builds,
 * I suggest to move the tmpfs code out of kernel tree to resolve this.
 */
// int fs_stat(const char *pathname, struct stat *statbuf);
// int fs_getdents(int fd, struct dirent *dirp, size_t count);

void tmpfs_dispatch(ipc_msg_t *ipc_msg)
{
	int ret = 0;
	bool ret_with_cap = false;

	if (ipc_msg->data_len >= 4) {
		struct fs_request *fr =
		    (struct fs_request *)ipc_get_msg_data(ipc_msg);
		switch (fr->req) {
		case FS_REQ_SCAN: {
			/* TODO: add lock */
			int cap = ipc_get_msg_cap(ipc_msg, 0);
			ret = usys_map_pmo(SELF_CAP, cap, TMPFS_BUF_VADDR,
					   VM_READ | VM_WRITE);

			if (ret < 0) {
				error("usys_map_pmo on copied pmo ret %d\n",
				      ret);
				usys_exit(-1);
			}

			ret = fs_scan(fr->path, fr->offset,
				      (void *)TMPFS_BUF_VADDR, fr->count);

			usys_unmap_pmo(SELF_CAP, cap, TMPFS_BUF_VADDR);
			break;
		}
		case FS_REQ_MKDIRAT:
			ret = fs_mkdirat(ipc_msg, fr);
			break;
		case FS_REQ_UNLINKAT:
		case FS_REQ_RMDIRAT:
			ret = fs_unlinkat(ipc_msg, fr);
			break;
		case FS_REQ_CREAT:
			ret = fs_creat(fr->path, fr->mode);
			break;
		case FS_REQ_OPEN:
			ret = fs_open(ipc_msg, fr);
			break;
		case FS_REQ_CLOSE:
			ret = fs_close(fr->fd);
			break;
		case FS_REQ_WRITE: {
			/* int cap = ipc_get_msg_cap(ipc_msg, 0); */
			/* ret = usys_map_pmo(SELF_CAP, cap, TMPFS_BUF_VADDR, */
			/* 		   VM_READ | VM_WRITE); */

			/* if (ret < 0) { */
			/* 	error("usys_map_pmo on copied pmo ret %d\n", */
			/* 	      ret); */
			/* 	usys_exit(-1); */
			/* } */

			/* ret = fs_write(fr->path, fr->offset, */
			/* 	       (void *)TMPFS_BUF_VADDR, fr->count); */
			/* usys_unmap_pmo(SELF_CAP, cap, TMPFS_BUF_VADDR); */
			ret = tmpfs_write(fr->fd,
					  ipc_get_msg_data(ipc_msg) +
					      sizeof(struct fs_request),
					  fr->count);
			break;
		}
		case FS_REQ_READ: {
			int cap = ipc_get_msg_cap(ipc_msg, 0);
			ret = usys_map_pmo(SELF_CAP, cap, TMPFS_BUF_VADDR,
					   VM_READ | VM_WRITE);

			if (ret < 0) {
				error("usys_map_pmo on copied pmo ret %d\n",
				      ret);
				usys_exit(-1);
			}

			ret = fs_read(fr->path, fr->offset,
				      (void *)TMPFS_BUF_VADDR, fr->count);
			usys_unmap_pmo(SELF_CAP, cap, TMPFS_BUF_VADDR);
			break;
		}
		case FS_REQ_READ_FD: {
			ret = tmpfs_read(fr->fd, fr->count, ipc_msg);
			break;
		}
		case FS_REQ_GETDENTS64: {
			ret = fs_getdents(fr->fd, fr->count, ipc_msg);
			break;
		}
		case FS_REQ_LSEEK: {
			ret = tmpfs_lseek(fr->fd, fr->offset, fr->whence);
			break;
		}
		case FS_REQ_CHDIR: {
			ret = fs_chdir(ipc_msg, fr);
			break;
		}
		case FS_REQ_FCHDIR: {
			ret = fs_fchdir(ipc_msg, fr);
			break;
		}
		case FS_REQ_GETCWD: {
			ret = fs_getcwd(ipc_msg, fr);
			break;
		}
		case FS_REQ_GET_SIZE: {
			ret = fs_get_size(fr->path);
			break;
		}
		case FS_REQ_FSTATAT: {
			ret = fs_fstatat(ipc_msg, fr);
			break;
		}
		case FS_REQ_FSTAT: {
			ret = fs_fstat(ipc_msg, fr);
			break;
		}
		case FS_REQ_STATFS: {
			ret = fs_fstatfsat(ipc_msg, fr);
			break;
		}
		case FS_REQ_FSTATFS: {
			ret = fs_fstatfs(ipc_msg, fr);
			break;
		}
		case FS_REQ_FACCESSAT: {
			ret = fs_faccessat(ipc_msg, fr);
			break;
		}
		case FS_REQ_SYMLINKAT: {
			ret = fs_symlinkat(ipc_msg, fr);
			break;
		}
		case FS_REQ_READLINKAT: {
			ret = fs_readlinkat(ipc_msg, fr);
			break;
		}
		case FS_REQ_RENAMEAT: {
			ret = fs_rename(fr);
			break;
		}
		case FS_REQ_FTRUNCATE: {
			ret = fs_ftruncate(fr);
			break;
		}
		case FS_REQ_FALLOCATE: {
			ret = fs_fallocate(fr);
			break;
		}
		case FS_REQ_FCNTL: {
			ret = fs_fcntl(fr);
			break;
		}
		case FS_REQ_FMAP: {
			ret = fs_fmap(ipc_msg, fr, &ret_with_cap);
			break;
		}
		default:
			error("%s: %d Not impelemented yet (fr->req=%d)\n",
			      __func__, ((int *)ipc_get_msg_data(ipc_msg))[0],
			      fr->req);
			usys_exit(-1);
			break;
		}
	} else {
		usys_exit(-1);
	}

	if (ret_with_cap)
		ipc_return_with_cap(ipc_msg, ret);
	else
		ipc_return(ipc_msg, ret);
}

int init_tmpfs(void)
{
	tmpfs_root = new_dir(NULL);
	tmpfs_root_dent = new_dent(tmpfs_root, "/", 1);

	init_id_manager(&fidman, MAX_NR_FID_RECORDS);
	/**
	 * Allocate the first id, which should be 0.
	 * No request should use 0 as the fid.
	 */
	assert(alloc_id(&fidman) == 0);

	return 0;
}

int init_fs(void)
{
	return init_tmpfs();
}

int main(int argc, char *argv[], char *envp[])
{
	void *token;
	u64 size;

	token = strstr(argv[0], KERNEL_SERVER);

	if (token == NULL) {
		error("tmpfs: It is meaningless to run me in Shell. Bye!\n");
		usys_exit(-1);
		return 0;
	}

	init_tmpfs();

	size = usys_fs_load_cpio(RAMDISK_CPIO, QUERY_SIZE, 0);
	kernel_cpio_bin = mmap(0, size, PROT_READ | PROT_WRITE,
			       MAP_PRIVATE | MAP_ANONYMOUS,
			       -1, 0);
	if (kernel_cpio_bin == (void *)-1) {
		error("Failed: mmap for loading ramdisk_cpio.\n");
		usys_exit(-1);
	}
	usys_fs_load_cpio(RAMDISK_CPIO, LOAD_CPIO, kernel_cpio_bin);

	tfs_load_image((char *)kernel_cpio_bin);
	info("[tmpfs] register server value = %u\n",
	     ipc_register_server(tmpfs_dispatch,
				 DEFAULT_CLIENT_REGISTER_HANDLER));

	while (1) {
		usys_yield();
	}

	return 0;
}
