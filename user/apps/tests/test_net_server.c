#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "tests.h"

void *handle_thread(void *arg)
{
	char large_buffer[NET_BUFSIZE];
	struct iovec iosend[1];
	struct sockaddr_in client_addr;
	struct msghdr msgsend;
	int accept_fd = (int)(long)arg;
	socklen_t len;
	char IP[16];
	int ret = 0;
	unsigned int port;
	cpu_set_t mask;

	/* set aff */
	CPU_ZERO(&mask);
	CPU_SET(accept_fd % 4, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	chcore_assert(ret == 0, "Set affinity failed!");
	sched_yield();

	bzero(&client_addr, sizeof(client_addr));
	len = sizeof(client_addr);
	ret = getpeername(accept_fd, (struct sockaddr *)&client_addr, &len);
	chcore_assert(ret >= 0, "[server] getpeername return error!");

	inet_ntop(AF_INET, &client_addr.sin_addr, IP, sizeof(IP));
	port = ntohs(client_addr.sin_port);

	for (int i = 0; i < sizeof(large_buffer); i++) {
		large_buffer[i] = 'a' + i % 26;
	}

	if (accept_fd % 5 == 1) { /* Use send msg */
		memset(&msgsend, 0, sizeof(msgsend));
		memset(iosend, 0, sizeof(iosend));
		msgsend.msg_name = &client_addr;
		msgsend.msg_namelen = sizeof(client_addr);
		;
		msgsend.msg_iov = iosend;
		msgsend.msg_iovlen = 1;
		iosend[0].iov_base = large_buffer;
		iosend[0].iov_len = sizeof(large_buffer);
		ret = sendmsg(accept_fd, &msgsend, 0);
		chcore_assert(ret == sizeof(large_buffer),
			      "[server] send return error!");
	} else if (accept_fd % 5 == 2) {
		ret = send(accept_fd, large_buffer, sizeof(large_buffer), 0);
		chcore_assert(ret == sizeof(large_buffer),
			      "[server] send return error!");

	} else {
		ret = write(accept_fd, large_buffer, sizeof(large_buffer));
		chcore_assert(ret == sizeof(large_buffer),
			      "[server] send return error!");
	}
	info("[server] receive from %s:%d send msg len:%d\n", IP, port, ret);
	ret = close(accept_fd);
	chcore_assert(ret >= 0, "[server] close return error!");

	return 0;
}

int main(int argc, char *argv[], char *envp[])
{

	int server_fd = 0;
	int ret = 0;
	int accept_fd = 0;
	struct sockaddr_in my_addr;
	struct sockaddr_in sa, ac;
	socklen_t len;
	int type = 0;
	cpu_set_t mask;

	/* set aff */
	CPU_ZERO(&mask);
	CPU_SET(2, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	chcore_assert(ret == 0, "Set affinity failed!");
	sched_yield();

	do {
		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd >= 0) /* succ */
			break;
		printf("create socket failed! errno = %d\n", errno);
		sched_yield();
	} while (1);

	ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1},
			 sizeof(int));
	chcore_assert(ret >= 0, "[server] setsockopt return error!");

	len = sizeof(int);
	ret = getsockopt(server_fd, SOL_SOCKET, SO_TYPE, &type, &len);
	chcore_assert(ret >= 0 && type == SOCK_STREAM,
		      "[server] getsockopt return error!");

	memset(&sa, 0, sizeof(struct sockaddr));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(NET_SERVERPORT);
	ret =
	    bind(server_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
	chcore_assert(ret >= 0, "[server] bind return error!");

	ret = listen(server_fd, 5);
	chcore_assert(ret >= 0, "[server] listen return error!");

	bzero(&my_addr, sizeof(my_addr));
	len = sizeof(my_addr);

	ret = getsockname(server_fd, (struct sockaddr *)&my_addr, &len);
	chcore_assert(ret >= 0, "[server] getsockname return error!");

	info("LWIP tcp server up!\n");

	do {
		len = sizeof(struct sockaddr_in);

		do {
			accept_fd =
			    accept(server_fd, (struct sockaddr *)&ac, &len);
			if (accept_fd >= 0) /* succ */
				break;
			printf("accept socket failed! errno = %d\n", errno);
			sched_yield();
		} while (1);
		pthread_t tid;
		pthread_create(&tid, NULL, handle_thread,
			       (void *)(long)accept_fd);
	} while (1);

	ret = close(server_fd);
	chcore_assert(ret >= 0, "[server] close return error!");

	return 0;
}
