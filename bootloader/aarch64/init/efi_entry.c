#include <boot.h>
#include <lib/efi.h>		/* in kernel/include/lib/efi.h */
#include <uart.h>
#include <printf.h>

void *simple_memcpy(void *dst, const void *src, size_t len);
void simple_memset(char *dst, char ch, unsigned long len);

unsigned long efi_entry_c(void *handle, efi_system_table_t *sys_table,
			  unsigned long *image_addr)
{
	efi_status_t status;
#if 0
	unsigned long image_size = 0;
	unsigned long reserve_size = 0;
	unsigned long reserve_addr = 0;
	unsigned long dram_base = 0;
#endif
	efi_sys_table = sys_table;

	if (sys_table->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE) {
		efi_error("efi tale signature check fails\n",
			  (efi_status_t) sys_table->hdr.signature);
	}

	early_uart_init();
	printf("\r\n[EFI] In efi_entry\r\n");

	status = efi_get_phy_memory_map(sys_table);
	if (status != EFI_SUCCESS)
		efi_error("get physical memory map failed", status);
	/* Move memory map to a continuous space */
	unsigned long dram_base = 0;
	status = get_dram_base(sys_table, &dram_base);
	printf("get dram_base status %d dram_base %lx\r\n", status, dram_base);
	move_memmap(dram_base);

#if 0
	status = get_dram_base(sys_table, &dram_base);
	printf("get dram_base status %d dram_base %lx\r\n", status, dram_base);
	status = handle_kernel_image(sys_table, image_addr, &image_size,
				     &reserve_addr,
				     &reserve_size, dram_base, 0);
#endif
	status = exit_boot(sys_table, handle);
	if (status != 0)
		efi_error("exit_boot failed", status);
	return (unsigned long)*image_addr;
}

/*
 * With EFI, kernel image is automatically loaded by EFI at first.
 * So, we move the kernel image to some fixed target_phys_address.
 */
void *move_kernel_image(void *phy_addr)
{
	void *target_addr = (void *)TEXT_OFFSET;
	size_t kernel_size = &img_end - &img_start;
	unsigned long kernel_bss_size;

	// TODO: solve potential overlapping
	if (!((target_addr + kernel_size <= phy_addr) ||
	      (phy_addr + kernel_size < target_addr))) {
		efi_error("address overlapping in move kernel image\n\r",
			  (efi_status_t) phy_addr);
	}

	simple_memcpy(target_addr, phy_addr, kernel_size);
	printf("[EFI] Move kernel image from 0x%lx to 0x%lx size 0x%lx\r\n",
	       phy_addr, target_addr, kernel_size);

	/* Clear the bss section */
	kernel_bss_size = (&_bss_end - &_bss_start);
	simple_memset((char *)&_bss_start, 0, kernel_bss_size);
	printf("[EFI] Move kernel finished\r\n");

	return target_addr;
}
