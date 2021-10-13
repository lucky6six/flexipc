#pragma once

#ifndef __ASM__
#include <machine.h>
#include <common/types.h>

struct tss {
	u8 reserved0[4];
	u64 rsp[3];
	u64 ist[8];
	u8 reserved1[10];
	u16 iomba;
	u8 iopb[0];
} __attribute__ ((packed, aligned(16)));
#endif

/*
 * The offset in the per_cpu struct, i.e., struct per_cpu_info.
 * The base addr of this struct is stored in GS register.
 *
 * IMPORTANT: modify the following offset values after
 * modifing struct per_cpu_info.
 */
#define OFFSET_CURRENT_SYSCALL_NO	0
#define OFFSET_CURRENT_EXEC_CTX		8
#define OFFSET_CURRENT_FPU_OWNER	16
#define OFFSET_FPU_DISABLE		24
#define OFFSET_LOCAL_CPU_ID		28
#define OFFSET_CURRENT_TSS		32
	/* offset for tss->rsp[0]: 32 + 4 */
	#define OFFSET_CURRENT_INTERRUPT_STACK 36

#ifndef __ASM__
struct per_cpu_info {
	/* Used for handling syscall (saving rax, i.e., syscall_num) */
	u64 cur_syscall_no;
	/*
	 * Used for handling syscall to find the execution context of
	 * current thread.
	 */
	u64 cur_exec_ctx;

	/* struct thread *fpu_owner */
	void *fpu_owner;
	u32 fpu_disable;

	/* Local CPU ID */
	u32 cpu_id;

	/* per-CPU TSS area: for setting interrupt stack */
	struct tss tss;

	volatile u8 cpu_status;

	char pad[pad_to_cache_line(sizeof(u64) * 2 +
				   sizeof(void*) +
				   sizeof(u32) * 2 +
				   sizeof(struct tss) +
				   sizeof(u8))];
} __attribute__((packed, aligned(64)));

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];

enum cpu_state {
	cpu_hang = 0,
	cpu_run = 1,
	cpu_idle = 2
};

extern volatile char cpu_status[PLAT_CPU_NUM];

void enable_smp_cores(void);
u32 smp_get_cpu_id(void);
void smp_print_status(u32 cpuid);
void init_per_cpu_info(u32 cpuid);

#endif
