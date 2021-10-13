#include <irq/timer.h>
#include <machine.h>
#include <common/kprint.h>
#include <common/types.h>
#include <arch/machine/smp.h>
#include <sched/sched.h>
#include <arch/tools.h>

u64 cntv_tval;
u64 tick_per_us;
u64 cntv_init;
u64 cntv_freq;

/* Per core IRQ SOURCE MMIO address */
u64 core_timer_irqcntl[PLAT_CPU_NUM] = {
	CORE0_TIMER_IRQCNTL,
	CORE1_TIMER_IRQCNTL,
	CORE2_TIMER_IRQCNTL,
	CORE3_TIMER_IRQCNTL
};

void plat_timer_init(void)
{
	u64 count_down = 0;
	u64 timer_ctl = 0;
	u32 cpuid = smp_get_cpu_id();

#if 0
	u64 cur_cnt = 0;
	/* Why GPU here? Still need to read the manual */
	cur_cnt = get32(BCM2835_GPU_TIMER_CLO);
	cur_cnt = cur_cnt + DEBUG_INTERVAL;
	put32(BCM2835_GPU_TIMER_C1, cur_cnt);
	put32(BCM2835_IRQ_ENABLE1, SYSTEM_TIMER_IRQ_1);
#endif

	/* Since QEMU only emulate the generic timer, we use the generic timer here */
	asm volatile ("mrs %0, cntpct_el0":"=r" (cntv_init));
	kdebug("timer init cntpct_el0 = %lu\n", cntv_init);
	asm volatile ("mrs %0, cntfrq_el0":"=r" (cntv_freq));
	kdebug("timer init cntfrq_el0 = %lu\n", cntv_freq);

	/* Calculate the tv */
	cntv_tval = (cntv_freq * TICK_MS / 1000);
	tick_per_us = cntv_freq / 1000 / 1000;
	// kinfo("CPU freq %lu, set timer %lu\n", cntv_freq, cntv_tval);

	/* set the timervalue here */
	asm volatile ("msr cntv_tval_el0, %0"::"r" (cntv_tval));
	asm volatile ("mrs %0, cntv_tval_el0":"=r" (count_down));
	kdebug("timer init cntv_tval_el0 = %lu\n", count_down);

	put32(core_timer_irqcntl[cpuid], INT_SRC_TIMER3);

	/* Set the control register */
	timer_ctl = 0 << 1 | 1;	/* IMASK = 0 ENABLE = 1 */
	asm volatile ("msr cntv_ctl_el0, %0"::"r" (timer_ctl));
	asm volatile ("mrs %0, cntv_ctl_el0":"=r" (timer_ctl));
	kdebug("timer init cntv_ctl_el0 = %lu\n", timer_ctl);
	/* enable interrupt controller */
	return;
}

void plat_set_next_timer(u64 tick_delta)
{
	asm volatile ("msr cntv_tval_el0, %0"::"r" (tick_delta));
}

void plat_handle_timer_irq(u64 tick_delta)
{
	asm volatile ("msr cntv_tval_el0, %0"::"r" (tick_delta));
#if 0
	unsigned int cur_cnt = 0;
	unsigned int cur_cntvct = 0;

	asm volatile ("mrs %0, cntpct_el0":"=r" (cur_cnt));
	kdebug("Handle timer IRQ cntpct_el0 = %lu\n", cur_cnt);
	kdebug("Tick\n");
	asm volatile ("mrs %0, cntvct_el0":"=r" (cur_cntvct));
	kdebug("Handle timer IRQ cntvct = %lu\n", cur_cntvct);
	cur_cnt = get32(BCM2835_GPU_TIMER_CLO);
	kdebug("handle timer cur_cnt = %lx\n", cur_cnt);
	cur_cnt = cur_cnt + DEBUG_INTERVAL;
	put32(BCM2835_GPU_TIMER_C1, cur_cnt);
	put32(BCM2835_GPU_TIMER_CS, SYSTEM_TIMER_IRQ_1);
#endif
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
