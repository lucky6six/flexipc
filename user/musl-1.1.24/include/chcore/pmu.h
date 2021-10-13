#pragma once

#include <chcore/type.h>

#if defined(__aarch64__)

static inline u64 pmu_read_real_cycle(void)
{
	s64 tv;
	asm volatile ("mrs %0, pmccntr_el0":"=r" (tv));
	return tv;
}

static inline void pmu_clear_cnt(void)
{
	asm volatile ("msr pmccntr_el0, %0"::"r" (0));
}

#endif

#if defined(__x86_64__)

static inline u64 pmu_read_real_cycle(void)
{
	u32 lo, hi;

	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return (((u64)hi) << 32) | lo;
}

static void pmu_clear_cnt(void)
{
	/* No such function */
}

#endif