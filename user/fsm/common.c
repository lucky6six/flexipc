#include "common.h"

#include <fcntl.h>

// FIXME(TCZ): we need to define what does reference mean
void ref_fs(struct FSRecord *fs) { fs->refcnt += 1; }

void unref_fs(struct FSRecord *fs)
{
	assert(fs->refcnt > 0);
	fs->refcnt += 1;
}

struct FSRecord *get_root_fs()
{
	assert(fs_num > 0);
	assert(strcmp(fs_records[0].mount_point, "/") == 0);
	return get_fs_record(0);
}

struct FSRecord *new_fs_record(void)
{
	assert(fs_num < MAX_FS_NUM);
	return &fs_records[fs_num++];
}

struct FSRecord *get_fs_record(int idx)
{
	/* Check the validity and increament the refcnt. */
	assert(idx >= 0);
	assert(idx < fs_num);
	ref_fs(&fs_records[idx]);
	return &fs_records[idx];
}

void put_fs_record(struct FSRecord *fs)
{
	/* Leave this function for refcnt. */
	unref_fs(fs);
	return;
}

struct FDRecord *new_fd_record(struct ClientRecord *client)
{
	struct FDRecord *fd;
	int id;

	id = alloc_id(&client->fdman);
	assert(id >= 0);

	/* 0 is never used. */
	if (id == 0) {
		id = alloc_id(&client->fdman);
		assert(id > 0);
	}

	fd = &client->fd_table[id];
	fd->fd = id;
	fd->used = true;

	return fd;
}

/**
 * return a `struct FDRecord *` if fd is valid, else NULL.
 */
struct FDRecord *client_fd(struct ClientRecord *client, int fd)
{
	int err;

	err = query_id(&client->fdman, fd);
	if (err)
		return NULL;

	return client->fd_table[fd].used ? &client->fd_table[fd] : NULL;
}

int del_fd_record(struct ClientRecord *client, int fd)
{
	int err;

	/* We don't use 0 for fd. */
	if (fd == 0)
		return -EBADF;

	err = query_id(&client->fdman, fd);
	if (err)
		/* Not found. */
		return -EBADF;

	client->fd_table[fd].used = false;
	client->fd_table[fd].fs = NULL;
	client->fd_table[fd].fid = 0;
	client->fd_table[fd].fd = 0;
	client->fd_table[fd].path[0] = '\0';

	err = free_id(&client->fdman, fd);
	assert(err == 0);

	return 0;
}

int simple_forward(ipc_struct_t *ipc_struct, struct fs_request *fr)
{
	return simple_ipc_forward(ipc_struct, fr, sizeof(*fr));
}

int walk_path_init(struct walk_path *walk, int client_id, int dirfd,
		   const char *pathname)
{
	if (strlen(pathname) == 0)
		return -ENOENT;

	walk->path = pathname;
	walk->path_start = 0;
	walk->client = get_client(client_id);
	walk->next_fs_idx = -1;
	if (pathname[0] == '/') {
		walk->fs = get_root_fs();
		/*
		 * It's okay with prepending '/'s.
		 * So we don't need to increment path_start.
		 */
		walk->fid = AT_FDROOT;
		debug("Absolute path %s\n", pathname);
	} else {
		if (dirfd == AT_FDCWD) {
			ref_fs(walk->client->cwd_fs);
			walk->fs = walk->client->cwd_fs;
			walk->fid = walk->client->cwd_fid;
			debug("AT_FDCWD + %s; fs=%p, fid=%d\n",
			      pathname, walk->fs, walk->fid);
			return 0;
		}
		debug("fd=%d is given with pathname=%s\n", dirfd, pathname);
		struct FDRecord *fd_record = client_fd(walk->client, dirfd);
		if (!fd_record)
			return -ENOENT;

		ref_fs(fd_record->fs);
		walk->fs = fd_record->fs;
		walk->fid = fd_record->fid;
	}

	if (!walk->fs)
		return -ENOENT;

	return 0;
}

int fsm_walk(struct walk_path *walk, struct fs_request *fr,
	     fsm_walk_cb_t callback, void *data)
{
	int ret;
	ipc_msg_t *ipc_msg;
	struct fs_request *ret_fr;

	assert(walk);
	assert(fr);

	/* IPC send cap */
	ipc_msg = ipc_create_msg(walk->fs->_fs_ipc_struct,
				 sizeof(struct fs_request), 0);

	/* Setup path. */
	strncpy((char *)fr->path, walk->path + walk->path_start,
		FS_REQ_PATH_LEN);
	fr->path[FS_REQ_PATH_LEN] = '\0';

	fr->fd = walk->fid;

	ipc_set_msg_data(ipc_msg, (char *)fr, 0, sizeof(struct fs_request));
	ret = ipc_call(walk->fs->_fs_ipc_struct, ipc_msg);
	ret_fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
	assert(ret_fr);

	if (ret == -EAGAIN) {
		/**
		 * NOTE(MK): The EAGAIN error is general, so we handle it here.
		 */
		/* Get info of the next fs to prepare next walk. */
		walk->path_start += ret_fr->path_advanced;
		walk->next_fs_idx = ret_fr->next_fs_idx;
	}
	if (callback)
		ret = callback(ret, walk, ret_fr, data);

	ipc_destroy_msg(ipc_msg);

	return ret;
}
