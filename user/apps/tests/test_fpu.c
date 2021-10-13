#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>

#include "tests.h"

#define FPU_TEST_NUM		(256)
#define THREAD_NUM		32
#define PLAT_CPU_NUM            4

int global_error = 0;

static void suicide(void)
{
	int *ptr;

	ptr = (int *)0xdead;
	*ptr = 1;
}

void test_fpu(int base)
{
	int i, j;
	double *fnums;

	fnums = malloc(FPU_TEST_NUM * sizeof(double));

	for (i = 0; i < FPU_TEST_NUM; i++) {
		fnums[i] = base + 1 / (2 + i);
	}

	for (i = 0; i < 1000000; i++) {
		for (j = 0; j < FPU_TEST_NUM; ++j) {
			if ((fnums[j] < base) || (fnums[j] > base + 1)) {
				suicide();
			}
		}
	}
}

volatile int finish_flag;

void *thread_routine(void *arg)
{
        int i = (int) (long) arg;
	int ret = 0;
	/* binding to different cores */
        cpu_set_t mask;
	CPU_ZERO(&mask);
        CPU_SET(i % PLAT_CPU_NUM, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        test_fpu(i+1);

        __atomic_add_fetch(&finish_flag, 1, __ATOMIC_SEQ_CST);

        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        int i = 0;
	pthread_t thread_id;

	global_error = 0;

        finish_flag = 0;
        for (i = 0; i < THREAD_NUM; i++)
	        pthread_create(&thread_id, NULL, thread_routine, (void *)(unsigned long)i);
        while (finish_flag != THREAD_NUM);

	printf("global_error = %d (corrent value is 0)\n", global_error);
	if (global_error == 1) {
		suicide();
	}

        printf("test_fpu done\n");
        return 0;
}
