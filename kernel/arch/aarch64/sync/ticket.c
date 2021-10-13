#include <common/types.h>
#include <common/errno.h>
#include <common/macro.h>
#include <common/lock.h>
#include <common/kprint.h>
#include <arch/sync.h>

#include "ticket.h"

int lock_init(struct lock *l)
{
	struct lock_impl *lock = (struct lock_impl *)l;
	BUG_ON(!lock);
	/* Initialize ticket lock */
	lock->owner = 0;
	lock->next = 0;
        smp_wmb();
	return 0;
}

void lock(struct lock *l)
{
        struct lock_impl *lock = (struct lock_impl *)l;
        u32 lockval = 0, newval = 0, ret = 0;

        BUG_ON(!lock);
        asm volatile(
                /* TODO Prefetch need be tested */
                "       prfm    pstl1strm, %3\n"
                "1:     ldaxr   %w0, %3\n"
                "       add     %w1, %w0, #0x1\n"
                "       stxr    %w2, %w1, %3\n"
                "       cbnz    %w2, 1b\n"
                "2:     ldar    %w2, %4\n"
                "       cmp     %w0, %w2\n"
                "       b.ne    2b\n"
                : "=&r"(lockval), "=&r" (newval), "=&r"(ret), "+Q" (lock->next)
                : "Q" (lock->owner)
                : "memory");
}

int try_lock(struct lock *l)
{
        struct lock_impl *lock = (struct lock_impl *)l;
        u32 lockval = 0, newval = 0, ret = 0, ownerval = 0;

        BUG_ON(!lock);
        asm volatile(
                /* TODO Prefetch need be tested */
                "       prfm    pstl1strm, %4\n"
                "       ldaxr   %w0, %4\n"
                "       ldar    %w3, %5\n"
                "       add     %w1, %w0, #0x1\n"
                "       cmp     %w0, %w3\n"
                "       b.ne    1f\n"
                "       stxr    %w2, %w1, %4\n"
                "       cbz     %w2, 2f\n"
                "1:     mov     %w2, #0xffffffffffffffff\n" /* fail */
                "       b       3f\n"
                "2:     mov     %w2, #0x0\n" /* success */
                "       dmb     ish\n" /* store -> load/store barrier */
                "3:\n"
                : "=&r"(lockval), "=&r" (newval), "=&r"(ret),
                  "=&r"(ownerval), "+Q" (lock->next)
                : "Q" (lock->owner)
                : "memory");
        return ret;
}

void unlock(struct lock *l)
{
        struct lock_impl *lock = (struct lock_impl *)l;

        BUG_ON(!lock);
        smp_mb();       /* load, store -> store barrier may use stlr here */
        lock->owner = lock->owner + 1;
}

int is_locked(struct lock *l)
{
        struct lock_impl *lock = (struct lock_impl *)l;

        return lock->owner < lock->next;
}
