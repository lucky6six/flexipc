#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/type.h>
#include <chcore/proc.h>
#include <syscall_arch.h>

void usys_putc(char ch)
{
	chcore_syscall6(CHCORE_SYS_putc, ch, 0, 0, 0, 0, 0);
}

u32 usys_getc(void)
{
	return (u32) chcore_syscall6(CHCORE_SYS_getc, 0, 0, 0, 0, 0, 0);
}

void usys_exit(u64 ret)
{
	chcore_syscall6(CHCORE_SYS_thread_exit, ret, 0, 0, 0, 0, 0);
}

u64 usys_yield(void)
{
	return chcore_syscall6(CHCORE_SYS_yield, 0, 0, 0, 0, 0, 0);
}

int usys_create_device_pmo(u64 paddr, u64 size)
{
	return chcore_syscall6(CHCORE_SYS_create_device_pmo, paddr, size, 0, 0, 0, 0);
}

int usys_create_pmo(u64 size, u64 type)
{
	return chcore_syscall6(CHCORE_SYS_create_pmo, size, type, 0, 0, 0, 0);
}

int usys_map_pmo(u64 cap_group_cap, u64 pmo_cap, u64 addr, u64 rights)
{
	return chcore_syscall6(CHCORE_SYS_map_pmo, cap_group_cap, pmo_cap, addr, rights,
		       0, 0);
}

int usys_unmap_pmo(u64 cap_group_cap, u64 pmo_cap, u64 addr)
{
       return chcore_syscall6(CHCORE_SYS_unmap_pmo, cap_group_cap, pmo_cap, addr,
		      0, 0, 0);
}

int usys_set_affinity(u64 thread_cap, s32 aff)
{
	return chcore_syscall6(CHCORE_SYS_set_affinity, thread_cap, (u64)aff, 0, 0, 0, 0);
}

s32 usys_get_affinity(u64 thread_cap)
{
	return chcore_syscall6(CHCORE_SYS_get_affinity, thread_cap, 0, 0, 0, 0, 0);
}

int usys_get_pmo_paddr(u64 pmo_cap, u64 *buf)
{
	return chcore_syscall6(CHCORE_SYS_get_pmo_paddr, pmo_cap, (u64)buf, 0, 0, 0, 0);
}

int usys_get_phys_addr(void *vaddr, u64 *paddr)
{
	return chcore_syscall6(CHCORE_SYS_get_phys_addr, (u64)vaddr, (u64)paddr, 0, 0, 0, 0);
}

int usys_create_thread(u64 thread_args_p)
{
	return chcore_syscall1(CHCORE_SYS_create_thread, thread_args_p);
}

int usys_create_cap_group(u64 badge, char *name, u64 name_len)
{
	return chcore_syscall6(CHCORE_SYS_create_cap_group, badge, (u64)name,
			       name_len, 0, 0, 0);
}

u64 usys_register_server(u64 callback, u64 register_thread_cap)
{
	return chcore_syscall6(CHCORE_SYS_register_server, callback,
		       register_thread_cap, 0, 0, 0, 0);
}

u32 usys_register_client(u32 server_cap, u64 vm_config_ptr)
{
	return chcore_syscall6(CHCORE_SYS_register_client, server_cap, vm_config_ptr,
		       0, 0, 0, 0);
}

/* TODO: ipc data transfer through registers */
/* XXX: all args are passed by registers as stack is modified
 * is shared stack is used*/
u64 usys_ipc_call(u32 conn_cap, u64 ipc_msg_ptr, u64 cap_num)
{
	return chcore_syscall3(CHCORE_SYS_ipc_call, conn_cap, ipc_msg_ptr,
			       cap_num);
}

void usys_ipc_return(u64 ret, u64 cap_num)
{
	chcore_syscall6(CHCORE_SYS_ipc_return, ret, cap_num, 0, 0, 0, 0);
}

void usys_ipc_register_cb_return(u64 server_thread_cap,
				 u64 server_shm_addr)
{
	chcore_syscall6(CHCORE_SYS_ipc_register_cb_return, server_thread_cap,
		server_shm_addr, 0, 0, 0, 0);
}

u64 usys_ipc_send_cap(u32 conn_cap, u32 send_cap)
{
	return chcore_syscall6(CHCORE_SYS_ipc_send_cap, conn_cap, send_cap, 0, 0, 0, 0);
}

void usys_debug_va(u64 va)
{
	chcore_syscall6(CHCORE_SYS_debug_va, va, 0, 0, 0, 0, 0);
}

int usys_cap_copy_to(u64 dest_cap_group_cap, u64 src_slot_id)
{
	return chcore_syscall6(CHCORE_SYS_cap_copy_to, dest_cap_group_cap, src_slot_id,
		       0, 0, 0, 0);
}

int usys_cap_copy_from(u64 src_cap_group_cap, u64 src_slot_id)
{
	return chcore_syscall6(CHCORE_SYS_cap_copy_from, src_cap_group_cap, src_slot_id,
		       0, 0, 0, 0);
}

long usys_fs_load_cpio(u64 wh_cpio, u64 op, void *vaddr)
{
	return chcore_syscall6(CHCORE_SYS_fs_load_cpio,
			       wh_cpio, op, (u64)vaddr, 0, 0, 0);
}

int usys_create_pmos(void *req, u64 cnt)
{
	return chcore_syscall6(CHCORE_SYS_create_pmos, (u64)req, cnt, 0,
		       0, 0, 0);
}

int usys_map_pmos(u64 cap, void *req, u64 cnt)
{
	return chcore_syscall6(CHCORE_SYS_map_pmos, cap, (u64)req, cnt,
		       0, 0, 0);
}

int usys_write_pmo(u64 cap, u64 offset, void *buf, u64 size)
{
	return chcore_syscall6(CHCORE_SYS_write_pmo, cap, offset, (u64)buf, size,
		       0, 0);
}

int usys_read_pmo(u64 cap, u64 offset, void *buf, u64 size)
{
	return chcore_syscall6(CHCORE_SYS_read_pmo, cap, offset, (u64)buf, size,
		       0, 0);
}

int usys_transfer_caps(u64 cap_group, int *src_caps, int nr, int *dst_caps)
{
	return chcore_syscall6(CHCORE_SYS_transfer_caps, cap_group, (u64)src_caps,
		       (u64)nr, (u64)dst_caps, 0, 0);
}

void usys_perf_start(void)
{
	chcore_syscall6(CHCORE_SYS_perf_start, 0, 0, 0, 0, 0, 0);
}

void usys_perf_end(void)
{
	chcore_syscall6(CHCORE_SYS_perf_end, 0, 0, 0, 0, 0, 0);
}

void usys_perf_null(void)
{
	chcore_syscall6(CHCORE_SYS_perf_null, 0, 0, 0, 0, 0, 0);
}

void usys_top(void)
{
	chcore_syscall6(CHCORE_SYS_top, 0, 0, 0, 0, 0, 0);
}

int usys_irq_register(int irq)
{
	return chcore_syscall6(CHCORE_SYS_irq_register, irq, 0, 0, 0, 0, 0);
}

int usys_irq_wait(int irq_cap, bool is_block)
{
	return chcore_syscall6(CHCORE_SYS_irq_wait, irq_cap, is_block, 0, 0, 0, 0);
}

int usys_irq_ack(int irq_cap)
{
	return chcore_syscall6(CHCORE_SYS_irq_ack, irq_cap, 0, 0, 0, 0, 0);
}

int usys_create_notifc()
{
	return chcore_syscall6(CHCORE_SYS_create_notifc, 0, 0, 0, 0, 0, 0);
}

int usys_wait(u32 notifc_cap, bool is_block, void *timeout)
{
	return chcore_syscall6(CHCORE_SYS_wait, notifc_cap, is_block,
			       (u64)timeout, 0, 0, 0);
}

int usys_notify(u32 notifc_cap)
{
	return chcore_syscall6(CHCORE_SYS_notify, notifc_cap, 0, 0, 0, 0, 0);
}

int usys_translate_aarray(vaddr_t *aarray, size_t aarray_size,
			  off_t offset, size_t count, int *pmo_p)
{
	return chcore_syscall6(CHCORE_SYS_translate_aarray, (u64)aarray,
			      aarray_size, offset, count, (u64)pmo_p,
			      0);
}

/* Only used for recycle process */
int usys_register_recycle_thread(int cap, u64 buffer)
{
	return chcore_syscall6(CHCORE_SYS_register_recycle,
			       cap, buffer, 0, 0, 0, 0);
}

int usys_cap_group_recycle(int cap)
{
	return chcore_syscall6(CHCORE_SYS_cap_group_recycle,
			       cap, 0, 0, 0, 0, 0);
}
