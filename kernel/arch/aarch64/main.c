#include <sched/sched.h>
#include <sched/fpu.h>
//#include <ipc/connection.h>
#include <common/kprint.h>
#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/lock.h>
#include <arch/machine/smp.h>
#include <arch/machine/pmu.h>
#include <mm/mm.h>
#include <io/uart.h>
/* In /kernel/include/plat/xxx/machine.h */
#include <machine.h>
#include <irq/irq.h>
#include <object/thread.h>

ALIGN(STACK_ALIGNMENT)
char kernel_stack[PLAT_CPU_NUM][KERNEL_STACK_SIZE];
struct lock big_kernel_lock;

/* Kernel Test */
void run_test(void);

void main(void *addr)
{
	u32 ret = 0;

	/* Init uart */
	uart_init();
	kinfo("[ChCore] uart init finished\n");

	/* Init exception vector */
	arch_interrupt_init();
	timer_init();
	kinfo("[ChCore] interrupt init finished\n");

	/* Init mm */
	// TODO: pass mem info from bootloader to mm_init
	mm_init(NULL);
	kinfo("[ChCore] mm init finished\n");

	/* Init big kernel lock */
	ret = lock_init(&big_kernel_lock);
	kinfo("[ChCore] lock init finished\n");
	BUG_ON(ret != 0);

	/* Enable PMU by setting PMCR_EL0 register */
	pmu_init();
	kinfo("[ChCore] pmu init finished\n");

	/* Init scheduler with specified policy */
	/* XXX perf: indirect call may hurt the performance */
	sched_init(&rr);
	// sched_init(&pbrr);
	kinfo("[ChCore] sched init finished\n");

	/* Init per_cpu info */
	init_per_cpu_info();

	/* Other cores are busy looping on the addr, wake up those cores */
	enable_smp_cores(addr);
	kinfo("[ChCore] boot multicore finished\n");

#ifdef KERNEL_TEST
	kinfo("[ChCore] kernel tests start\n");
	run_test();
	kinfo("[ChCore] kernel tests done\n");
#endif

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	disable_fpu_usage();
#endif

	/* Create initial thread here, which use the `init.bin` */
	create_root_thread();
	kinfo("[ChCore] create initial thread done\n");

	/* Leave the scheduler to do its job */
	sched();

	/* Context switch to the picked thread */
	eret_to_thread(switch_context());

	/* Should provide panic and use here */
	BUG("[FATAL] Should never be here!\n");
}

void secondary_start(void)
{
	u32 cpuid = smp_get_cpu_id();

	/* Set the cpu status to inform the primary cpu */
	cpu_status[cpuid] = cpu_run;

#ifdef KERNEL_TEST
	run_test();
#endif
	arch_interrupt_init_per_cpu();
	timer_init();
	pmu_init();

	/* Init per_cpu info */
	init_per_cpu_info();

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	disable_fpu_usage();
#endif

	sched();
	eret_to_thread(switch_context());
}
