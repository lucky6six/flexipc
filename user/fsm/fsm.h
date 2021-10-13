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
#include "ops.h"

int init_fsm(void);

int fsm_mount_fs(const char *path, const char *mount_point,
		 bool builtin_binary);
int fsm_umount_fs(void);

int fsm_match_fs(const char *path);
int fsm_match_or_record_client(u64 client_cap);

void fsm_dispatch(ipc_msg_t *ipc_msg, u64 client_badge);
