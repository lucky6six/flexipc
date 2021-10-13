#pragma once

/* Use -1 instead of 0 (NULL) since 0 is used as the ending mark of envp */
#define ENVP_NONE_PMOS (-1)
#define ENVP_NONE_CAPS (-1)

/* Used for usys_fs_load_cpio */
/* wh_cpio */
#define FSM_CPIO     0
#define TMPFS_CPIO   1
#define RAMDISK_CPIO 2 
/* op */
#define QUERY_SIZE   0
#define LOAD_CPIO    1


/*
 * Used as a token which is added at the beginnig of the path name
 * during booting a system server. E.g., "kernel-server/fsm.srv"
 *
 * This is simply for preventing users unintentionally
 * run system servers in Shell. That is meaningless.
 */
#define KERNEL_SERVER "kernel-server"

#define NO_AFF       (-1)
#define NO_ARG	     (0)
#define PASSIVE_PRIO (-1)

/*
 * TODO (tmac): uapi can help us to clean the code,
 * i.e., only keepping one MARCO for both kernel and user.
 */

/* virtual memory rights */
#define VM_READ  (1 << 0)
#define VM_WRITE (1 << 1)
#define VM_EXEC  (1 << 2)
#define VM_FORBID     (0)

/* PMO types */
#define PMO_ANONYM 0
#define PMO_DATA   1
//#define PMO_FILE   2
#define PMO_SHM    3
#define PMO_FORBID 10

/* a thread's own cap_group */
#define SELF_CAP   0

#define MAX_PRIO   255

/* TODO: use something like **vmalloc** to remvoe the magic adresses */
#define CHILD_THREAD_STACK_BASE         (0x500000800000UL)
#define CHILD_THREAD_STACK_SIZE         (0x800000UL)
#define CHILD_THREAD_PRIO               (MAX_PRIO - 1)

#define MAIN_THREAD_STACK_BASE		(0x500000000000UL)
#define MAIN_THREAD_STACK_SIZE          (0x800000UL)
#define MAIN_THREAD_PRIO		(MAX_PRIO - 1)

/*
 * IPC_SHM_BASE:
 * the base virtual address for mapping PMO_SHM of each IPC connection.
 *
 * Because the definition of IPC_SHM_BASE and THREAD_STACK_BASE,
 * we support up to (0x80000000000 / 0x200000) == 4194304 threads
 * in one process.
 */
#define IPC_SHM_BASE				(0x580000000000UL)
#define IPC_PER_SHM_SIZE                        (0x1000)

#define ROUND_UP(x, n)		(((x) + (n) - 1) & ~((n) - 1))
#define ROUND_DOWN(x, n)	((x) & ~((n) - 1))

#ifndef PAGE_SIZE
#define PAGE_SIZE   0x1000
#endif

/*
 * Syscall indexes
 */
#define CHCORE_SYS_putc                 0
#define CHCORE_SYS_getc                 1
#define CHCORE_SYS_yield                2
#define CHCORE_SYS_thread_exit          3
#define CHCORE_SYS_sleep                4
#define CHCORE_SYS_create_pmo           5
#define CHCORE_SYS_map_pmo              6
#define CHCORE_SYS_create_thread        7
#define CHCORE_SYS_create_cap_group     8
#define CHCORE_SYS_register_server      9
#define CHCORE_SYS_register_client      10

#define CHCORE_SYS_ipc_register_cb_return 11
#define CHCORE_SYS_ipc_call             12
#define CHCORE_SYS_ipc_return           13
#define CHCORE_SYS_ipc_send_cap         14
#define CHCORE_SYS_cap_copy_to          15
#define CHCORE_SYS_cap_copy_from        16
#define CHCORE_SYS_unmap_pmo            17
#define CHCORE_SYS_set_affinity         18
#define CHCORE_SYS_get_affinity         19
#define CHCORE_SYS_create_device_pmo    20
#define CHCORE_SYS_irq_register         21
#define CHCORE_SYS_irq_wait             22
#define CHCORE_SYS_irq_ack              23
#define CHCORE_SYS_get_pmo_paddr        24
#define CHCORE_SYS_get_phys_addr        25
#define CHCORE_SYS_create_notifc	26
#define CHCORE_SYS_wait			27
#define CHCORE_SYS_notify		28
#define CHCORE_SYS_clock_gettime	29
#define CHCORE_SYS_clock_nanosleep	30

#define CHCORE_SYS_create_pmos          101
#define CHCORE_SYS_map_pmos             102
#define CHCORE_SYS_write_pmo            103
#define CHCORE_SYS_read_pmo             104
#define CHCORE_SYS_transfer_caps        105

/* Temp: performance test */
#define CHCORE_SYS_perf_start           150
#define CHCORE_SYS_perf_end             151
#define CHCORE_SYS_perf_null            152

#define CHCORE_SYS_handle_brk           201
#define CHCORE_SYS_handle_mmap          202
#define CHCORE_SYS_handle_munmap        203
#define CHCORE_SYS_mprotect	        204
#define CHCORE_SYS_shmget		205
#define CHCORE_SYS_shmat		206
#define CHCORE_SYS_shmdt		207
#define CHCORE_SYS_shmctl		208
#define CHCORE_SYS_handle_fmap          209
#define CHCORE_SYS_translate_aarray     210

#define CHCORE_SYS_register_recycle     211
#define CHCORE_SYS_process_exit		212
#define CHCORE_SYS_cap_group_recycle    213

/* Temp: debugging */
#define CHCORE_SYS_top                  252
#define CHCORE_SYS_fs_load_cpio         253
#define CHCORE_SYS_debug_va             254
#define CHCORE_SYS_debug                255
