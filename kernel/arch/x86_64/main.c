#include <common/types.h>
#include <common/kprint.h>
#include <common/lock.h>
#include <io/uart.h>
#include <mm/mm.h>
#include <sched/sched.h>
#include <sched/fpu.h>
#include <arch/machine/smp.h>
#include <arch/machine/machine.h>
#include <irq/irq.h>
#include <irq/timer.h>
#include <object/thread.h>

/* Global big kernel lock */
struct lock big_kernel_lock;

void run_test(void);

void main(u64 mbmagic, u64 mbaddr)
{
	u32 ret = 0;

	uart_init();
	kinfo("[ChCore] uart init finished\n");

	init_per_cpu_info(0);	/* should passed from boot? */
	kinfo("[ChCore] per cpu info init finished\n");

	arch_interrupt_init();
	timer_init();
	kinfo("[ChCore] interrupt init finished\n");

	/* Configure the syscall entry */
	arch_syscall_init();
	kinfo("[ChCore] SYSCALL init finished\n");

	mm_init((void*)mbaddr);
	kinfo("[ChCore] mm init finished\n");

	/* Configure CPU features: setting per_core registers */
	arch_cpu_init();

	/* Init big kernel lock */
	ret = lock_init(&big_kernel_lock);
	kinfo("[ChCore] lock init finished\n");
	BUG_ON(ret != 0);

	// sched_init(&pbrr);
	sched_init(&rr);
	kinfo("[ChCore] sched init finished\n");

	enable_smp_cores();
	kinfo("[ChCore] boot smp\n");

	/* Test should be done when IRQ is not enabled */
#ifdef KERNEL_TEST
	kinfo("[ChCore] kernel tests start\n");
	run_test();
	kinfo("[ChCore] kernel tests done\n");
#endif

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	disable_fpu_usage();
#endif

	/* Create initial thread like init-process in Linux */
	create_root_thread();
	kinfo("[ChCore] create initial thread done\n");

	sched();
	eret_to_thread(switch_context());
	BUG("Should never be here!\n");
}

/* For booting smp cores */
void secondary_start(u32 cpuid)
{
	init_per_cpu_info(cpuid);
	arch_interrupt_init_per_cpu();
	timer_init();

	/* Configure the syscall entry */
	arch_syscall_init();
	/* Configure CPU features: setting per_core registers */
	arch_cpu_init();

	/* Test should be done when IRQ is not enabled */
#ifdef KERNEL_TEST
	run_test();
#endif

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	disable_fpu_usage();
#endif

	/* Run the scheduler on the current CPU core */
	sched();
	eret_to_thread(switch_context());
	BUG("Should never be here!\n");
}
