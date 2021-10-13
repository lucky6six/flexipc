#include <common/types.h>
#include <io/uart.h>
#include <ipc/connection.h>
#include <mm/uaccess.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <common/kprint.h>
#include <common/debug.h>
#include "syscall_num.h"

#if HOOKING_SYSCALL == ON
void hook_syscall(long n)
{
	if ((n != SYS_putc) && (n != SYS_getc) && (n != SYS_yield)
	    && (n != SYS_handle_brk))
		kinfo("[SYSCALL TRACING] hook_syscall num: %ld\n", n);
}
#endif

/* Placeholder for system calls that are not implemented */
void sys_null_placeholder(long arg)
{
	BUG("Invoke non-implemented syscall\n");
}

void sys_putc(char ch)
{
	uart_send((unsigned int)ch);
}

u32 sys_getc(void)
{
	return nb_uart_recv();
}

/* A debugging function which can be used for adding trace points in apps */
void sys_debug_log(long arg)
{
	kinfo("%s: %ld\n", __func__, arg);
}

/* RAMDISK symbol */
extern const char binary_ramdisk_cpio_bin_start;
extern size_t binary_ramdisk_cpio_bin_size;

extern const char binary_fsm_cpio_bin_start;
extern size_t binary_fsm_cpio_bin_size;

extern const char binary_tmpfs_cpio_bin_start;
extern size_t binary_tmpfs_cpio_bin_size;

#define FSM_CPIO     0
#define TMPFS_CPIO   1
#define RAMDISK_CPIO 2

#define QUERY_SIZE   0
#define LOAD_CPIO    1

/*
 * The calling process should query the cpio size first and then prepare
 * a enough space for loading.
 */
u64 sys_fs_load_cpio(u64 wh_cpio, u64 op, u64 vaddr)
{
	//struct pmobject *cpio_pmo;
	//struct vmspace *vmspace;
	//int cpio_pmo_cap, ret;
	//size_t len;

	int ret;
	u64 size = 0;
	u64 start = 0;

	BUG_ON(wh_cpio > RAMDISK_CPIO);

	switch (wh_cpio) {
	case FSM_CPIO: {
		size = binary_fsm_cpio_bin_size;
		start = (u64)&binary_fsm_cpio_bin_start;
		break;
	}
	case TMPFS_CPIO: {
		size = binary_tmpfs_cpio_bin_size;
		start = (u64)&binary_tmpfs_cpio_bin_start;
		break;
	}
	case RAMDISK_CPIO: {
		size = binary_ramdisk_cpio_bin_size;
		start = (u64)&binary_ramdisk_cpio_bin_start;
		break;
	}
	default: {
		kinfo("No required CPIO!\n");
		return -EINVAL;
	}
	}

	if (op == QUERY_SIZE)
		return size;

	BUG_ON(op != LOAD_CPIO);
	ret = copy_to_user((void *)vaddr, (void *)start, size);

	return ret;
}

const void *syscall_table[NR_SYSCALL] = {
	[0 ... NR_SYSCALL - 1] = sys_null_placeholder,
	[SYS_putc] = sys_putc,
	[SYS_getc] = sys_getc,
	[SYS_yield] = sys_yield,
	[SYS_thread_exit] = sys_thread_exit,
	[SYS_create_device_pmo] = sys_create_device_pmo,
	[SYS_create_pmo] = sys_create_pmo,
	[SYS_map_pmo] = sys_map_pmo,
	[SYS_unmap_pmo] = sys_unmap_pmo,
	[SYS_create_thread] = sys_create_thread,
	[SYS_create_cap_group] = sys_create_cap_group,
	[SYS_register_server] = sys_register_server,
	[SYS_register_client] = sys_register_client,
	[SYS_ipc_register_cb_return] = sys_ipc_register_cb_return,
	[SYS_ipc_call] = sys_ipc_call,
	[SYS_ipc_return] = sys_ipc_return,
	[SYS_ipc_send_cap] = sys_ipc_send_cap,
	// [SYS_debug_va] = sys_debug_va,
	[SYS_debug_va] = sys_debug_log,
	[SYS_cap_copy_to] = sys_cap_copy_to,
	[SYS_cap_copy_from] = sys_cap_copy_from,
	[SYS_set_affinity] = sys_set_affinity,
	[SYS_get_affinity] = sys_get_affinity,

	[SYS_get_pmo_paddr] = sys_get_pmo_paddr,
	[SYS_get_phys_addr] = sys_get_phys_addr,

	[SYS_create_pmos] = sys_create_pmos,
	[SYS_map_pmos] = sys_map_pmos,
	[SYS_write_pmo] = sys_write_pmo,
	[SYS_read_pmo] = sys_read_pmo,
	[SYS_transfer_caps] = sys_transfer_caps,

	[SYS_handle_brk] = sys_handle_brk,
	[SYS_handle_mmap] = sys_handle_mmap,
	[SYS_handle_munmap] = sys_handle_munmap,
	[SYS_mprotect] = sys_handle_mprotect,

	[SYS_shmget] = sys_shmget,
	[SYS_shmat] = sys_shmat,
	[SYS_shmdt] = sys_shmdt,
	[SYS_shmctl] = sys_shmctl,

	[SYS_handle_fmap] = sys_handle_fmap,
	[SYS_translate_aarray] = sys_translate_aarray,

	[SYS_register_recycle] = sys_register_recycle,
	[SYS_exit_group] = sys_exit_group,
	[SYS_cap_group_recycle] = sys_cap_group_recycle,

	[SYS_irq_register] = sys_irq_register,
	[SYS_irq_wait] = sys_irq_wait,
	[SYS_irq_ack] = sys_irq_ack,

	[SYS_create_notifc] = sys_create_notifc,
	[SYS_wait] = sys_wait,
	[SYS_notify] = sys_notify,
	[SYS_clock_gettime] = sys_clock_gettime,
	[SYS_clock_nanosleep] = sys_clock_nanosleep,

	/* TMP FS */
	[SYS_fs_load_cpio] = sys_fs_load_cpio,

	/* Perf */
	[SYS_perf_start] = sys_perf_start,
	[SYS_perf_end] = sys_perf_end,
	[SYS_perf_null] = sys_perf_null,

	[SYS_top] = sys_top,
};
