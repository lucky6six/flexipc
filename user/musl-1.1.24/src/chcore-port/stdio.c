/* File operation towards STDIN, STDOUT and STDERR */

#include "fd.h"
#include <assert.h>
#include <chcore/bug.h>
#include <errno.h>
#include <syscall_arch.h>
#include <sys/ioctl.h>
#include <termios.h>

#define warn(fmt, ...) printf("[WARN] " fmt, ##__VA_ARGS__)

/* STDIN */
static int chcore_stdin_read(int fd, void *buf, size_t count)
{
	size_t i;
	int ch;

	if (count <= 0)
		return -EAGAIN;

	/* First char should wait. If we return 0, musl won't read anymore */
	do {
		ch = chcore_syscall0(CHCORE_SYS_getc);
		chcore_syscall0(CHCORE_SYS_yield);
	} while (ch < 0);
	*(char *)buf = ch;
	if (ch == '\r')
		ch = '\n';

	for (i = 1; i < count; ++i) {
		ch = chcore_syscall0(CHCORE_SYS_getc);
		if (ch < 0)
			break;
		*((char *)buf + i) = ch;
	}
	return i;
}

static int chcore_stdin_write(int fd, void *buf, size_t count)
{
	return -EINVAL;
}

static int chcore_stdin_close(int fd)
{
	WARN("Not support close stdin!\n");
	/* TODO Support close stdin */
	return 0;
}

static int chcore_stdio_ioctl(int fd, unsigned long request, void *arg)
{
	/* A fake window size */
	if (request == TIOCGWINSZ) {
		struct winsize *ws;

		ws = (struct winsize *)arg;
		ws->ws_row = 10;
		ws->ws_col = 80;
		ws->ws_xpixel = 0;
		ws->ws_ypixel = 0;
		return 0;
	}
	struct fd_desc *fdesc = fd_dic[fd];
	assert(fdesc);
	switch (request) {
	case TCGETS: {
		struct termios *t = (struct termios*) arg;
		*t = fdesc->termios;
		return 0;
	}
	case TCSETS: {
		struct termios *t = (struct termios*) arg;
		fdesc->termios = *t;
		return 0;
	}
	}
	warn("Unsupported ioctl fd=%d, cmd=0x%lx, arg=0x%lx\n",
	     fd, request, arg);
	return 0;
}

struct fd_ops stdin_ops = {
    .read = chcore_stdin_read,
    .write = chcore_stdin_write,
    .close = chcore_stdin_close,
    .poll = NULL,
    .ioctl = chcore_stdio_ioctl,
};

/* STDOUT */
static int chcore_stdout_read(int fd, void *buf, size_t count)
{
        return -EINVAL;
}

static int chcore_stdout_write(int fd, void *buf, size_t count)
{
        int i;
	for (i = 0; i < count; ++i) {
		char ch = ((char *)buf)[i];
		/* NOTE(MK): Currently we use usys_putc to
		 * implement prints to UART device. Thus we
		 * need to add an additional CR before each LF.
		 * This should be implemented in the kernel in
		 * the future. */
		if (ch == '\n')
			chcore_syscall1(CHCORE_SYS_putc, '\r');
		chcore_syscall1(CHCORE_SYS_putc, ch);
	}
	return count;
}

static int chcore_stdout_close(int fd)
{
        WARN("Not support close stdout!\n");
        /* TODO Support close stdout */
        return 0;
}


struct fd_ops stdout_ops = {
        .read = chcore_stdout_read,
        .write = chcore_stdout_write,
        .close = chcore_stdout_close,
	.poll = NULL,
	.ioctl = chcore_stdio_ioctl,
};

/* STDERR */
static int chcore_stderr_read(int fd, void *buf, size_t count)
{
        return -EINVAL;
}


static int chcore_stderr_write(int fd, void *buf, size_t count)
{
        return chcore_stdout_write(fd, buf, count);
}

static int chcore_stderr_close(int fd)
{
        WARN("Not support close stderr!\n");
        /* TODO Support close stderr */
        return 0;
}

struct fd_ops stderr_ops = {
        .read = chcore_stderr_read,
        .write = chcore_stderr_write,
        .close = chcore_stderr_close,
	.poll = NULL,
	.ioctl = chcore_stdio_ioctl,
};
