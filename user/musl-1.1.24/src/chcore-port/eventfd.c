#include <atomic.h>
#include <debug_lock.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall_arch.h>
#include <time.h>
#include <pthread.h>

#include <chcore/bug.h>
#include <chcore/container/list.h>
#include <chcore/defs.h>

#include "eventfd.h"
#include "fd.h"
#include "poll.h"

struct eventfd {
	uint64_t efd_val; /* 64-bit efd_val */
	int volatile efd_lock;
	int writer_notifc_cap;
	uint64_t writer_waiter_cnt;
	int reader_notifc_cap;
	uint64_t reader_waiter_cnt;
	struct list_head poll_list;
};

struct poll_node {
	int notifc_cap;
	int events;
	struct list_head list_entry;
};

int chcore_eventfd(unsigned int count, int flags)
{
	int fd = 0;
	int ret = 0;
	struct eventfd *efdp = 0;
	int notifc_cap = 0;

	if ((fd = alloc_fd()) < 0) {
		ret = fd;
		goto fail;
	}
	fd_dic[fd]->flags = flags;
	fd_dic[fd]->type = FD_TYPE_EVENT;
	fd_dic[fd]->fd_op = &event_op;

	if ((efdp = malloc(sizeof(struct eventfd))) <= 0) {
		ret = -ENOMEM;
		goto fail;
	}
	/* use efd_val to store the initval */
	efdp->efd_val = count;
	/* init per eventfd lock */
	efdp->efd_lock = 0;
	/* alloc notification for eventfd */
	if ((notifc_cap = chcore_syscall0(CHCORE_SYS_create_notifc)) < 0) {
		ret = notifc_cap;
		goto fail;
	}
	efdp->reader_notifc_cap = notifc_cap;
	efdp->reader_waiter_cnt = 0;

	if ((notifc_cap = chcore_syscall0(CHCORE_SYS_create_notifc)) < 0) {
		ret = notifc_cap;
		goto fail;
	}
	efdp->writer_notifc_cap = notifc_cap;
	efdp->writer_waiter_cnt = 0;

	/* init poll_list */
	init_list_head(&efdp->poll_list);

	a_barrier();
	fd_dic[fd]->private_data = efdp;
out:
	return fd;
fail:
	if (fd >= 0)
		free_fd(fd);
	if (efdp > 0)
		free(efdp);
	return ret;
}

static int chcore_eventfd_read(int fd, void *buf, size_t count)
{
	/* Already checked fd before call this function */
	BUG_ON(!fd_dic[fd]->private_data);

	/* A read fails with the error EINVAL if the size
	   of the supplied buffer is less than 8 bytes. */
	if (count < sizeof(uint64_t))
		return -EINVAL;

	struct eventfd *efdp = fd_dic[fd]->private_data;
	uint64_t val = 0, i = 0;
	int ret = 0;
	struct poll_node *iter = NULL, *tmp = NULL;

again:
	chcore_spin_lock(&efdp->efd_lock);

	/* Fast Path */
	if ((val = efdp->efd_val) != 0) {
		if (fd_dic[fd]->flags & EFD_SEMAPHORE) {
			/* EFD as semaphore */
			efdp->efd_val--;
			*(uint64_t *)buf = 1;
			/* At most 1 writer can be waked */
			if (efdp->writer_waiter_cnt) {
				ret = chcore_syscall1(CHCORE_SYS_notify,
						      efdp->writer_notifc_cap);
				efdp->writer_waiter_cnt -= 1;
			}
		} else {
			efdp->efd_val = 0;
			*(uint64_t *)buf = val;
			/* At most `val` writer can be waked */
			if (efdp->writer_waiter_cnt) {
				for (i = 0;
				     i < efdp->writer_waiter_cnt && i < val;
				     i++)
					ret = chcore_syscall1(
					    CHCORE_SYS_notify,
					    efdp->writer_notifc_cap);
				efdp->writer_waiter_cnt -= i;
			}
		}

		ret = sizeof(uint64_t);
		goto out;
	}

	/* Slow Path */
	if (fd_dic[fd]->flags & EFD_NONBLOCK) {
		ret = -EAGAIN;
		goto out;
	}

	/* going to wait notific */
	BUG_ON(efdp->reader_waiter_cnt == 0xffffffffffffffff);
	efdp->reader_waiter_cnt++;
	chcore_spin_unlock(&efdp->efd_lock);

	ret =
	    chcore_syscall3(CHCORE_SYS_wait, efdp->reader_notifc_cap, true, 0);
	BUG_ON(ret);
	goto again;

out:
	/* Wake suitable poller in poll_list */
	if (!list_empty(&efdp->poll_list)) {
		for_each_in_list_safe(iter, tmp, list_entry, &efdp->poll_list)
		{
			if (iter->events & POLLOUT ||
			    iter->events & POLLWRNORM) {
				chcore_syscall1(CHCORE_SYS_notify,
						iter->notifc_cap);
				list_del(&iter->list_entry);
				free(iter);
			}
		}
	}
	chcore_spin_unlock(&efdp->efd_lock);
	return ret;
}

static int chcore_eventfd_write(int fd, void *buf, size_t count)
{
	/* Already checked fd before call this function */
	BUG_ON(!fd_dic[fd]->private_data);

	if (count < sizeof(uint64_t))
		return -EINVAL;
	if (*(uint64_t *)buf == 0xffffffffffffffff)
		return -EINVAL;

	struct eventfd *efdp = fd_dic[fd]->private_data;
	int ret = 0;
	uint64_t i = 0;
	struct poll_node *iter = NULL, *tmp = NULL;

again:
	/* Should wait until all value has written */
	chcore_spin_lock(&efdp->efd_lock);
	/* Fast Path */
	if (*(uint64_t *)buf < 0xffffffffffffffff - efdp->efd_val) {
		efdp->efd_val += *(uint64_t *)buf;
		if (efdp->reader_waiter_cnt) {
			/* wake up reader */
			if (fd_dic[fd]->flags & EFD_SEMAPHORE) {
				/* sem mode */
				for (i = 0; i < efdp->reader_waiter_cnt &&
					    i < efdp->efd_val;
				     i++)
					ret = chcore_syscall1(
					    CHCORE_SYS_notify,
					    efdp->reader_notifc_cap);
				efdp->reader_waiter_cnt -= i;
			} else {
				ret = chcore_syscall1(CHCORE_SYS_notify,
						      efdp->reader_notifc_cap);
				efdp->reader_waiter_cnt -= 1;
			}
		}
		BUG_ON(ret != 0);
		ret = sizeof(uint64_t);
		goto out;
	}

	/* Slow Path */
	if (fd_dic[fd]->flags & EFD_NONBLOCK) {
		ret = -EAGAIN;
		goto out;
	}

	/* going to wait notific */
	BUG_ON(efdp->writer_waiter_cnt == 0xffffffffffffffff);
	efdp->writer_waiter_cnt++;
	chcore_spin_unlock(&efdp->efd_lock);

	printf("%p go to sleep\n", pthread_self());
	ret =
	    chcore_syscall3(CHCORE_SYS_wait, efdp->writer_notifc_cap, true, 0);
	BUG_ON(ret);
	goto again;
out:
	/* Wake suitable poller in poll_list */
	if (!list_empty(&efdp->poll_list)) {
		for_each_in_list_safe(iter, tmp, list_entry, &efdp->poll_list)
		{
			if (iter->events & POLLIN ||
			    iter->events & POLLRDNORM) {
				// printf("fd %d notify %d efdp->efd_val %d\n",
				// fd, iter->notifc_cap, efdp->efd_val);
				chcore_syscall1(CHCORE_SYS_notify,
						iter->notifc_cap);
				list_del(&iter->list_entry);
				free(iter);
			}
		}
	}
	chcore_spin_unlock(&efdp->efd_lock);
	return ret;
}

static int chcore_eventfd_close(int fd)
{
	BUG_ON(!fd_dic[fd]->private_data);
	struct eventfd *efdp = fd_dic[fd]->private_data;
	struct poll_node *iter = NULL, *tmp = NULL;

	/* Free the poll list */
	if (!list_empty(&efdp->poll_list)) {
		for_each_in_list_safe(iter, tmp, list_entry, &efdp->poll_list)
		    free(iter);
	}

	free(fd_dic[fd]->private_data);
	free_fd(fd);
	return 0;
}

static int chcore_eventfd_poll(int fd, struct pollarg *arg)
{
	/* Already checked fd before call this function */
	BUG_ON(!fd_dic[fd]->private_data);
	int mask = 0;
	struct eventfd *efdp = fd_dic[fd]->private_data;
	struct poll_node *pnode;
	struct poll_node *iter = NULL, *tmp = NULL;

	/* Only check no need to lock */
	if (arg->events & POLLIN || arg->events & POLLRDNORM) {
		// printf("CHECK POLLIN fd %d efdp->efd_val %d\n", fd,
		// efdp->efd_val);
		/* Check whether can read */
		mask |= efdp->efd_val ? POLLIN | POLLRDNORM : 0;
	}

	/* Only check no need to lock */
	if (arg->events & POLLOUT || arg->events & POLLWRNORM) {
		/* Check whether can write */
		mask |= efdp->efd_val < 0xfffffffffffffffe
			    ? POLLOUT | POLLWRNORM
			    : 0;
	}

	/* Fast path, no need to delete (we will update events next time amd the
	 * freed notifc won't case fault) */
	if (mask)
		goto out;

	/* Add notifc_cap to private data */
	chcore_spin_lock(&efdp->efd_lock);
	/* Check whether the notifc is already exist */
	if (!list_empty(&efdp->poll_list)) {
		for_each_in_list_safe(iter, tmp, list_entry, &efdp->poll_list)
		{
			if (iter->notifc_cap == arg->notifc_cap) {
				/* update the events (if the old has been freed
				 * and new notifc use the same cap) */
				iter->events = arg->events;
				goto unlock_out;
			}
		}
	}
	/* Add new pnode to the poll list */
	pnode = malloc(sizeof(struct poll_node));
	pnode->notifc_cap = arg->notifc_cap;
	pnode->events = arg->events;
	list_append(&pnode->list_entry, &efdp->poll_list);
unlock_out:
	chcore_spin_unlock(&efdp->efd_lock);
out:
	return mask;
}

static int chcore_eventfd_ioctl(int fd, unsigned long req, void *arg)
{
	WARN("EVENTFD not support ioctl!\n");
	return 0;
}

/* EVENTFD */
struct fd_ops event_op = {
    .read = chcore_eventfd_read,
    .write = chcore_eventfd_write,
    .close = chcore_eventfd_close,
    .poll = chcore_eventfd_poll,
    .ioctl = chcore_eventfd_ioctl,
};