#define _GNU_SOURCE

#include <pthread.h>
#include <stdint.h> /* Definition of uint64_t */
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define EFD_NUM         5
#define TFD_NUM         5
#define TEST_NUM        5

int efd[EFD_NUM];
int tfd[TFD_NUM];

void *thread_routine(void *arg)
{
	int i = 0;
        int index = (int) (long) arg;

        for (i = 0; i < TEST_NUM; i++) {
		sleep(index);
                uint64_t u = i + 1;
		int s = write(efd[index], &u, sizeof(uint64_t));
		if (s != sizeof(uint64_t)) {
                        printf("EVENTFD WRITE FAILED!\n");
                        return 0;
                }
                // printf("Write %d %d\n", efd[index], u);
        }
	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t tid[EFD_NUM];
	uint64_t u;
	ssize_t s;
        int i = 0, j = 0, ret = 0;
        struct itimerspec new_value;
	uint64_t counter;

        /* Init event fd */
        for (i = 0; i < EFD_NUM; i++) {
	        efd[i] = eventfd(0, 0);
	        if (efd[i] == -1) {
                        return -1;
                }
        }

        /* Init timer fd */
        for (i = 0; i < TFD_NUM; i++) {
                tfd[i] = timerfd_create(CLOCK_MONOTONIC, 0);
                new_value.it_value.tv_sec = i;
                new_value.it_value.tv_nsec = 0;
                new_value.it_interval.tv_sec = 10;
                new_value.it_interval.tv_nsec = 0;
	        timerfd_settime(tfd[i], 0, &new_value, NULL);
        }

        /* Create thread to write event fd */
        for (i = 0; i < EFD_NUM; i++) {
	        pthread_create(&tid[i], NULL, thread_routine, (void *)(long)i);
        }

	struct pollfd item[EFD_NUM + TFD_NUM];
        for (i = 0; i < EFD_NUM; i++) {
                item[i].fd = efd[i];
                item[i].events = POLLIN;
                item[i].revents = 0;
        }

        for (j = 0; j < TFD_NUM; i ++, j ++) {
                item[i].fd = tfd[j];
                item[i].events = POLLIN;
                item[i].revents = 0;
        }

	while (1) {
		ret = poll(item, EFD_NUM + TFD_NUM, -1);
                printf("POLL Return %d\n", ret);
		if (ret == -1) {
			printf("error");
			exit(EXIT_SUCCESS);
		} 
                for (i = 0; i < EFD_NUM; i++) {
                        if (item[i].revents & POLLIN) {
                                printf("FD %d can read!\n",  item[i].fd);
                                s = read(item[i].fd, &u, sizeof(uint64_t));
			        if (s != sizeof(uint64_t)) {
                                        printf("Read failed!\n");
                                        return -1;
                                }
			        printf("Parent read %llu (0x%llx) from efd\n",
			                (unsigned long long)u, (unsigned long long)u);
                        }
                }

                for (j = 0; j < TFD_NUM; i ++, j ++) {
                        if (item[i].revents & POLLIN) {
                                printf("FD %d can read!\n",  item[i].fd);
                                s = read(item[i].fd, &counter, sizeof(counter));
                                printf("fd %d read %d\n", item[i].fd, counter);
                        }
                }
	}
	return 0;
}
