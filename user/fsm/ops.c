#include "ops.h"
#include <fcntl.h>


int fsm_scan(int client_id, const char *path, int scan_pmo, int start)
{
	struct walk_path walk;
	struct fs_request fr;
	ipc_msg_t *ipc_msg;
	int err;

	err = walk_path_init(&walk, client_id, AT_FDROOT, path);
	if (err != 0)
		return err;

	fr.req = FS_REQ_SCAN;
	fr.offset = start;
	fr.count = PAGE_SIZE;
	strncpy((void *)fr.path, path, FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';

	/* IPC send cap */
	ipc_msg = ipc_create_msg(walk.fs->_fs_ipc_struct,
				 sizeof(struct fs_request), 1);
	ipc_set_msg_cap(ipc_msg, 0, scan_pmo);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	err = ipc_call(walk.fs->_fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return err;
}

int fsm_read_file(int client_id, const char *path, int read_pmo_cap,
		  off_t offset, size_t count)
{
	struct walk_path walk;
	int err;
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request fr;

	err = walk_path_init(&walk, client_id, AT_FDROOT, path);
	if (err != 0)
		return err;

	strncpy((void *)fr.path, path, FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';
	fr.offset = offset;
	fr.count = count;
	fr.req = FS_REQ_READ;

	ipc_msg = ipc_create_msg(walk.fs->_fs_ipc_struct,
				 sizeof(struct fs_request), 1);
	ipc_set_msg_cap(ipc_msg, 0, read_pmo_cap);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(walk.fs->_fs_ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int fsm_get_size(int client_id, const char *path)
{
	struct walk_path walk;
	struct fs_request fr;
	int err;

	err = walk_path_init(&walk, client_id, AT_FDROOT, path);
	if (err != 0)
		return err;

	fr.req = FS_REQ_GET_SIZE;
	strncpy((void *)fr.path, path, FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';

	return simple_forward(walk.fs->_fs_ipc_struct, &fr);
}

int fsm_openat_cb(int retcode, struct walk_path *walk,
		  struct fs_request *response, void *fid)
{
	if (!retcode)
		*(int *)fid = ((struct fs_request *)response)->fid;

	return retcode;
}

/**
 * FS could be recursively mounted.
 * To access a path, we need to go through all fs instances on the path.
 *
 * Returns:
 *  < 0 for errors
 *  >=0 for valid fd
 */
int fsm_openat(int client_id, struct fs_request *ret_fr)
{
	struct walk_path walk;
	struct fs_request fr;
	int fid; // filled by callback, when file is open in a FS
	int dirfd = ret_fr->dirfd;
	const char *pathname = ret_fr->path;
	int flags = ret_fr->flags;
	mode_t mod = ret_fr->mode;
	int err;

	err = walk_path_init(&walk, client_id, dirfd, pathname);
	if (err != 0)
		return err;

	fr.req = FS_REQ_OPEN;
	fr.flags = flags;
	fr.mode = mod;

	// forward request with remaining path
	while (true) {
		err = fsm_walk(&walk, &fr, fsm_openat_cb, &fid);
		if (err == 0) {
			/* Succeed */
			struct FDRecord *fd = new_fd_record(walk.client);
			fd->fs = walk.fs;
			fd->fid = fid;
			debug("%s: fd=%d\n", __func__, fd->fd);
			ret_fr->cap = fd->fs->fs_cap;
			ret_fr->fid = fid;
			return fd->fd;
		}
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			/**
			 * Continue walking in the next fs instance. Thus, we
			 * always start from the root of the next fs.
			 */
			walk.fid = AT_FDROOT;
			continue;
		}
		return err;
	}
	assert(0);
}

int fsm_close(int client_idx, int fd)
{
	struct ClientRecord *client;
	struct FDRecord *fd_record;
	struct FSRecord *fs_record;
	struct fs_request fr;
	ipc_msg_t *ipc_msg;
	int ret;

	/* Get client. */
	client = get_client(client_idx);
	assert(client);

	/* Get fd_record. */
	fd_record = client_fd(client, fd);
	if (!fd_record)
		return -ENOENT;
	assert(fd_record->fs);

	fs_record = fd_record->fs;

	/* Prepare and invoke IPC. */
	fr.fd = fd_record->fid;
	fr.req = FS_REQ_CLOSE;
	ipc_msg = ipc_create_msg(fs_record->_fs_ipc_struct,
				 sizeof(struct fs_request), 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));

	ret = ipc_call(fs_record->_fs_ipc_struct, ipc_msg);

	if (ret == 0 || ret == -EBADF) {
		/**
		 * Delete the fid record if the fs deletes it successfully or
		 * the fs forgets the fid.
		 */
		ret = del_fd_record(client, fd);
	}

	ipc_destroy_msg(ipc_msg);

	return ret;
}

int fsm_statx(int dirfd, const char *pathname, int flags, unsigned int mask,
	      void *statxbuf)
{
	info("statx: not implemented yet\n");
	return 0;
}

/**
 * This helper function serves for fstat and fstatfs.
 * req == FS_REQ_FSTAT => bufsize == sizeof(struct fstat)
 * req == FS_REQ_FSTATFS => bufsize == sizeof(struct fstatfs)
 */
int fsm_fstatxx(int client_idx, int req, int fd, void *statbuf, size_t bufsize)
{
	struct ClientRecord *client;
	struct FDRecord *fd_record;
	struct FSRecord *fs_record;
	struct fs_request fr;
	ipc_msg_t *ipc_msg;
	int ret;

	/* Get client. */
	client = get_client(client_idx);
	assert(client);

	/* Get fd_record. */
	fd_record = client_fd(client, fd);
	if (!fd_record)
		return -ENOENT;
	assert(fd_record->fs);

	fs_record = fd_record->fs;

	/* Prepare and invoke IPC. */
	/* TODO(MK): Can we reuse libc_ipc_msg? */
	fr.fd = fd_record->fid;
	fr.req = req;
	ipc_msg = ipc_create_msg(fs_record->_fs_ipc_struct,
				 sizeof(struct fs_request), 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));

	ret = ipc_call(fs_record->_fs_ipc_struct, ipc_msg);

	if (ret == 0) {
		/**
		 * We could copy struct stat or struct statfs, depending on the
		 * bufsize.
		 */
		memcpy(statbuf, ipc_get_msg_data(ipc_msg), bufsize);
	}

	ipc_destroy_msg(ipc_msg);

	return ret;
}

int fsm_fstatxxat_cb(int retcode, struct walk_path *walk,
		     struct fs_request *response, void *__args)
{
	struct fstatxxat_cb_args *args = __args;
	// info("%s: retcode=%d\n", __func__, retcode);
	if (retcode == 0) {
		/* Succeed. */
		/* copy from response */
		memcpy(args->statbuf, (void *)response, args->bufsize);
		return 0;
	}
	/* For reset errors. */
	return retcode;
}

int fsm_fstatxxat(int client_id, int req, int dirfd, const char *pathname,
		  int flags, void *statbuf, size_t bufsize)
{
	struct fstatxxat_cb_args args;
	struct walk_path walk;
	struct fs_request fr;
	int err;

	args.statbuf = statbuf;
	args.bufsize = bufsize;

	err = walk_path_init(&walk, client_id, dirfd, pathname);
	if (err != 0)
		return err;

	fr.req = req;
	fr.flags = flags;

	// forward request with remaining path
	while (true) {
		err = fsm_walk(&walk, &fr, fsm_fstatxxat_cb, &args);
		if (err == 0) {
			/* Succeed */
			return 0;
		}
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			/**
			 * Continue walking in the next fs instance. Thus, we
			 * always start from the root of the next fs.
			 */
			walk.fid = AT_FDROOT;
			continue;
		}
		if (err == -ENOENT)
			return err;
		error("Other errors not handled yet.\n");
		assert(0);
		break;
	}
	assert(0);
}

int fsm_lseek(int fd, off_t offset, int whence, ipc_struct_t *ipc_struct)
{
	struct fs_request fr;

	fr.fd = fd;
	fr.offset = offset;
	fr.whence = whence;
	fr.req = FS_REQ_LSEEK;

	return simple_forward(ipc_struct, &fr);
}

int fsm_symlinkat(int client_id, const char *path1, int dirfd,
		  const char *path2)
{
	struct walk_path walk;
	struct fs_request fr;
	int err;

	err = walk_path_init(&walk, client_id, dirfd, path2);
	if (err != 0)
		return err;

	fr.req = FS_REQ_SYMLINKAT;
	// save path1 in fr.path2 since fr.path will be used for the walk
	strncpy(fr.path2, path1, FS_REQ_PATH_LEN);
	fr.path2[FS_REQ_PATH_LEN] = '\0';

	// forward request with remaining path
	while (true) {
		err = fsm_walk(&walk, &fr, NULL, NULL);
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			walk.fid = AT_FDROOT;
		} else {
			return err;
		}
	}
	assert(0);
}

int fsm_readlinkat_cb(int retcode, struct walk_path *walk,
		      struct fs_request *response, void *libc_ipc_msg)
{
	if (retcode > 0) {
		ipc_set_msg_data((ipc_msg_t *)libc_ipc_msg, response->path2, 0,
				 retcode);
	}

	return retcode; // retcode is buf size on success
}

int fsm_readlinkat(int client_id, int dirfd, const char *pathname,
		   size_t bufsiz, ipc_msg_t *libc_ipc_msg)
{
	struct walk_path walk;
	struct fs_request fr;
	int err;

	err = walk_path_init(&walk, client_id, dirfd, pathname);
	if (err != 0)
		return err;

	fr.req = FS_REQ_READLINKAT;
	fr.bufsiz = bufsiz;
	// forward request with remaining path
	while (true) {
		err = fsm_walk(&walk, &fr, fsm_readlinkat_cb, libc_ipc_msg);
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			walk.fid = AT_FDROOT;
		} else {
			return err;
		}
	}
	assert(0);
	return 0;
}

int fsm_read(int fd, size_t count, ipc_struct_t *ipc_struct,
	     ipc_msg_t *libc_ipc_msg)
{
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request fr;

	fr.fd = fd;
	fr.count = count;
	fr.req = FS_REQ_READ_FD;
	ipc_msg = ipc_create_msg(ipc_struct, libc_ipc_msg->data_len, 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(ipc_struct, ipc_msg);
	ipc_set_msg_data(libc_ipc_msg, ipc_get_msg_data(ipc_msg), 0, ret);
	ipc_destroy_msg(ipc_msg);

	return ret;
}

int fsm_write(int fd, size_t count, ipc_struct_t *ipc_struct,
	      ipc_msg_t *libc_ipc_msg)
{
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request fr;

	fr.fd = fd;
	fr.count = count;
	fr.req = FS_REQ_WRITE;
	ipc_msg =
	    ipc_create_msg(ipc_struct, sizeof(struct fs_request) + count, 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ipc_set_msg_data(
	    ipc_msg, ipc_get_msg_data(libc_ipc_msg) + sizeof(struct fs_request),
	    sizeof(struct fs_request), count);
	ret = ipc_call(ipc_struct, ipc_msg);
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int fsm_mkdirat(int client_id, int dirfd, const char *pathname, mode_t mode)
{
	struct walk_path walk;
	int err;
	struct fs_request fr;

	debug("dirfd=%d pathname=%s mode=0x%x\n", dirfd, pathname, mode);

	err = walk_path_init(&walk, client_id, dirfd, pathname);
	if (err)
		return err;

	fr.req = FS_REQ_MKDIRAT;
	fr.mode = mode;
	debug("Entering the loop...\n");
	while (true) {
		err = fsm_walk(&walk, &fr, NULL, NULL);
		if (err == 0) {
			/* Succeed */
			return 0;
		}
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			/**
			 * Continue walking in the next fs instance. Thus, we
			 * always start from the root of the next fs.
			 */
			walk.fid = AT_FDROOT;
			continue;
		}
		if (err == -ENOENT)
			return err;
		error("Other errors not handled yet. err=%d\n", err);
		assert(0);
		break;
	}
	assert(0);
}

int fsm_unlinkat(int client_id, int dirfd, const char *pathname, int flags)
{
	struct walk_path walk;
	int err;
	struct fs_request fr;

	err = walk_path_init(&walk, client_id, dirfd, pathname);
	if (err)
		return err;

	if (flags & AT_REMOVEDIR) {
		fr.req = FS_REQ_RMDIRAT;
		flags &= (~AT_REMOVEDIR);
	} else {
		fr.req = FS_REQ_UNLINKAT;
	}
	fr.flags = flags;
	while (true) {
		err = fsm_walk(&walk, &fr, NULL, NULL);
		if (err == 0) {
			/* Succeed */
			return 0;
		}
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			/**
			 * Continue walking in the next fs instance. Thus, we
			 * always start from the root of the next fs.
			 */
			walk.fid = AT_FDROOT;
			continue;
		}
		if (err == -ENOENT || err == -EISDIR || err == -ENOTDIR)
			return err;
		error("Other errors not handled yet.\n");
		assert(0);
		break;
	}
	assert(0);
}

int fsm_chdir(int client_id, const char *path)
{
	struct walk_path walk;
	struct fs_request fr;
	ipc_msg_t *ipc_msg;
	int ret;
	int err;
	struct ClientRecord *client = get_client(client_id);

	err = walk_path_init(&walk, client_id, AT_FDCWD, path);
	if (err != 0)
		return err;

	debug("path=%s\n", path);
	fr.req = FS_REQ_CHDIR;
	strncpy((void *)fr.path, path, FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';
	fr.dirfd = walk.fid;

	ipc_msg = ipc_create_msg(walk.fs->_fs_ipc_struct,
				 sizeof(struct fs_request), 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(walk.fs->_fs_ipc_struct, ipc_msg);
	if (ret > 0) {
		debug("dir changed to %s, fid=%d\n", path, ret);
#if 0
		if (fr.dirfd == AT_FDROOT) {
			strncpy(client->cwd, path, FS_REQ_PATH_LEN);
		} else {
			info("=> %s\n", client->fd_table[fr.dirfd].path);
			strncpy(client->cwd, client->fd_table[fr.dirfd].path,
				FS_REQ_PATH_LEN);
			strncat(client->cwd, "/", FS_REQ_PATH_LEN);
			strncat(client->cwd, path, FS_REQ_PATH_LEN);
			client->cwd[FS_REQ_PATH_LEN - 1] = '\0';
		}
#endif
		client->cwd_fs = walk.fs;
		client->cwd_fid = ret;
		ret = 0;
	}
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int fsm_fchdir(struct ClientRecord *client, int fd, ipc_struct_t *ipc_struct,
	       ipc_msg_t *libc_ipc_msg)
{
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request fr;

	fr.req = FS_REQ_FCHDIR;
	fr.fd = client->fd_table[fd].fid;
	ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct fs_request), 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(ipc_struct, ipc_msg);
	if (!ret) {
		// strcpy(client->cwd, client->fd_table[fd].path);
		client->cwd_fs = client->fd_table[fd].fs;
		client->cwd_fid = client->fd_table[fd].fid;
	}
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int fsm_getcwd(struct ClientRecord *client, ipc_msg_t *libc_ipc_msg)
{
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request fr;

	if (client->cwd_fid == AT_FDROOT) {
		ipc_set_msg_data(libc_ipc_msg, "/", 0, 2);
		return 0;
	}

	fr.req = FS_REQ_GETCWD;
	fr.fd = client->cwd_fid;
	ipc_msg = ipc_create_msg(client->cwd_fs->_fs_ipc_struct,
				 sizeof(struct fs_request), 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(client->cwd_fs->_fs_ipc_struct, ipc_msg);
	if (!ret) {
		// strcpy(client->cwd, client->fd_table[fd].path);
		char *cwd = ipc_get_msg_data(ipc_msg);
		ipc_set_msg_data(libc_ipc_msg, cwd, 0, strlen(cwd) + 1);
		debug("client->cwd: %s\n", cwd);
	}
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int fsm_getdents(struct ClientRecord *client, ipc_msg_t *libc_ipc_msg, int fd,
		 unsigned int count)
{
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request fr;
	ipc_struct_t *ipc_struct = client->fd_table[fd].fs->_fs_ipc_struct;

	fr.fd = client->fd_table[fd].fid;
	fr.count = count;
	fr.req = FS_REQ_GETDENTS64;
	ipc_msg = ipc_create_msg(ipc_struct, libc_ipc_msg->data_len, 0);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));
	ret = ipc_call(ipc_struct, ipc_msg);
	if (ret < 0)
		goto error;
	ipc_set_msg_data(libc_ipc_msg, ipc_get_msg_data(ipc_msg), 0, ret);

error:
	ipc_destroy_msg(ipc_msg);
	return ret;
}

int fsm_faccessat(int client_id, int dirfd, const char *pathname, int amode,
		  int flags)
{
	struct walk_path walk;
	struct fs_request fr;
	int err;

	err = walk_path_init(&walk, client_id, dirfd, pathname);
	if (err != 0)
		return err;

	fr.req = FS_REQ_FACCESSAT;
	fr.flags = flags;
	fr.amode = amode;

	// forward request with remaining path
	while (true) {
		err = fsm_walk(&walk, &fr, NULL, NULL);
		if (err == -EAGAIN) {
			put_fs_record(walk.fs);
			walk.fs = get_fs_record(walk.next_fs_idx);
			walk.fid = AT_FDROOT;
		} else {
			return err;
		}
	}
	assert(0);
}

int fsm_rename(int client_id, int oldfd, const char *old, int newfd,
	       const char *new)
{
	struct FSRecord *fs;
	struct FDRecord *fd_old = NULL, *fd_new = NULL;
	struct ClientRecord *client;
	struct fs_request fr;

	client = get_client(client_id);
	assert(client);

	if (old[0] == '/') {
		fs = get_root_fs();
	} else {
		if ((fd_old = client_fd(client, oldfd)) == NULL)
			return -ENOENT;
		fs = fd_old->fs;
	}

	if (new[0] == '/') {
		if (fs != get_root_fs()) {
			error("does not support cross-fs rename yet.\n");
			return -1;
		}
	} else {
		if ((fd_new = client_fd(client, newfd)) == NULL)
			return -ENOENT;
		if (fs != fd_new->fs) {
			error("does not support cross-fs rename yet.\n");
			return -1;
		}
	}

	fr.req = FS_REQ_RENAMEAT;
	fr.dirfd = fd_old == NULL ? AT_FDROOT : fd_old->fid;
	strncpy((void *)fr.path, (const char *)old, FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';
	fr.dirfd2 = fd_new == NULL ? AT_FDROOT : fd_new->fid;
	strncpy((void *)fr.path2, (const char *)new, FS_REQ_PATH_LEN);
	fr.path[FS_REQ_PATH_LEN] = '\0';

	return simple_forward(fs->_fs_ipc_struct, &fr);
}

int fsm_ftruncate(int client_id, int fd, off_t length)
{
	struct fs_request fr;
	struct ClientRecord *client;

	client = get_client(client_id);
	assert(client);
	ipc_struct_t *ipc_struct = client->fd_table[fd].fs->_fs_ipc_struct;

	fr.fd = client->fd_table[fd].fid;
	fr.len = length;
	fr.req = FS_REQ_FTRUNCATE;

	return simple_forward(ipc_struct, &fr);
}

int fsm_fallocate(int client_id, int fd, int mode, off_t offset, off_t len)
{
	struct fs_request fr;
	struct ClientRecord *client;

	client = get_client(client_id);
	assert(client);
	ipc_struct_t *ipc_struct = client->fd_table[fd].fs->_fs_ipc_struct;

	fr.fd = client->fd_table[fd].fid;
	fr.mode = mode;
	fr.offset = offset;
	fr.len = len;
	fr.req = FS_REQ_FALLOCATE;

	return simple_forward(ipc_struct, &fr);
}

int fsm_fcntl(int client_id, int fd, int fcntl_cmd, int fcntl_arg)
{
	struct fs_request fr;
	struct ClientRecord *client;
	struct FDRecord *fd_record;
	struct FSRecord *fs_record;

	client = get_client(client_id);
	assert(client);
	if ((fd_record = client_fd(client, fd)) == NULL)
		return -EBADF;
	assert(fd_record->fs);
	fs_record = fd_record->fs;

	fr.req = FS_REQ_FCNTL;
	fr.fcntl_cmd = fcntl_cmd;
	fr.fcntl_arg = fcntl_arg;
	fr.fd = fd_record->fid;

	return simple_forward(fs_record->_fs_ipc_struct, &fr);
}

int fsm_fmap(int client_idx, int fd, size_t count, off_t offset,
	     ipc_msg_t *libc_ipc_msg, bool *ret_with_cap)
{
	struct ClientRecord *client;
	struct FDRecord *fd_record;
	struct FSRecord *fs_record;
	struct fs_request fr;
	ipc_msg_t *ipc_msg;
	int ret;

	/* Get client. */
	client = get_client(client_idx);
	assert(client);

	/* Get fd_record. */
	fd_record = client_fd(client, fd);
	if (!fd_record)
		return -ENOENT;
	assert(fd_record->fs);

	fs_record = fd_record->fs;

	/* Prepare and invoke IPC. */
	/* fs_fmap needs (req, fd, offset, count)*/
	fr.fd = fd_record->fid;
	fr.req = FS_REQ_FMAP;
	fr.offset = offset;
	fr.count = count;

	/* One cap slot number to receive translated_pmo_cap. */
	ipc_msg = ipc_create_msg(fs_record->_fs_ipc_struct,
				 sizeof(struct fs_request), 1);
	ipc_set_msg_data(ipc_msg, (char *)&fr, 0, sizeof(struct fs_request));

	ret = ipc_call(fs_record->_fs_ipc_struct, ipc_msg);

	if (ret == 0) {
		assert(ipc_msg->cap_slot_number_ret > 0);
		/* Transfer the returned cap to libc.  */
		ret = ipc_set_ret_msg_cap(libc_ipc_msg,
					  ipc_get_msg_cap(ipc_msg, 0));
		*ret_with_cap = true;
		assert(ret == 0);
	}

	ipc_destroy_msg(ipc_msg);

	return ret;
}
