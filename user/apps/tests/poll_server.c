#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <poll.h>
#include <errno.h>

#include "tests.h"

/*
 * This file generates poll_server.bin and epoll_server.bin.
 * Use poll by default unless USE_EPOLL is defined.
 */
#ifndef USE_EPOLL
#define USE_POLL
#endif

#define MAX_CLIENT 64

#ifdef USE_POLL
/* Handle fds get by poll. Returns number of fd closed */
static int handle_poll_fds(struct pollfd *pfds, int count)
{
	int i, ret;
	const int len = 256;
	char data_buf[len];
	int closed_count = 0;

	for (i = 0; i < MAX_CLIENT; i++) {
		if ((pfds[i].revents & POLLIN) != 0) {
			ret = recv(pfds[i].fd, data_buf, len, 0);
			data_buf[ret] = 0;
			chcore_assert(ret >= 0, "recv error");
			if (ret > 0) {
				printf("[client:%d] recv:\n%s", i, data_buf);
			} else {
				close(pfds[i].fd);
				closed_count++;
			}
		}
	}
	return closed_count;
}
#endif

#ifdef USE_EPOLL
/* Handle events get by epoll. Returns number of fd closed */
static int handle_poll_events(struct epoll_event *events, int count)
{
	int i, ret;
	const int len = 256;
	char data_buf[len];
	int closed_count = 0;

	for (i = 0; i < count; i++) {
		if ((events[i].events & POLLIN) != 0) {
			ret = recv(events[i].data.fd, data_buf, len, 0);
			data_buf[ret] = 0;
			chcore_assert(ret >= 0, "recv error");
			if (ret > 0) {
				printf("[client:%d] recv %s", i, data_buf);
			} else {
				printf("[client:%d] close\n", i);
				close(events[i].data.fd);
				closed_count++;
			}
		}
	}
	return closed_count;
}
#endif

const int sleep_time[] = { -1, 0, 1, 1, 50, 200 };
static int idx_to_time(int idx)
{
	return sleep_time[idx % (sizeof(sleep_time) / sizeof(sleep_time[0]))];
}

int main(int argc, char *argv[], char *envp[])
{

	int server_fd = 0, accept_fd = 0;
	int ret = 0, i;
	struct sockaddr_in sa, ac;
	socklen_t socklen;
	int closed_count;
	int event_count;
#ifdef USE_EPOLL
	const char* poll_method = "epoll";
	struct epoll_event event, events[MAX_CLIENT];
	int epoll_fd = epoll_create1(0);

	if(epoll_fd == -1)
	{
		fprintf(stderr, "Failed to create epoll file descriptor\n");
		return 1;
	}
#elif defined(USE_POLL)
	const char* poll_method = "poll";
	struct pollfd pfds[MAX_CLIENT];
#endif

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	chcore_assert(ret >= 0, "[server] create socket return error!");

	memset(&sa, 0, sizeof(struct sockaddr));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	sa.sin_port = htons(9001);
	ret = bind(server_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
	chcore_assert(ret >= 0, "[server] bind return error!");

	ret = listen(server_fd, MAX_CLIENT);
	chcore_assert(ret >= 0, "[server] listen return error!");

	printf("LWIP %s server up!\n", poll_method);

	socklen = sizeof(struct sockaddr_in);
	for (i = 0; i < MAX_CLIENT; i++) {
		accept_fd = accept(server_fd, (struct sockaddr *)&ac, &socklen);
		chcore_assert(accept_fd >= 0, "[server] accept return error!");
		// printf("accept %d\n", accept_fd);
#ifdef USE_EPOLL
		event.events = EPOLLIN;
		event.data.fd = accept_fd;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &event))
		{
			printf("failed to add fd to epoll\n");
			close(epoll_fd);
			return 1;
		}
#elif defined(USE_POLL)
		pfds[i].fd = accept_fd;
		pfds[i].events = POLLIN | POLLERR;
#endif
	}

	closed_count = 0;
	i = 0;
	while (closed_count != MAX_CLIENT) {
		i++;
#ifdef USE_EPOLL
		event_count = epoll_wait(epoll_fd, events, MAX_CLIENT,
					 idx_to_time(i));
#elif defined(USE_POLL)
		event_count = poll(pfds, MAX_CLIENT, idx_to_time(i));
#endif
		if (event_count < 0) {
			printf("poll ret %d errno %d\n", event_count, errno);
			return event_count;
		} else if (event_count == 0) {
			continue;
		}

#ifdef USE_EPOLL
		closed_count += handle_poll_events(events, event_count);
#elif defined(USE_POLL)
		closed_count += handle_poll_fds(pfds, event_count);
#endif
		chcore_assert(closed_count <= MAX_CLIENT, "wrong closing");
	}

	ret = close(server_fd);
	chcore_assert(ret >= 0, "[server] close return error!");

	printf("LWIP %s server done!\n", poll_method);

	return 0;
}
