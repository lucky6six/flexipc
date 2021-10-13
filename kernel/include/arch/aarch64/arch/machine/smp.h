#pragma once

#include <common/vars.h>
#include <machine.h>
#include <common/types.h>

enum cpu_state {
	cpu_hang = 0,
	cpu_run = 1,
	cpu_idle = 2
};

struct per_cpu_info {
	/* struct thread *fpu_owner */
	void *fpu_owner;
	u32 fpu_disable;

} __attribute__((aligned(64)));

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];

extern volatile char cpu_status[PLAT_CPU_NUM];

void enable_smp_cores(void *addr);
void init_per_cpu_info(void);
struct per_cpu_info *get_per_cpu_info(void);
u32 smp_get_cpu_id(void);
u64 smp_get_mpidr(void);
void smp_print_status(u32 cpuid);
