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

#include "defs.h"

struct walk_path {
	struct ClientRecord *client;
	struct FSRecord *fs;
	int fid;
	const char *path;
	int path_start;
	int next_fs_idx;
};

typedef int (*fsm_walk_cb_t)(int retcode, struct walk_path *walk,
			     struct fs_request *response, void *data);

void ref_fs(struct FSRecord *fs);
void unref_fs(struct FSRecord *fs);

struct FSRecord *get_root_fs();
struct FSRecord *new_fs_record(void);
struct FSRecord *get_fs_record(int idx);
void put_fs_record(struct FSRecord *fs);

struct FDRecord *new_fd_record(struct ClientRecord *client);
struct FDRecord *client_fd(struct ClientRecord *client, int fd);
int del_fd_record(struct ClientRecord *client, int fd);

int simple_forward(ipc_struct_t *ipc_struct, struct fs_request *fr);

int walk_path_init(struct walk_path *walk, int client_id, int dirfd,
		   const char *pathname);
int fsm_walk(struct walk_path *walk, struct fs_request *fr,
	     fsm_walk_cb_t callback, void *data);
