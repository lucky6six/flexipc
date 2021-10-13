#pragma once

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef long long s64;
typedef int s32;
typedef short s16;
typedef signed char s8;

#ifdef CHCORE
#include <posix/sys/types.h>

#define NULL ((void *)0)

#else
#include <stdlib.h>
#endif

typedef char bool;
#define true (1)
#define false (0)
typedef u64 paddr_t;
typedef u64 vaddr_t;

typedef u64 atomic_cnt;

/* Different platform may have different cacheline size and may have some features like prefetch */
#define CACHELINE_SZ 64
#define r_align(n, r)        (((n) + (r) - 1) & -(r))
#define cache_align(n)       r_align(n , CACHELINE_SZ)
#define pad_to_cache_line(n) (cache_align(n) - (n))
