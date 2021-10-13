#define _GNU_SOURCE

#include <stdio.h>
#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <chcore/pmu.h>
#include <sched.h>

#include "perf.h"

#define PERF_NUM        10000000

int main(int argc, char *argv[])
{
	s64 start = 0, end = 0;
        int i = 0;
	cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(2, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) != 0)
                error("sched_setaffinity failed!\n");
        sched_yield();

	info("Start measure syscall\n");

	start = pmu_read_real_cycle();
        for (i = 0; i < PERF_NUM; i ++)
                usys_perf_null();

	end = pmu_read_real_cycle();
	info("Syscall: %ld cycles\n", (end - start)/PERF_NUM);
	return 0;
}
