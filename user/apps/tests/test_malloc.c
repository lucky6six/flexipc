#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>
#include "tests.h"

/* Testing mmap only */
#define TEST_MMAP	0
/* Testing malloc (with mmap) */
#define TEST_MALLOC	1

#define TEST_OPTION TEST_MALLOC

#define MALLOC_TEST_NUM         (256*4)
#define THREAD_NUM		16
#define PLAT_CPU_NUM            4

struct fake_tls_struct {
	char tls[64];
};

struct fake_tls_struct temp_tls[THREAD_NUM];

unsigned long buffer_array[MALLOC_TEST_NUM * THREAD_NUM];
int global_error = 0;

void check_repeat(void)
{
	int i;
	int j;

	for (i = 0; i < MALLOC_TEST_NUM * THREAD_NUM; ++i) {
		for (j = i + 1; j < MALLOC_TEST_NUM * THREAD_NUM; ++j) {
			if ((buffer_array[i] != 0) &&
			     (buffer_array[j] != 0)) {
			if (labs(buffer_array[i] - buffer_array[j]) < 0x1000) {
				printf("check_repeat error\n");
				printf("buf %d: %p. buf %d: %p\n",
				       i, buffer_array[i],
				       j, buffer_array[j]);
				while (1) {}
			}
			}
		}
	}
	printf("check_repeat OK\n");
}

static void suicide(void)
{
	int *ptr;

	ptr = (int *)0xdead;
	*ptr = 1;
}

/* This function is for testing FS saving on x86_64 */
void checkfs(int i)
{
#if 0
	long fs;

	fs = (long)&temp_tls[i];
	if (__builtin_ia32_rdfsbase64() != fs) {
		printf("%s error. tid: %d, fs should be %p (but is %p)\n",
		       __func__, i, fs, __builtin_ia32_rdfsbase64());
		suicide();
	}
#endif
}

void malloc_test(int tid)
{
        int i = 0, j = 0, size = 0;
        char *buf[MALLOC_TEST_NUM];

	printf("start malloc_test\n");

	char target_val = tid + 9;

        for (i = 0; i < MALLOC_TEST_NUM; ++i) {
                size = 0x1000;
		buf[i] = NULL;
#if TEST_OPTION == TEST_MALLOC
		buf[i] = malloc(size);
#elif TEST_OPTION == TEST_MMAP
		buf[i] = (char*) mmap(0, size, PROT_READ|PROT_WRITE,
				      MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif
		buffer_array[tid * MALLOC_TEST_NUM + i] = (long)buf[i];
		if (buf[i] == NULL) {
			while (1)
				printf("malloc return NULL!");
		}
		// check_repeat();
                for (j = 0; j < size; ++j) {
                        *((char *)buf[i] + j) = target_val;
			if (*((char *)buf[i] + j) != target_val) {
				printf("[write time failed]\n");
				suicide();
			}
			checkfs(tid);
                }
	}

        for (i = 0; i < MALLOC_TEST_NUM; ++i) {
                size = 0x1000;
                for (j = 0; j < size; ++j) {
			if (*((char *)buf[i] + j) != target_val) {
				global_error = 1;
				//suicide();
			}
			checkfs(tid);
                }
	}

	printf("end malloc_test\n");
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


/* The following is for testing FS saving on x86_64 */
#if 0
	long fs;
	fs = (long)&temp_tls[i];
	printf("set tls with fs: %p\n", fs);
	__asm__ __volatile__("wrfsbase %0"::"r"(fs));
#endif

        malloc_test(i);

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

	check_repeat();
	printf("global_error = %d (corrent value is 0)\n", global_error);
	if (global_error == 1) {
		suicide();
	}

        printf("test_malloc done\n");
        return 0;
}
