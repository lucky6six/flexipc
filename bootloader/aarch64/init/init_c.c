#include "printf.h"
#include "uart.h"
#include "boot.h"
#include "image.h"
#include <lib/efi.h>

ALIGN(16) VISIBLE
char boot_cpu_stack[CONFIG_MAX_NUM_CPUS][BIT(PAGE_BITS)] = { 0 };
long secondary_boot_flag[CONFIG_MAX_NUM_CPUS + 1] = { 0 };


unsigned long get_sctlr_el1();

void init_c(void)
{
	/* Initialize UART before enabling MMU. */
	early_uart_init();

	/* Initialize Boot Page Table. */
	printf("[BOOT] Install boot page table\r\n");
	init_boot_pt();

#if 0
	printf("current el is %d\r\n", get_current_el());
	printf("sctlr.span is %d\r\n", (get_sctlr_el1() & (1<<23)) == 0 ? 0 : 1);

	unsigned long sctlr_el1 = get_sctlr_el1();
	printf("sctlr_el1: 0x%lx, current el is %d\n", sctlr_el1,
	      get_current_el());
#endif
	
	/* Enable MMU. */
	printf("[BOOT] Enable el1 MMU\r\n");
	el1_mmu_activate();


	/* Call Kernel Main. */
	printf("[BOOT] Jump to kernel main at 0x%lx\r\n\n\n\n\n", start_kernel);
	start_kernel(secondary_boot_flag);
	
	printf("[BOOT] Should not be here!\r\n");
}

void secondary_init_c(int cpuid)
{
	el1_mmu_activate();
	secondary_cpu_boot(cpuid);
}
