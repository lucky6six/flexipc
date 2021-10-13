#include "fsm.h"
#include <string.h>
#include <sys/mman.h>

struct FSRecord fs_records[MAX_FS_NUM];
struct ClientRecord client_records[MAX_FS_NUM];

int fs_num = 0;
int client_num = 0;

int init_fsm(void)
{
	u64 size;
	int ret;

	size = usys_fs_load_cpio(TMPFS_CPIO, QUERY_SIZE, 0);
	kernel_cpio_bin = mmap(0, size, PROT_READ | PROT_WRITE,
			       MAP_PRIVATE | MAP_ANONYMOUS,
			       -1, 0);
	if (kernel_cpio_bin == (void *)-1) {
		printf("Failed: mmap for loading tmpfs_cpio.\n");
		usys_exit(-1);
	}
	usys_fs_load_cpio(TMPFS_CPIO, LOAD_CPIO, kernel_cpio_bin);

	ret = fsm_mount_fs("/tmpfs.srv", "/", /* builtin_binary */ true);
	if (ret < 0) {
		error("failed to mount tmpfs, ret %d\n", ret);
		usys_exit(-1);
	}

	return 0;
}

int fsm_mount_fs(const char *path, const char *mount_point, bool builtin_binary)
{
	int fs_idx;

	if (fs_num == MAX_FS_NUM) {
		error("maximal number of FSs is reached: %d\n", fs_num);
		usys_exit(-1);
	}

	if (strlen(mount_point) > MAX_MOUNT_POINT_LEN) {
		error("mount point too long: > %d\n", MAX_MOUNT_POINT_LEN);
		usys_exit(-1);
	}

	if (mount_point[0] != '/') {
		error("mount point should start with '/'\n");
		usys_exit(-1);
	}

	for (fs_idx = 0; fs_idx < fs_num; ++fs_idx) {
		if (strcmp(mount_point, fs_records[fs_idx].mount_point) == 0) {
			error("mount point already in use: %s\n", mount_point);
			usys_exit(-1);
		}
	}

	if (!builtin_binary) {
		info("not implemented. before implement this, please fix the"
		     " issue in the comment\n");
		usys_exit(-1);
	} else {
		int ret;
		int i;

		/* create a new process */
		info("Mounting fs from kernel cpio: %s...\n", path);

		ret = run_cmd_from_kernel_cpio(path, &fs_records[fs_num].fs_cap,
					       NULL, 0, FIXED_BADGE_TMPFS);

		if (ret != 0) {
			info("create_process returns %d\n", ret);
			usys_exit(-1);
		}

		// register IPC client
		fs_records[fs_num]._fs_ipc_struct =
		    ipc_register_client(fs_records[fs_num].fs_cap);
		if (fs_records[fs_num]._fs_ipc_struct == NULL) {
			info("ipc_register_client failed\n");
			usys_exit(-1);
		}

		for (i = 0; i < strlen(mount_point); ++i) {
			fs_records[fs_num].mount_point[i] = mount_point[i];
		}
		fs_records[fs_num].mount_point[i] = '\0';

		info("new fs is up, with cap = %d\n",
		     fs_records[fs_num].fs_cap);
	}
	fs_num++;

	return 0;
}

int fsm_umount_fs(void)
{
	error("not implemented\n");
	usys_exit(-1);

	return 0;
}

// given full path, find which FSRecord corresponds to it, returns the index
int fsm_match_fs(const char *path)
{
	int i, fs_idx = 0;
	int len;
	// take max because mount points can have common prefix
	int len_matched_max = 0;

	if (path[0] == '/') {
		for (i = 0; i < fs_num; ++i) {
			len = strlen(fs_records[i].mount_point);
			if (len > len_matched_max) {
				if (strncmp(path, fs_records[i].mount_point,
					    len) == 0) {
					len_matched_max = len;
					fs_idx = i;
				}
			}
		}

		if (len_matched_max == 0) {
			error("no match fs for this path: %s\n", path);
			usys_exit(-1);
		}

		return fs_idx;
	} else {
		error("cannot find corresponding fs with relative path, no cwd "
		      "yet\n");
		usys_exit(-1);
	}

	return 0;
}

int fsm_match_or_record_client(u64 client_badge)
{
	int client_idx;

	for (client_idx = 0; client_idx < client_num; ++client_idx) {
		if (client_records[client_idx].client_badge == client_badge) {
			break;
		}
	}

	if (client_idx == client_num) {
		if (client_num == MAX_CLIENT_NUM) {
			return -1;
		}
		/* New Client. */
		client_num++;
		/* Init manager for fd. */
		init_id_manager(&client_records[client_idx].fdman, MAX_FD_NUM);
		client_records[client_idx].client_badge = client_badge;
		strcpy(client_records[client_idx].cwd, "/");
		client_records[client_idx].cwd_fs = get_root_fs();
		client_records[client_idx].cwd_fid = AT_FDROOT;
	}
	return client_idx;
}

static int fsm_record_client_cap(struct ClientRecord *client, int cap,
				 ipc_msg_t *ipc_msg, bool *ret_with_cap)
{
	int i, first_zero = MAX_FS_NUM;
	for (i = 0; i < MAX_FS_NUM; i++) {
		if (client->cap_table[i] == cap)
			return 0;
		if (first_zero == MAX_FS_NUM && client->cap_table[i] == 0)
			first_zero = i;
	}
	ipc_set_ret_msg_cap(ipc_msg, cap);
	*ret_with_cap = true;
	client->cap_table[first_zero] = cap;
	return 1;
}

/*
 * Types in the following two functions would conflict with existing builds,
 * I suggest to move the tmpfs code out of kernel tree to resolve this.
 */
// int fs_stat(const char *pathname, struct stat *statbuf);
// int fs_getdents(int fd, struct dirent *dirp, size_t count);

void fsm_dispatch(ipc_msg_t *ipc_msg, u64 client_badge)
{
	int ret = 0;
	struct fs_request *fr;
	int client_idx;
	bool ret_with_cap = false;

	if (ipc_msg->data_len >= 4) {
		// recognize client
		client_idx = fsm_match_or_record_client(client_badge);
		if (client_idx < 0) {
			error("cannot serve more clients. client num: %d\n",
			      client_num);
			usys_exit(-1);
		}

		// construct full path, concat cwd to relative path if needed
		fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

		switch (fr->req) {
		case FS_REQ_SCAN: {
			int scan_pmo = ipc_get_msg_cap(ipc_msg, 0);
			ret = fsm_scan(client_idx,
				       fr->path,
				       scan_pmo,
				       /* start */ fr->offset);
			break;
		}
		case FS_REQ_MKDIRAT: {
			ret = fsm_mkdirat(client_idx, fr->dirfd, fr->path,
					  fr->mode);
			break;
		}
		case FS_REQ_RMDIR: {
			/* no rmdir in musl libc, it use unlinkat with flag
			 * AT_REMOVEDIR */
			assert(0);
			break;
		}
		case FS_REQ_CREAT: {
			break;
		}
		case FS_REQ_UNLINKAT: {
			ret = fsm_unlinkat(client_idx, fr->dirfd, fr->path,
					   fr->flags);
			break;
		}
		case FS_REQ_OPEN: {
			/* no open in musl libc, it use openat only */
			assert(0);
			break;
		}
		case FS_REQ_OPENAT: {
			ret = fsm_openat(client_idx, fr);
			if (ret < 0)
				break;

			fsm_record_client_cap(get_client(client_idx),
					      fr->cap, ipc_msg, &ret_with_cap);
			strcpy(client_records[client_idx].fd_table[ret].path,
			       fr->path);
			break;
		}
		case FS_REQ_CLOSE: {
			ret = fsm_close(client_idx, fr->fd);
			break;
		}
		case FS_REQ_STATX: {
			ret = fsm_statx(fr->dirfd, fr->path, fr->flags,
					fr->mask, fr->buff);
			break;
		}
		/* NOTE(MK): For stat-like requests, we pass statbuf in
		 * msg_data. */
		case FS_REQ_FSTAT:
		case FS_REQ_FSTATFS: {
			/**
			 * ipc_get_msg_data returns the pointer, which we can
			 * also use to write data.
			 */
			void *statbuf = ipc_get_msg_data(ipc_msg);
			size_t bufsize = (fr->req == FS_REQ_FSTAT)
					     ? sizeof(struct stat)
					     : sizeof(struct statfs);

			ret = fsm_fstatxx(client_idx, fr->req, fr->fd, statbuf,
					  bufsize);
			break;
		}
		case FS_REQ_STATFS:
		case FS_REQ_FSTATAT: {
			/**
			 * ipc_get_msg_data returns the pointer, which we can
			 * also use to write data.
			 */
			void *statbuf = ipc_get_msg_data(ipc_msg);
			size_t bufsize;
			int flags;

			if (fr->req == FS_REQ_STATFS) {
				bufsize = sizeof(struct statfs);
				flags = 0;
			} else {
				bufsize = sizeof(struct stat);
				flags = fr->flags;
			}

			/**
			 * The libc manages cwd, thus for FS_REQ_STATFS, the
			 * fr->dirfd should give the cwd fd.
			 */
			ret = fsm_fstatxxat(client_idx, fr->req, fr->dirfd,
					    fr->path, flags, statbuf, bufsize);
			break;
		}
		case FS_REQ_LSEEK: {
			ret = fsm_lseek(
			    client_records[client_idx].fd_table[fr->fd].fid,
			    fr->offset, fr->whence,
			    client_records[client_idx]
				.fd_table[fr->fd]
				.fs->_fs_ipc_struct);
			break;
		}
		case FS_REQ_WRITE: {
			ret = fsm_write(
			    client_records[client_idx].fd_table[fr->fd].fid,
			    fr->count,
			    client_records[client_idx]
				.fd_table[fr->fd]
				.fs->_fs_ipc_struct,
			    ipc_msg);
			break;
		}
		case FS_REQ_WRITEV: {
			info("writev: not implemented yet\n");
			break;
		}
		case FS_REQ_RENAMEAT: {
			ret = fsm_rename(client_idx, fr->dirfd, fr->path,
					 fr->dirfd2, fr->path2);
			break;
		}
		case FS_REQ_SYMLINKAT: {
			ret = fsm_symlinkat(client_idx, fr->path, fr->dirfd,
					    fr->path2);
			break;
		}
		case FS_REQ_CHDIR: {
			ret = fsm_chdir(client_idx, fr->path);
			break;
		}
		case FS_REQ_FCHDIR: {
			ret = fsm_fchdir(&client_records[client_idx], fr->fd,
					 client_records[client_idx]
					     .fd_table[fr->fd]
					     .fs->_fs_ipc_struct,
					 ipc_msg);
			break;
		}
		case FS_REQ_GETCWD: {
			ret = fsm_getcwd(&client_records[client_idx], ipc_msg);
			break;
		}
		case FS_REQ_GETDENTS64: {
			ret = fsm_getdents(&client_records[client_idx], ipc_msg,
					   fr->fd, fr->count);
			break;
		}
		case FS_REQ_READ: {
			/* TODO: support fd */
			ret = fsm_read_file(
			    client_idx,
			    fr->path,
			    /* read_pmo */ ipc_get_msg_cap(ipc_msg, 0),
			    fr->offset, fr->count);
			break;
		}
		case FS_REQ_READ_FD: {
			ret = fsm_read(
			    client_records[client_idx].fd_table[fr->fd].fid,
			    fr->count,
			    client_records[client_idx]
				.fd_table[fr->fd]
				.fs->_fs_ipc_struct,
			    ipc_msg);
			break;
		}
		case FS_REQ_READLINKAT: {
			ret = fsm_readlinkat(client_idx, fr->dirfd, fr->path,
					     fr->bufsiz, ipc_msg);
			break;
		}
		case FS_REQ_GET_SIZE: {
			ret = fsm_get_size(client_idx, fr->path);
			break;
		}
		case FS_REQ_FACCESSAT: {
			ret = fsm_faccessat(client_idx, fr->dirfd, fr->path,
					    fr->amode, fr->flags);
			break;
		}
		case FS_REQ_FTRUNCATE: {
			ret = fsm_ftruncate(client_idx, fr->fd, fr->len);
			break;
		}
		case FS_REQ_FALLOCATE: {
			ret = fsm_fallocate(client_idx, fr->fd, fr->mode,
					    fr->offset, fr->len);
			break;
		}
		case FS_REQ_FCNTL: {
			ret = fsm_fcntl(client_idx, fr->fd, fr->fcntl_cmd,
					fr->fcntl_arg);
			break;
		}
		case FS_REQ_FMAP: {
			ret = fsm_fmap(client_idx, fr->fd, fr->count,
				       fr->offset, ipc_msg, &ret_with_cap);
			break;
		}
		default:
			error("%s: %d Not impelemented yet\n", __func__,
			      ((int *)(ipc_get_msg_data(ipc_msg)))[0]);
			usys_exit(-1);
			break;
		}
	} else {
		error("FSM: no operation num\n");
		usys_exit(-1);
	}

	if (ret_with_cap)
		ipc_return_with_cap(ipc_msg, ret);
	else
		ipc_return(ipc_msg, ret);
}

int main(int argc, char *argv[], char *envp[])
{
	void *token;

	/*
	 * A token for starting the FSM server.
	 * This is just for preventing users run FSM in the Shell.
	 * It is actually harmless to remove this.
	 */
	token = strstr(argv[0], KERNEL_SERVER);
	if (token == NULL) {
		error("FSM: I am a system server instead of an (Shell) "
		      "application. Bye!\n");
		usys_exit(-1);
	}

	init_fsm();
	info("[FSM] register server value = %u\n",
	     ipc_register_server(fsm_dispatch,
				 DEFAULT_CLIENT_REGISTER_HANDLER));

	/*
	 * TODO:
	 *  - use other inferfaces like sleep later.
	 *    This thread is only required when others connect this server.
	 *  - This thread can do anything it wants.
	 *
	 * TODO: When a thread exits, its cap in other cap_group should be
	 * removed (e.g., cap revoke).
	 */
	while (1) {
		usys_yield();
	}

	return 0;
}
