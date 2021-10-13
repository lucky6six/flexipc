#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>

#define SLEEP_COUNT	10

struct sleeper_arg {
	int thread_id;
	int cpu;
};

static int notifc_cap;
static pthread_barrier_t test_end_barrier, test_start_barrier;

void *sleeper(void *arg)
{
	struct sleeper_arg *sarg = (struct sleeper_arg *)arg;
	struct timespec ts;
	int i, ret;
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(sarg->cpu, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
	BUG_ON(ret != 0);

	/* notification timeout test */
	pthread_barrier_wait(&test_start_barrier);
	for (i = 0; i < SLEEP_COUNT; i++) {
		if (sarg->thread_id % 2) {
			/* wait with timeout of 1s */
			ts.tv_sec  = 1;
			ts.tv_nsec = 0;
			ret = usys_wait(notifc_cap, true, &ts);
			printf("thread:%d wait ret %d\n", sarg->thread_id, ret);
		} else {
			/* signal every 1.5s */
			ts.tv_sec  = 1;
			ts.tv_nsec = 500UL * 1000 * 1000;
			nanosleep(&ts, NULL);

			printf("thread:%d notify\n", sarg->thread_id);
			usys_notify(notifc_cap);
		}
	}
	pthread_barrier_wait(&test_end_barrier);

	/* nanosleep test */
	pthread_barrier_wait(&test_start_barrier);
	for (i = 0; i < SLEEP_COUNT; i++) {
		ts.tv_sec  = 0;
		ts.tv_nsec = 400UL * 1000 * 1000 * sarg->thread_id;
		printf("cpu:%d thread:%ld sleep:%ld.%ld\n",
		       sarg->cpu, sarg->thread_id, ts.tv_sec,
		       ts.tv_nsec);
		nanosleep(&ts, NULL);
	}
	pthread_barrier_wait(&test_end_barrier);

	return NULL;
}

#define THREAD_NUM	8
#define PLAT_CPU_NUM	4

int main() {
	pthread_t pthreads[THREAD_NUM * PLAT_CPU_NUM];
	int i, j;
	struct sleeper_arg args[THREAD_NUM][PLAT_CPU_NUM];

	notifc_cap = usys_create_notifc();
	BUG_ON(notifc_cap < 0);
	pthread_barrier_init(&test_end_barrier, NULL,
			     THREAD_NUM * PLAT_CPU_NUM + 1);
	pthread_barrier_init(&test_start_barrier, NULL,
			     THREAD_NUM * PLAT_CPU_NUM + 1);

	for (i = 0; i < THREAD_NUM; i++) {
		for (j = 0; j < PLAT_CPU_NUM; j++) {
			args[i][j].thread_id = i + 1;
			args[i][j].cpu = j;
			pthread_create(&pthreads[i * PLAT_CPU_NUM + j], NULL, sleeper,
				       &args[i][j]);
		}
	}

	pthread_barrier_wait(&test_start_barrier);
	pthread_barrier_wait(&test_end_barrier);
	printf("[sleep test]: timeout notification complete\n");

	pthread_barrier_wait(&test_start_barrier);
	pthread_barrier_wait(&test_end_barrier);
	printf("[sleep test]: nanosleep complete\n");

	void *ret;
	for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
		pthread_join(pthreads[i], &ret);
	}

	return 0;
}
