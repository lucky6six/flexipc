#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <syscall.h>
#include <pthread.h>

#include "tests.h"

#define TOTAL_TEST_NUM          16
#define SCHED_TEST_NUM          32

#define PLAT_CPU_NUM            4
#define THREAD_NUM		512

volatile int finish_flag;

void sched_test(int cpuid)
{
        cpu_set_t mask;
        int ret = 0;
        int i = 0, j = 0;

        /* pthread not implemented, should add some other threads */
        for (i = 0; i < SCHED_TEST_NUM ; i++) {
                CPU_ZERO(&mask);
                CPU_SET(i % PLAT_CPU_NUM, &mask);
	        ret = sched_setaffinity(0, sizeof(mask), &mask);
                chcore_assert(ret == 0, "sched_setaffinity failed!");
                sched_yield();
                CPU_ZERO(&mask);
                ret = sched_getaffinity(0, sizeof(mask), &mask);
                chcore_assert(ret == 0, "sched_getaffinity failed!");
                for (j = 0; j < 128; j++) {
		        if (CPU_ISSET(j, &mask))
                                break;
		}
                chcore_assert((i % PLAT_CPU_NUM) == j, "sched_setaffinity check failed!");
        }

        /* set a fault cpuid */
        CPU_ZERO(&mask);
        CPU_SET(i + 128, &mask); 
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret != 0, "sched_setaffinity fault check failed!");

        /* set to cpuid */
        CPU_ZERO(&mask);
        CPU_SET(cpuid, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();
}

void *thread_routine(void *arg)
{
        int i = 0;
	u64 tid = (u64)arg;

        for (i = 0; i < TOTAL_TEST_NUM; i++) {
                sched_test((tid + i) % PLAT_CPU_NUM);
        }
        __atomic_add_fetch(&finish_flag, 1, __ATOMIC_SEQ_CST);
        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        int i = 0;
	pthread_t thread_id;

        finish_flag = 0;
        for (i = 0; i < THREAD_NUM; i++)
	        pthread_create(&thread_id, NULL, thread_routine, (void *)(unsigned long)i);
        while (finish_flag != THREAD_NUM);

        printf("test_sched done\n");
        return 0;
}
