#include <common/vars.h>
#include <common/kprint.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/util.h>
#include <mm/mm.h>
#include <mm/kmalloc.h>
#include <arch/machine/smp.h>
#include <arch/x2apic.h>
#include <arch/tools.h>
#include <machine.h>
#include <seg.h>
#include <memlayout.h>
#include <irq/ipi.h>

struct per_cpu_info cpu_info[PLAT_CPU_NUM] __attribute__((aligned(64)));
char *cur_cpu_stack;
u32 cur_cpu_id;

void init_per_cpu_info(u32 cpuid)
{
	u32 gdt_tss_lo;
	u32 gdt_tss_hi;
	struct desc_ptr gdt_descr;

	cpu_info[cpuid].cur_syscall_no = 0;
	cpu_info[cpuid].cur_exec_ctx = 0;
	cpu_info[cpuid].fpu_owner = NULL;
	cpu_info[cpuid].fpu_disable = 0;
	cpu_info[cpuid].cpu_id = cpuid;
	cpu_info[cpuid].cpu_status = cpu_run;

	gdt_tss_lo = GDT_TSS_BASE_LO + cpuid * 2;
	gdt_tss_hi = GDT_TSS_BASE_HI + cpuid * 2;

	/* for setting interrupt stack, set up per cpu gdt TSS seg */
	bootgdt[gdt_tss_lo] = (struct segdesc)
				      SEGDESC((u64)&(cpu_info[cpuid].tss),
				      sizeof(cpu_info[cpuid].tss) - 1,
				      SEG_P|SEG_TSS64A);

	bootgdt[gdt_tss_hi] = (struct segdesc)
				      SEGDESCHI((u64)&(cpu_info[cpuid].tss));
	asm volatile ("ltr %0" : : "r" ((u16)(gdt_tss_lo << 3)));

	asm volatile ("wrmsr"::
		      "a" ((u32) ((u64) & cpu_info[cpuid])),
		      "d"((u32) ((u64) & cpu_info[cpuid] >> 32)),
		      "c"(MSR_GSBASE));
	asm volatile ("swapgs");

	gdt_descr.size = sizeof(bootgdt) - 1;
	gdt_descr.address = (u64)&bootgdt;
	asm volatile("lgdt (%0)" : : "r" (&gdt_descr) : "memory");
}

extern char text;
extern char _mp_start;
extern char _mp_start_end;

void enable_smp_cores(void)
{
	u64 mp_code;
	int cpuid;

	for (cpuid = 1; cpuid < PLAT_CPU_NUM; cpuid ++)
	{
		/* check secondary boot size */
		BUG_ON((u64)(&text) < MP_BOOT_ADDR + (u64)(&_mp_start_end) - (u64)(&_mp_start));

		mp_code = phys_to_virt(MP_BOOT_ADDR);
		memmove((void *)mp_code, (void *)(&_mp_start), (u64)(&_mp_start_end) - (u64)(&_mp_start));

		/* Set kernel stack */
		cur_cpu_stack = (char *)kmalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
		cur_cpu_id = cpuid;

		/* should use lapic to pass the physical boot address */
		/* XXX: Should get the real hwid and pass it to x2apic_sipi */
		asm volatile("mfence");
		x2apic_sipi(cpuid, (u64)virt_to_phys((void *)mp_code));
		kdebug("[SMP] send sipi to core %d\n", cpuid);
		while (cpu_info[cpuid].cpu_status != cpu_run);
		kinfo("[SMP] CPU %d is running\n", cpuid);
		/* check target cpu status */
	}

	init_ipi_data();
}

inline u32 smp_get_cpu_id(void)
{
	u32 cpuid;

	/*
	 * %c: Require a constant operand and
	 * print the constant expression with no punctuation.
	 *
	 * We do not use offsetof since it cannot be used in
	 * irq_entry.S.
	 */
	asm volatile ("mov %%gs:%c1, %0"
		      : "=r"(cpuid)
		      : "i"(OFFSET_LOCAL_CPU_ID)
		      : "memory");
	return cpuid;
}

void smp_print_status(u32 cpuid)
{
	/* TODO */
}
