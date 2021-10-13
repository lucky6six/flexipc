#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h> /* Definition of uint64_t */
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "tests.h"

#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

int efd1;
int efd2;
int efd3;
int efd4;

#define WRITEVAL 1
#define WRITECNT 1000000
#define THREAD 20
#define PLAT_CPU_NUM 4

void *writer_routine(void *arg)
{
	cpu_set_t mask;
	uint64_t u;
	int i, s;
	int ret;

	CPU_ZERO(&mask);
	CPU_SET((unsigned long)arg % PLAT_CPU_NUM, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	chcore_assert(ret == 0, "Set affinity failed!");
	sched_yield();

	printf("writer thread %p up\n", pthread_self());
	/* EFD_SEMAPHORE */

	for (i = 0; i < WRITECNT; i++) {
		u = WRITEVAL;
		s = write(efd1, &u, sizeof(uint64_t));
		chcore_assert(s == sizeof(uint64_t), "read return error");
	}

	printf("writer thread %p finish efd1\n", pthread_self());
	/* EFD_NONBLOCK */

	for (i = 0; i < WRITECNT; i++) {
		u = WRITEVAL;
		s = write(efd2, &u, sizeof(uint64_t));
		chcore_assert(s == sizeof(uint64_t), "read return error");
	}

	printf("writer thread %p finish efd2\n", pthread_self());
	/* Larger than max */
	/* BLOCK WRITER */
	u = 0xffffffffffffffff;
	s = write(efd3, &u, sizeof(uint64_t));
	chcore_assert(s == -1, "read return error");

	u = 0xfffffffffffffffe;
	s = write(efd3, &u, sizeof(uint64_t));
	chcore_assert(s == sizeof(uint64_t), "read return error");
	info("write thread %lx write efd %d finished\n", pthread_self(), efd3);

	/* EFD_SEMAPHORE BLOCK WRITER */
	for (i = 0; i < WRITECNT; i++) {
		u = WRITEVAL;
		s = write(efd4, &u, sizeof(uint64_t));
		chcore_assert(s == sizeof(uint64_t), "read return error");
	}

	info("writer thread %lx finished\n", pthread_self());
	return 0;
}

void *reader_routine(void *arg)
{
	cpu_set_t mask;
	uint64_t u;
	int i, s;
	int ret;
	uint64_t val = 0;

	/* EFD_SEMAPHORE BLOCK READER */

	CPU_ZERO(&mask);
	CPU_SET((unsigned long)arg % PLAT_CPU_NUM, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	chcore_assert(ret == 0, "Set affinity failed!");
	sched_yield();
	printf("reader thread %p up\n", pthread_self());

	for (i = 0; i < WRITECNT * WRITEVAL; i++) {
		s = read(efd1, &u, sizeof(uint64_t));
		chcore_assert(s == sizeof(uint64_t), "read return error");
		chcore_assert(u == 1, "EFD_SEMAPHORE test failed!");
	}

	printf("reader thread %p finish efd1\n", pthread_self());

	/* EFD_NONBLOCK */
	for (i = 0; i < WRITECNT * WRITEVAL; i++) {
		s = read(efd2, &u, sizeof(uint64_t));
		val += (s == sizeof(uint64_t)) ? u : 0;
	}

	printf("reader thread %p finish efd2\n", pthread_self());

	/* BLOCK WRITER */
	s = read(efd3, &u, sizeof(uint64_t));
	chcore_assert(s == sizeof(uint64_t), "read return error");
	chcore_assert(u == 0xfffffffffffffffe, "BLOCK WRITER test failed!")
	    info("Read %llu (0x%llx) from efd %d\n", (unsigned long long)u,
		 (unsigned long long)u, efd3);

	/* EFD_SEMAPHORE BLOCK WRITER */
	for (i = 0; i < WRITECNT * WRITEVAL; i++) {
		s = read(efd4, &u, sizeof(uint64_t));
		chcore_assert(s == sizeof(uint64_t), "read return error");
		chcore_assert(u == 1, "EFD_SEMAPHORE test failed!");
	}

	info("reader thread %lx finished\n", pthread_self());
	return (void *)val;
}

#define INIT_VAL 10

int main(int argc, char *argv[])
{
	pthread_t writer[THREAD];
	pthread_t reader[THREAD];
	uint64_t ret;
	uint64_t efd2_val = 0;
	ssize_t s;
	int i = 0;

	if ((efd1 = eventfd(0, EFD_SEMAPHORE)) < 0)
		handle_error("eventfd");
	if ((efd2 = eventfd(INIT_VAL, EFD_NONBLOCK)) < 0)
		handle_error("eventfd");
	if ((efd3 = eventfd(0, 0)) < 0)
		handle_error("eventfd");
	if ((efd4 = eventfd(0, EFD_SEMAPHORE)) < 0)
		handle_error("eventfd");

	ret = 0xfffffffffffffffe;
	s = write(efd4, &ret, sizeof(uint64_t));

	for (i = 0; i < THREAD; i++) {
		pthread_create(&reader[i], NULL, reader_routine,
			       (void *)(unsigned long)i);
		pthread_create(&writer[i], NULL, writer_routine,
			       (void *)(unsigned long)(THREAD - i));
	}

	for (i = 0; i < THREAD; i++) {
		pthread_join(reader[i], (void **)&ret);
		pthread_join(writer[i], NULL);
		efd2_val += ret;
	}
	s = read(efd2, &ret, sizeof(uint64_t));
	efd2_val += (s == sizeof(uint64_t)) ? ret : 0;

	chcore_assert(efd2_val == WRITECNT * WRITEVAL * THREAD + INIT_VAL,
		      "EFD_NONBLOCK failed\n");

	info("Finish eventfd test!\n");
	return 0;
}
