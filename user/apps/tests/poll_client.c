#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sched.h>

#include "tests.h"

#define MAX_CLIENT 64
#define SEND_COUNT 10

int main(int argc, char *argv[], char *envp[])
{

	int client_fd = 0;
	int ret = 0;
	char data_buf[256];
	int i, j;
	struct sockaddr_in target;
	int fds[MAX_CLIENT];
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(3, &mask); 
        ret = sched_setaffinity(0, sizeof(mask), &mask);
	sched_yield();

	for (i = 0; i < MAX_CLIENT; i++) {
		client_fd = socket(AF_INET, SOCK_STREAM, 0);
		chcore_assert(client_fd >= 0, "[client] create socket failed\n");

		memset(&target, 0, sizeof(struct sockaddr_in));
		target.sin_family = AF_INET;
		target.sin_addr.s_addr = inet_addr("127.0.0.1");
		target.sin_port = htons(9001);
		ret = connect(client_fd, (struct sockaddr *)&target,
				      sizeof(struct sockaddr_in));
		chcore_assert(ret >= 0, "[client] cannot connect!");

		fds[i] = client_fd;
	}

	for (i = 0; i < SEND_COUNT; i++) {
		usleep(1000);
		for (j = 0; j < MAX_CLIENT; j++) {
			usleep(1000);
			sprintf(data_buf, "msg from client:%d count:%d!\n", j, i);
			ret = send(fds[j], data_buf, strlen(data_buf), 0);
			chcore_assert(ret >= 0, "[client] send return error!");
		}
	}

	for (i = 0; i < MAX_CLIENT; i++) {
		ret = shutdown(fds[i], SHUT_WR);
		chcore_assert(ret >= 0, "[client] shutdown failed");

		ret = close(fds[i]);
		chcore_assert(ret >= 0, "[client] close return error!");
	}

	printf("LWIP poll client exit\n");
	return 0;
}
