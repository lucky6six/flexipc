#pragma once

#include <chcore/ipc.h>
#include <chcore/idman.h>

#define TMPFS_INFO_VADDR 0x200000

#define PREFIX "[fsm]"
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#if 0
#define debug(fmt, ...) \
	printf(PREFIX "<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) do { } while (0)
#endif
#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)

#define MAX_FS_NUM 10
#define MAX_CLIENT_NUM 10
#define MAX_FD_NUM 255
#define MAX_MOUNT_POINT_LEN 255
#define MAX_CWD_LEN 255
#define MAX_PATH_LEN 511
#define MAX_FILENAME_LEN (255)

#define get_client(idx) (&client_records[idx])

struct FSRecord {
	int fs_cap, info_page_pmo_cap;
	ipc_struct_t *_fs_ipc_struct;
	char mount_point[MAX_MOUNT_POINT_LEN + 1];
	int refcnt;
};

struct FDRecord {
	bool used;
	struct FSRecord *fs;
	int fid; // fid assigned by a specific fs
	int fd;  // user-perceived fd
	char path[MAX_PATH_LEN + 1];
};

struct ClientRecord {
	u64 client_badge;
	/* cwd */
	char cwd[MAX_CWD_LEN + 1];
	struct FSRecord *cwd_fs;
	int cwd_fid; // fid assigned by a specific fs

	struct id_manager fdman;
	struct FDRecord fd_table[MAX_FD_NUM]; // index equals fd
	int cap_table[MAX_FS_NUM];
};

// TODO: these containers do not support removal
extern struct FSRecord fs_records[MAX_FS_NUM];
extern struct ClientRecord client_records[MAX_FS_NUM];
extern char full_path_buf[MAX_PATH_LEN + 1];

extern int fs_num;
extern int client_num;
