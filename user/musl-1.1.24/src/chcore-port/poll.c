#include <atomic.h>
#include <bits/alltypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall_arch.h>

#include <chcore/container/list.h>
#include <chcore/ipc.h>
#include <chcore/lwip_defs.h>

#include "fd.h"
#include "poll.h"

#include "socket.h"  /* socket server poll function */
#include "timerfd.h" /* timerfd async poll function */

// #define chcore_poll_debug

#ifdef chcore_poll_debug
#define poll_debug(fmt, ...)                                                   \
	printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define poll_debug(fmt, ...)
#endif

/* Poll server functions */

/*
 * For files which services are provided by their servers,
 * call the poll function at server side and create a seperate thread to
 * wait the poll function return, than it will notify the notifc.
 */

/* Register the server by fd */
#define SOCK_SERVER_POLL 0x1
#define TIMER_ASYNC_POLL 0x2
#define SAMPLE_SERVER_POLL2 0x4
/* XXX: may use a more elegant way? */
static void reg_async_poll(int fd, unsigned long *flag)
{
	switch (fd_dic[fd]->type) {
	case FD_TYPE_SOCK:
		*flag |= SOCK_SERVER_POLL;
		break;
	case FD_TYPE_TIMER:
		*flag |= TIMER_ASYNC_POLL;
	}
}

/* call server poll function */
/* XXX: may use a more elegant way? */
static int call_async_poll(struct pollfd fds[], nfds_t nfds, int timeout,
			   unsigned long flag, int notifc_cap)
{
	int count = 0;
	int ret = 0;

	if (flag & SOCK_SERVER_POLL) {
		/* Call socket server poll */
		ret = chcore_socket_server_poll(fds, nfds, timeout, notifc_cap);
		if (ret < 0)
			goto fail;
		count += ret;
	}

	if (flag & TIMER_ASYNC_POLL) {
		ret = chcore_timerfd_async_poll(fds, nfds, timeout, notifc_cap);
		if (ret < 0)
			goto fail;
		count += ret;
	}
	return count;
fail:
	return ret;
}

/* epoll operation */

int chcore_epoll_create1(int flags)
{
	int epfd = 0, ret = 0;
	struct fd_desc *epoll_fd_desc;
	struct eventpoll *ep;

	epfd = alloc_fd();
	if (epfd < 0) {
		ret = epfd;
		goto fail;
	}

	if ((ep = malloc(sizeof(*ep))) == NULL) {
		ret = -ENOMEM;
		goto fail;
	}
	ep->epi_lock = 0;
	init_list_head(&ep->epi_list);
	ep->wait_count = 0;

	epoll_fd_desc = fd_dic[epfd];
	epoll_fd_desc->fd = epfd;
	epoll_fd_desc->type = FD_TYPE_EPOLL;
	epoll_fd_desc->private_data = ep;
	epoll_fd_desc->flags = flags;
	epoll_fd_desc->fd_op = &epoll_ops;

	return epfd;

fail:
	if (epfd >= 0)
		free_fd(epfd);
	if (ep > 0)
		free(ep);
	return ret;
}

int chcore_epoll_ctl(int epfd, int op, int fd, struct epoll_event *events)
{
	struct epitem *epi = NULL, *tmp = NULL;
	struct eventpoll *ep;
	int ret = 0;
	int fd_exist = 0;

	/* EINVAL events is NULL */
	if (events == NULL)
		return -EINVAL;

	/* EBADF  epfd or fd is not a valid file descriptor. */
	if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL)
		return -EBADF;

	/* EBADF  epfd or fd is not a valid file descriptor. */
	if (epfd < 0 || epfd >= MAX_FD || fd_dic[epfd] == NULL ||
	    fd_dic[epfd]->type != FD_TYPE_EPOLL ||
	    (ep = fd_dic[epfd]->private_data) == NULL)
		return -EBADF;

	/* EINVAL epfd is not an epoll file descriptor, or fd is the same as
	 * epfd */
	if (fd == epfd || fd_dic[epfd]->type != FD_TYPE_EPOLL)
		return -EINVAL;

	/* ELOOP  fd refers to an epoll instance and this EPOLL_CTL_ADD
	 * operation would result in a circular loop of epoll instances
	 * monitoring one another. (CHCORE NOT SUPPORT HERE) */
	if (fd_dic[fd]->type == FD_TYPE_EPOLL)
		return -ELOOP;

	/* EPERM  The target file fd does not support epoll.  This error can
	 * occur if fd refers to, for example, a regular file or a directory. */
	if (fd_dic[fd]->fd_op == NULL)
		return -EPERM;

	chcore_spin_lock(&ep->epi_lock);
	switch (op) {
	case EPOLL_CTL_ADD:
		/* Check if already exist */
		for_each_in_list_safe(epi, tmp, epi_node, &ep->epi_list)
		{
			if (epi->fd == fd) {
				/* EEXIST op was EPOLL_CTL_ADD, and the supplied
				 * file descriptor fd is already registered with
				 * this epoll instance. */
				ret = -EEXIST;
				goto out;
			}
		}
		if ((epi = malloc(sizeof(*epi))) == NULL) {
			/* ENOMEM There was insufficient memory to handle the
			 * requested op control operation. */
			ret = -ENOMEM;
			goto out;
		}
		epi->fd = fd;
		epi->event = *events;
		list_append(&epi->epi_node, &ep->epi_list);
		ep->wait_count++;
		ret = 0;
		break;
	case EPOLL_CTL_MOD:
		/* Check if exist */
		fd_exist = 0;
		for_each_in_list_safe(epi, tmp, epi_node, &ep->epi_list)
		{
			if (epi->fd == fd) {
				epi->event = *events; /* Update the event */
				fd_exist = 1;	      /* set the flag */
				break;
			}
		}
		/* ENOENT op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not
		 * registered with this epoll instance. */
		if (fd_exist == 0)
			ret = -ENOENT;
		break;
	case EPOLL_CTL_DEL:
		/* Check if exist */
		fd_exist = 0;
		for_each_in_list_safe(epi, tmp, epi_node, &ep->epi_list)
		{
			if (epi->fd == fd) {
				list_del(&epi->epi_node); /* delete the node */
				fd_exist = 1;		  /* set the flag */
				break;
			}
		}
		/* ENOENT op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not
		 * registered with this epoll instance. */
		if (fd_exist == 0)
			ret = -ENOENT;
		break;
	default:
		/* The requested operation op is not supported by this interface
		 */
		ret = -ENOSYS;
	}

out:
	chcore_spin_unlock(&ep->epi_lock);
	return ret;
}

/*
 * XXX: Convert epoll to poll, which is not efficient as all fds
 * are traserved.
 */
int chcore_epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
		       int timeout, const sigset_t *sigmask)
{
	struct eventpoll *ep;
	struct epitem *epi = NULL, *tmp = NULL;
	struct pollfd *fds;
	epoll_data_t *epdata;
	int i = 0, ret = 0, ev_idx = 0, nfds = 0;
	sigset_t origmask;

	/* EBADF  epfd or fd is not a valid file descriptor. */
	if (epfd < 0 || epfd >= MAX_FD || fd_dic[epfd] == NULL ||
	    fd_dic[epfd]->type != FD_TYPE_EPOLL ||
	    (ep = fd_dic[epfd]->private_data) == NULL)
		return -EBADF;

	/* EINVAL epfd is not an epoll file descriptor, or maxevents is less
	 * than or equal to zero. */
	if (fd_dic[epfd]->type != FD_TYPE_EPOLL || maxevents <= 0)
		return -EINVAL;

	/* EFAULT The memory area pointed to by events is not accessible with
	 * write permissions. */
	if (events == NULL)
		return -EFAULT;

	if (sigmask)
		pthread_sigmask(SIG_SETMASK, sigmask, &origmask);

	chcore_spin_lock(&ep->epi_lock);
	if ((fds = (struct pollfd *)malloc(ep->wait_count * sizeof(*fds))) ==
	    NULL) {
		chcore_spin_unlock(&ep->epi_lock);
		return -ENOMEM;
	}
	if ((epdata = (epoll_data_t *)malloc(ep->wait_count *
					     sizeof(epoll_data_t))) == NULL) {
		free(fds);
		chcore_spin_unlock(&ep->epi_lock);
		return -ENOMEM;
	}

	/* Convert epoll epi_list to fds */
	/* XXX: can be optimized by using the same format to store epoll_list */
	nfds = 0; /* index of fds */
	for_each_in_list_safe(epi, tmp, epi_node, &ep->epi_list)
	{
		fds[nfds].fd = epi->fd;
		/* EPOLLEXCLUSIVE, EPOLLWAKEUP, EPOLLONESHOT and EPOLLET are
		 * special */
		if (epi->event.events & EPOLLEXCLUSIVE ||
		    epi->event.events & EPOLLWAKEUP ||
		    epi->event.events & EPOLLONESHOT ||
		    epi->event.events & EPOLLET)
			WARN("EPOLLEXCLUSIVE, EPOLLWAKEUP, EPOLLONESHOT and "
			     "EPOLLET not supported!");
		fds[nfds].events = epi->event.events & ~EPOLLEXCLUSIVE &
				   ~EPOLLWAKEUP & ~EPOLLONESHOT & ~EPOLLET;
		/* Copy epdata */
		epdata[nfds] = epi->event.data;
		nfds++;
	}
	chcore_spin_unlock(&ep->epi_lock);

	ret = chcore_poll(fds, nfds, timeout);
	if (ret > 0) {
		ev_idx = 0; /* index of events */
		for (i = 0; i < nfds && ev_idx < maxevents; i++) {
			if ((fds[i].revents & fds[i].events)) {
				events[ev_idx].events = fds[i].revents;
				events[ev_idx].data = epdata[i];
				ev_idx++;
			}
		}
	}
	poll_debug("epoll events:%d,%d\n", ret, ev_idx);
out:
	free(fds);
	free(epdata);
	if (sigmask)
		pthread_sigmask(SIG_SETMASK, &origmask, NULL);
	return ret;
}

static int chcore_epoll_read(int fd, void *buf, size_t size)
{
	WARN("EPOLL CANNOT READ!\n");
	return -EINVAL;
}

static int chcore_epoll_write(int fd, void *buf, size_t size)
{
	WARN("EPOLL CANNOT WRITE!\n");
	return -EINVAL;
}

static int chcore_epoll_close(int fd)
{
	/* TODO Recycle threads */
	if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL ||
	    fd_dic[fd]->type != FD_TYPE_EPOLL ||
	    fd_dic[fd]->private_data == NULL)
		return -EBADF;

	free(fd_dic[fd]->private_data);
	free_fd(fd);
	return 0;
}

static int chcore_epoll_ioctl(int fd, unsigned long req, void *arg)
{
	WARN("EPOLL not support ioctl!\n");
	return 0;
}

struct fd_ops epoll_ops = {
    .read = chcore_epoll_read,
    .write = chcore_epoll_write,
    .close = chcore_epoll_close,
    .poll = NULL,
    .ioctl = chcore_epoll_ioctl,
};

int chcore_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
	unsigned long poll_server_flag = 0; /* Bitmap to record the server */
	int i, mask, ret;
	struct pollarg arg;
	int count = 0;
	int timed_out = 0;
	int notifc_cap = 0;
	struct timespec timeout_ts;

	/*
	 * EFAULT fds points outside the process's accessible address space.
	 * The array given as argument was not contained in the calling
	 * program's address space.
	 */
	if (fds == 0)
		return -EFAULT;

	/* XXX: stdin for shell */
	if (nfds == 1) {
		int fd = fds[0].fd;
		if (fd >= 0 && fd <= MAX_FD &&
		    fd_dic[fd] && fd_dic[fd]->type == FD_TYPE_STDIN) {
			fds[0].revents = POLLIN;
			return 1;
		}
	}

	for (i = 0; i < nfds; i++) {
		if (fds[i].fd < 0 || fds[i].fd >= MAX_FD) {
			printf("fd number not supported\n");
			return -EINVAL;
		}
		if (fd_dic[fds[i].fd] == NULL)
			continue;
		/* TODO: current only support lwip */
		if (fd_dic[fds[i].fd]->cap != lwip_server_cap) {
			printf("fd server is not lwip\n");
			return -EINVAL;
		}
	}

	if (nfds == 0)
		return 0;

	/* Allocate notification */
	if ((notifc_cap = chcore_syscall0(CHCORE_SYS_create_notifc)) < 0) {
		ret = notifc_cap;
		goto fail;
	}
	poll_debug("Poll nfds:%d fds %p timeout:%d notifc %d\n", nfds, fds,
		   timeout, notifc_cap);

	/* Clear revents */
	for (i = 0; i < nfds; i++) {
		fds[i].revents = 0;
	}

	for (;;) {
		for (i = 0; i < nfds; i++) {
			/*
			 * The field fd contains a file descriptor for an open
			 * file.  If this field is negative, then the
			 * corresponding events field is ignored and the revents
			 * field returns zero.  (This provides an easy way of
			 * ignoring a file descriptor for a single poll()
			 * call: simply negate the fd field.  Note, however,
			 * that this technique can't be used to ignore file
			 * descriptor 0.)
			 */
			if (fds[i].fd < 0) {
				/* False fd, just ignore them */
				fds[i].revents = 0;
				continue;
			}

			/*
			 * Invalid request: fd not open (only returned in
			 * revents; ignored in events). Or target fd does not
			 * support poll function
			 */
			if (fds[i].fd >= MAX_FD || fd_dic[fds[i].fd] == 0 ||
			    fd_dic[fds[i].fd]->fd_op == 0 ||
			    fd_dic[fds[i].fd]->fd_op->poll == 0) {
				fds[i].revents = POLLNVAL;
				continue;
			}

			arg.notifc_cap = notifc_cap;
			arg.events = fds[i].events;

			/* Call fd poll function */
			if ((mask = fd_dic[fds[i].fd]->fd_op->poll(fds[i].fd,
								   &arg))) {
				/* Already achieve the requirement */
				fds[i].revents = mask;
				count++;
			}
			poll_debug("poll fd %d mask %d\n", fds[i].fd, mask);
			/* Check whether we need to call a seperate poll
			 * function */
			reg_async_poll(fds[i].fd, &poll_server_flag);
		}

		poll_debug("call_async_poll nonblocking\n");
		/* Call service poll function => Batched non-blocking */
		if ((ret = call_async_poll(fds, nfds, 0, poll_server_flag,
					   notifc_cap)) < 0)
			goto fail;

		poll_debug("call_async_poll with timeout\n");

		if (ret > 0 || count || timed_out || timeout == 0) {
			count = ret > 0 ? count + ret : count;
			break;
		}

		/* Call server poll function => create new thread to poll */
		if ((ret = call_async_poll(fds, nfds, timeout, poll_server_flag,
					   notifc_cap)) < 0)
			goto fail;
		/* Some may ready here */
		if (ret > 0) {
			count = ret;
			break;
		}

		poll_debug("poll wait notifc %d\n", notifc_cap);
		/* Wait notifc */
		if (timeout > 0) {
			/* Find out the right timeout */
			timeout_ts.tv_sec = timeout / 1000;
			timeout_ts.tv_nsec = (timeout % 1000) * 1000000;
			ret = chcore_syscall3(CHCORE_SYS_wait, notifc_cap, true,
					      (long)&timeout_ts);
			/* XXX: calculate accurate timeout is slow (multiple
			 * syscall), directly return when notified */
			timed_out = 1;
		} else {
			ret = chcore_syscall3(CHCORE_SYS_wait, notifc_cap, true,
					      0);
		}
		if (ret == -EAGAIN) {
			timed_out = 1; /* Will direct return next time */
			poll_debug("poll timeout!\n");
		}
		poll_debug("up by %d timed_out %d\n", notifc_cap, timed_out);
	}
out:
	poll_debug("poll return count %d\n", count);
	/* TODO: Free notifc (not supported yet) */
	return count;
fail:
	poll_debug("poll fail return ret %d\n", ret);
	/* TODO: Free notifc (not supported yet) */
	return ret;
}

int chcore_ppoll(struct pollfd *fds, nfds_t n, const struct timespec *tmo_p,
		 const sigset_t *sigmask)
{
	int timeout;
	int ret;
	sigset_t origmask;

	/* EINVAL (ppoll()) The timeout value expressed in *ip is invalid
	 * (negative).*/
	if (tmo_p != NULL && (tmo_p->tv_sec < 0 || tmo_p->tv_nsec < 0))
		return -EINVAL;

	if (sigmask)
		pthread_sigmask(SIG_SETMASK, sigmask, &origmask);
	timeout = (tmo_p == NULL)
		      ? -1
		      : (tmo_p->tv_sec * 1000 + tmo_p->tv_nsec / 1000000);
	ret = chcore_poll(fds, n, timeout);
	if (sigmask)
		pthread_sigmask(SIG_SETMASK, &origmask, NULL);
	return ret;
}
