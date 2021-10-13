#include <common/vars.h>
#include <common/kprint.h>
#include <mm/mm.h>
#include <arch/machine/smp.h>
#include <common/types.h>
#include <arch/tools.h>
#include <irq/ipi.h>

volatile char cpu_status[PLAT_CPU_NUM] = { cpu_hang };

struct per_cpu_info cpu_info[PLAT_CPU_NUM] __attribute__((aligned(64)));

void enable_smp_cores(void *addr)
{
	int i = 0;
	long *secondary_boot_flag;

	/* Set current cpu status */
	cpu_status[smp_get_cpu_id()] = cpu_run;
	secondary_boot_flag = (long *)phys_to_virt(addr);
	for (i = 0; i < PLAT_CPU_NUM; i++) {
		secondary_boot_flag[i] = 1;
		flush_dcache_area((u64) secondary_boot_flag,
				  (u64) sizeof(u64) * PLAT_CPU_NUM);
		asm volatile ("dsb sy");
		while (cpu_status[i] == cpu_hang)
		;
		kinfo("CPU %d is active\n", i);
	}
	/* wait all cpu to boot */
	kinfo("All %d CPUs are active\n", PLAT_CPU_NUM);
	init_ipi_data();
}

inline u32 smp_get_cpu_id(void)
{
	u64 cpuid = 0;

	asm volatile ("mrs %0, tpidr_el1":"=r" (cpuid));
	return (u32) cpuid;
}

inline struct per_cpu_info *get_per_cpu_info(void)
{
	struct per_cpu_info *info;

	info = &cpu_info[smp_get_cpu_id()];
	return info;
}

void init_per_cpu_info(void)
{
	struct per_cpu_info *info;

	info = get_per_cpu_info();

	info->fpu_owner = NULL;
	info->fpu_disable = 0;
}

u64 smp_get_mpidr(void)
{
	u64 mpidr = 0;

	asm volatile ("mrs %0, mpidr_el1":"=r" (mpidr));
	return mpidr;
}

void smp_print_status(u32 cpuid)
{
	/* TODO */
}
