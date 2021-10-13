#define _GNU_SOURCE

#include <stdio.h> 
#include <stdlib.h> 
#include <sched.h>
#include <unistd.h>  
#include <pthread.h>
#include <string.h>
#include "tests.h"

#define THREAD_NUM	64
#define PLAT_CPU_NUM	4
#define TEST_NUM	50000

int g_varible;
pthread_mutex_t g_lock;

void *thread_routine(void *var)
{
	int i = (int)(unsigned long) var;
	int ret = 0;
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(i % PLAT_CPU_NUM, &mask); 
        ret = sched_setaffinity(0, sizeof(mask), &mask);
	chcore_assert(ret == 0, "Set affinity failed!");

	info("Pthread %lu alive!\n", (unsigned long) var);
	for (i = 0; i < TEST_NUM; i++) {
		pthread_mutex_lock(&g_lock);
		g_varible ++;
		pthread_mutex_unlock(&g_lock);
	}
	return var;
}

int main(int argc, char *argv[])
{
	pthread_t thread_id[THREAD_NUM];
	int i = 0;
	void *ret = 0;

	pthread_mutex_init(&g_lock, NULL);
	g_varible = 0;

	u64 start = pmu_read_real_cycle();

	for (i = 0; i < THREAD_NUM; i ++)
		pthread_create(&thread_id[i], NULL, thread_routine, 
			(void *)(unsigned long)i);
	for (i = 0; i < THREAD_NUM; i ++) {
		pthread_join(thread_id[i], &(ret));
		chcore_assert(((unsigned long)ret == (unsigned long)i), 
			"Return value check failed!");
	}

	u64 end = pmu_read_real_cycle();
	chcore_assert(g_varible == TEST_NUM * THREAD_NUM,
		"Pthread lock failed!");
	info("Mutex cycle %lf!\n", ((double)(end - start))/(TEST_NUM * THREAD_NUM));
	info("Pthread test finished!\n");
        return 0;
}
