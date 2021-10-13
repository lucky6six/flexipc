#include <common/types.h>
#include <common/errno.h>
#include <common/macro.h>
#include <common/lock.h>
#include <common/kprint.h>
#include <arch/sync.h>

/* Simple RWLock */

int rwlock_init(struct rwlock *rwlock)
{
	if (rwlock == 0)
		return -EINVAL;
	rwlock->lock = 0;
	return 0;
}

void read_lock(struct rwlock *rwlock)
{
	s32 old;
	do {
		old = atomic_fetch_add_32((s32 *)&rwlock->lock, 1);
	} while (old & 0x80000000);
	COMPILER_BARRIER();
}

int read_try_lock(struct rwlock *rwlock)
{
	s32 old;

	old = atomic_fetch_add_32((s32 *)&rwlock->lock, 1);
	COMPILER_BARRIER();
	return (old & 0x80000000)? -1: 0;
}

void read_unlock(struct rwlock *rwlock)
{
	COMPILER_BARRIER();
	atomic_fetch_add_32(&rwlock->lock, -1);
}

void write_lock(struct rwlock *rwlock)
{
	while(compare_and_swap_32((s32 *)&rwlock->lock, 0, 0x80000000) != 0)
		CPU_PAUSE();
	COMPILER_BARRIER();
}

int write_try_lock(struct rwlock *rwlock)
{
	int ret = 0;

	if(compare_and_swap_32((s32 *)&rwlock->lock, 0, 0x80000000) != 0)
		ret = -1;
	COMPILER_BARRIER();
	return ret;
}


void write_unlock(struct rwlock *rwlock)
{
	COMPILER_BARRIER();
	rwlock->lock = 0;
}
