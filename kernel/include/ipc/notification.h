/* This file defines ds and interfaces related with asynchronous notification */
#pragma once
#include <common/lock.h>
#include <object/thread.h>
#include <common/types.h>
#include <posix/time.h>

struct notification {
	u32 not_delivered_notifc_count;
	u32 waiting_threads_count;
	struct list_head waiting_threads;
	/*
	 * notifc_lock protects counter and list of waiting threads,
	 * including the internal states of waiting threads.
	 */
	struct lock notifc_lock;
};

void init_notific(struct notification *notifc);
s32 wait_notific(struct notification *notifc, bool is_block,
		 struct timespec *timeout);
s32 signal_notific(struct notification *notifc, int enable_direct_switch);

s32 sys_create_notifc();
s32 sys_wait(u32 notifc_cap, bool is_block, struct timespec *timeout);
s32 sys_notify(u32 notifc_cap);
