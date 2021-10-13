#define _GNU_SOURCE
#include <stdio.h>
#include <chcore/syscall.h>
#include <chcore/thread.h>
#include <sched.h>
#include <chcore/pmu.h>

#include "perf.h"

#define PRIO 255

/* Task switch benchmark */
#define TASKSWITCH_COUNT 10000000

// #define NO_VMSWITCH

volatile u64 end = 0;

volatile int barrier = 0;

void *switch_bench(void *arg)
{
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(2, &mask); 
        sched_setaffinity(0, sizeof(mask), &mask);
	int i = TASKSWITCH_COUNT;
	int j = (int) (long)arg;

	while (barrier != 1);
	while (i --)
		usys_yield();
	asm volatile("nop":::"memory");
	if (j != 0) {
		end = pmu_read_real_cycle();
	}
	usys_exit(0);
	return 0;
}

int main(int argc, char *argv[])
{
	int child_thread_cap0;
	u64 start;

	end = 0;
	info("Start measure context switch\n");
	child_thread_cap0 = create_thread(switch_bench, 0, PRIO, 0);
	if (child_thread_cap0 < 0)
		info("Create thread failed, return %d\n",
		       child_thread_cap0);
#ifndef NO_VMSWITCH
	int child_thread_cap1;
	child_thread_cap1 = create_thread(switch_bench, 1, PRIO, 0);
	if (child_thread_cap1 < 0)
		info("Create thread failed, return %d\n",
		       child_thread_cap1);
#endif
	start = pmu_read_real_cycle();
	barrier = 1;
	while(end == 0);

#ifdef NO_VMSWITCH
	info("Content switch: %ld cycles\n", ((end - start)/TASKSWITCH_COUNT));
#else
	info("Content switch: %ld cycles\n", ((end - start)/TASKSWITCH_COUNT)/2);
#endif
	return 0;
}
