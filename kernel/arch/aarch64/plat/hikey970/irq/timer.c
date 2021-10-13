#include <irq/timer.h>
#include <machine.h>
#include <lib/printk.h>
#include <common/types.h>
#include <common/kprint.h>
#include <arch/machine/smp.h>
#include <sched/sched.h>

u64 cntv_tval;
u64 tick_per_us;
u64 cntv_init;
u64 cntv_freq;

void plat_timer_init(void)
{
	u64 timer_ctl = 0x1;

	/* Get the frequency and init count */
	asm volatile ("mrs %0, cntpct_el0":"=r" (cntv_init));
	kdebug("timer init cntpct_el0 = %lu\n", cntv_init);
	asm volatile ("mrs %0, cntfrq_el0":"=r" (cntv_freq));
	kdebug("timer init cntfrq_el0 = %lu\n", cntv_freq);

	/* Calculate the tv */
	cntv_tval = (cntv_freq * TICK_MS / 1000);
	tick_per_us = cntv_freq / 1000 / 1000;
	// kinfo("CPU freq %lu, set timer %lu\n", cntv_freq, cntv_tval);

	/* Set the timervalue so that we will get a timer interrupt */
	asm volatile ("msr cntv_tval_el0, %0"::"r" (cntv_tval));

	/* Set the control register */
	asm volatile ("msr cntv_ctl_el0, %0"::"r" (timer_ctl));
}

void plat_set_next_timer(u64 tick_delta)
{
	asm volatile ("msr cntv_tval_el0, %0"::"r" (tick_delta));
}

void plat_handle_timer_irq(u64 tick_delta)
{
	/* Set the timervalue */
	asm volatile ("msr cntv_tval_el0, %0"::"r" (tick_delta));
}

void plat_disable_timer(void)
{
	u64 timer_ctl = 0x0;

	asm volatile ("msr cntv_ctl_el0, %0"::"r" (timer_ctl));
}

void plat_enable_timer(void)
{
	u64 timer_ctl = 0x1;

	asm volatile ("msr cntv_tval_el0, %0"::"r" (cntv_tval));
	asm volatile ("msr cntv_ctl_el0, %0"::"r" (timer_ctl));
}

/* Return the mono time using nano-seconds */
u64 plat_get_mono_time(void)
{
	u64 cur_cnt = 0;
	asm volatile ("mrs %0, cntpct_el0":"=r" (cur_cnt));
	return (cur_cnt - cntv_init) * NS_IN_US / tick_per_us;
}

u64 plat_get_current_tick(void)
{
	u64 cur_cnt = 0;
	asm volatile ("mrs %0, cntpct_el0":"=r" (cur_cnt));
	return cur_cnt;
}
