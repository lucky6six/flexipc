#pragma once

#include <chcore/defs.h>
#include <chcore/type.h>
#include <stdio.h>

long usys_fs_load_cpio(u64 wh_cpio, u64 op, void *vaddr);

void usys_putc(char ch);
u32 usys_getc(void);
void usys_exit(u64 ret);
u64 usys_yield(void);
int usys_create_device_pmo(u64 paddr, u64 size);
int usys_create_pmo(u64 size, u64 type);
int usys_map_pmo(u64 cap_group_cap, u64 pmo_cap, u64 addr, u64 perm);
int usys_create_thread(u64 thread_args_p);
int usys_create_cap_group(u64 badge, char *name, u64 name_len);
u64 usys_register_server(u64 ipc_handler, u64 reigster_cb_cap);
u32 usys_register_client(u32 server_cap, u64 vm_config_ptr);
u64 usys_ipc_call(u32 conn_cap, u64 ipc_msg_ptr, u64 cap_num);
void usys_ipc_return(u64 ret, u64 cap_num);
void usys_ipc_register_cb_return(u64, u64);
u64 usys_ipc_send_cap(u32 conn_cap, u32 send_cap);
void usys_debug_va(u64 va);
int usys_cap_copy_to(u64 dest_cap_group_cap, u64 src_slot_id);
int usys_cap_copy_from(u64 src_cap_group_cap, u64 src_slot_id);
int usys_unmap_pmo(u64 cap_group_cap, u64 pmo_cap, u64 addr);
int usys_set_affinity(u64 thread_cap, s32 aff);
s32 usys_get_affinity(u64 thread_cap);
int usys_get_pmo_paddr(u64 pmo_cap, u64 *buf);
int usys_get_phys_addr(void *vaddr, u64 *paddr);

int usys_create_pmos(void *, u64);
int usys_map_pmos(u64, void *, u64);
int usys_write_pmo(u64, u64, void *, u64);
int usys_read_pmo(u64 cap, u64 offset, void *buf, u64 size);
int usys_transfer_caps(u64, int *, int, int *);

void usys_perf_start(void);
void usys_perf_end(void);
void usys_perf_null(void);
void usys_top(void);

int usys_irq_register(int irq);
int usys_irq_wait(int irq_cap, bool is_block);
int usys_irq_ack(int irq_cap);
int usys_create_notifc();
int usys_wait(u32 notifc_cap, bool is_block, void *timeout);
int usys_notify(u32 notifc_cap);
int usys_translate_aarray(vaddr_t *aarray, size_t aarray_size,
			  off_t offset, size_t count, int *pmo_p);

int usys_register_recycle_thread(int cap, u64 buffer);
