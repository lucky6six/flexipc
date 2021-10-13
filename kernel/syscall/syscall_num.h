#pragma once

#define NR_SYSCALL   256

void sys_yield(void);
void sys_thread_exit(void);
void sys_create_device_pmo(void);
void sys_create_pmo(void);
void sys_map_pmo(void);
void sys_create_thread(void);
void sys_create_cap_group(void);
void sys_debug_va(void);
void sys_cap_copy_to(void);
void sys_cap_copy_from(void);
void sys_unmap_pmo(void);
void sys_set_affinity(void);
void sys_get_affinity(void);

void sys_get_pmo_paddr(void);
void sys_get_phys_addr(void);

void sys_create_pmos(void);
void sys_map_pmos(void);
void sys_write_pmo(void);
void sys_transfer_caps(void);
void sys_read_pmo(void);
void sys_irq_register(void);
void sys_irq_wait(void);
void sys_irq_ack(void);
void sys_create_notifc();
void sys_wait();
void sys_notify();
void sys_clock_gettime();
void sys_clock_nanosleep();

void sys_handle_brk(void);
void sys_handle_mmap(void);
void sys_handle_munmap(void);
void sys_handle_mprotect(void);

/* Temp impl */
void sys_shmget(void);
void sys_shmat(void);
void sys_shmdt(void);
void sys_shmctl(void);

void sys_handle_fmap(void);
void sys_translate_aarray(void);

void sys_register_recycle(void);
void sys_cap_group_recycle(void);

void sys_perf_start(void);
void sys_perf_end(void);
void sys_perf_null(void);
void sys_perf_ipc(void);
void sys_top(void);

#define SYS_putc				0
#define SYS_getc				1
#define SYS_yield				2
#define SYS_thread_exit				3
#define SYS_sleep				4
#define SYS_create_pmo				5
#define SYS_map_pmo				6
#define SYS_create_thread			7
#define SYS_create_cap_group			8
#define SYS_register_server			9
#define SYS_register_client			10

#define SYS_ipc_register_cb_return		11
#define SYS_ipc_call				12
#define SYS_ipc_return				13
#define SYS_ipc_send_cap			14
#define SYS_cap_copy_to				15
#define SYS_cap_copy_from			16
#define SYS_unmap_pmo				17
#define SYS_set_affinity                        18
#define SYS_get_affinity                        19
#define SYS_create_device_pmo			20
#define SYS_irq_register			21
#define SYS_irq_wait				22
#define SYS_irq_ack				23
#define SYS_get_pmo_paddr			24
#define SYS_get_phys_addr			25
#define SYS_create_notifc			26
#define SYS_wait				27
#define SYS_notify				28
#define SYS_clock_gettime			29
#define SYS_clock_nanosleep			30

#define SYS_create_pmos                         101
#define SYS_map_pmos                            102
#define SYS_write_pmo                           103
#define SYS_read_pmo				104
#define SYS_transfer_caps                       105

/* Perf Debug Syscall*/
#define SYS_perf_start				150
#define SYS_perf_end				151
#define SYS_perf_null                           152

/* Memory related */
#define SYS_handle_brk				201
#define SYS_handle_mmap				202
#define SYS_handle_munmap			203
#define SYS_mprotect				204
#define SYS_shmget				205
#define SYS_shmat				206
#define SYS_shmdt				207
#define SYS_shmctl				208
#define SYS_handle_fmap				209
#define SYS_translate_aarray			210

#define SYS_register_recycle			211
#define SYS_exit_group				212
#define SYS_cap_group_recycle			213

/* TEMP */
#define SYS_top                                 252
#define SYS_fs_load_cpio			253
#define SYS_debug_va				254
