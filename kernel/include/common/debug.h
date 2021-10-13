#pragma once

#define OFF 0
#define ON  1

/*
 * When HOOKING_SYSCALL is ON,
 * ChCore will print the syscall number before invoking the handler.
 * Please refer to kernel/syscall/syscall.c.
 */
#define HOOKING_SYSCALL OFF

/*
 * When DETECTING_DOUBLE_FREE_IN_SLAB is ON,
 * ChCore will check each free operation in the slab allocator.
 * Please refer to kernel/mm/slab.c.
 */
#define DETECTING_DOUBLE_FREE_IN_SLAB OFF
