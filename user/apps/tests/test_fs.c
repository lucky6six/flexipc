#define _GNU_SOURCE

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/mman.h>

#include "tests.h"

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_YELLOW_BG "\033[33;3m"
#define COLOR_DEFAULT "\033[0m"

#define GREEN_OK COLOR_GREEN"[OK]"COLOR_DEFAULT
#define YELLOW_START COLOR_YELLOW"[TEST]"COLOR_DEFAULT

#define test_start(fmt, ...) \
	printf(YELLOW_START" "fmt, ##__VA_ARGS__)

#define test_ok(fmt, ...) \
	printf(GREEN_OK" "fmt, ##__VA_ARGS__)


#define FILE_SIZE 10000

char buf1[FILE_SIZE];
char buf2[FILE_SIZE];

void xm(char *p, int len);
void random_char(char *p, int len);

int test_cwd()
{
	char buf[256];
	int ret;

	test_start("cwd\n");

	/* Check current wd. */
	buf[0] = 'a';
	getcwd(buf, 256);
	assert(strcmp("/", buf) == 0);

	/* Create, change to new dir, and check. */
	ret = mkdir("/newcwd", 0);
	assert(ret == 0);
	ret = chdir("/newcwd");
	assert(ret == 0);
	buf[0] = 'a';
	getcwd(buf, 256);
	assert(strcmp("/newcwd", buf) == 0);

	/* Check and chcwd with relative path, and check. */
	ret = mkdir("dir0", 0);
	assert(ret == 0);
	ret = chdir("dir0");
	assert(ret == 0);
	buf[0] = 'a';
	getcwd(buf, 256);
	assert(strcmp("/newcwd/dir0", buf) == 0);

	test_ok("chdir.\n");

	return 0;
}

int main(int argc, char *argv[], char *envp[])
{
	struct stat statbuf;
	struct statfs statfsbuf;
	char *str = "Hello, FS!\n";
	char *buf;
	char buf_arr[1024];
	char *fmap;
	int size;
	int i;
	int fd, fd_dup, fd2;
	int ret = 0;
	int iovcnt;
	struct iovec iov[2];

	info("Hello FS!\n");

	size = strlen(str);
	buf = malloc(size + 1);
	memset(buf, 0, size + 1);

	for (i = 0; i < size; ++i) {
		buf[i] = str[i];
	}

	printf("buf: %p, buf cont: %s\n", buf, buf);

	/* ------------ FS TEST ------------- */

	printf("before openat\n");
	i = openat(2, "/", 0);
	printf("after openat (ret=%d)\n", i);

	printf("before open2\n");
	fd = open("/test.txt", 0);
	printf("after open2 (ret=%d)\n", fd);

	printf("before open3\n");
	i = open("/", 0, 0);
	printf("after open3 (ret=%d)\n", i);

	printf("before open4\n");
	i = open("/hello.bin", 0);
	printf("after open4 (ret=%d)\n", i);

	/* printf("before close\n"); */
	/* i = close(0); */
	/* printf("after close\n"); */

	/* printf("before stat64\n"); */
	/* i = stat64("/", (void *)buf); */
	/* printf("after stat64\n"); */

	printf("before stat\n");
	statbuf.st_mode = 0;
	ret = stat("/", &statbuf);
	assert(statbuf.st_mode == S_IFDIR);
	printf("after stat\n");

	printf("before lstat\n");
	statbuf.st_mode = 0;
	ret = lstat("/", &statbuf);
	assert(statbuf.st_mode == S_IFDIR);
	printf("after lstat\n");

	printf("before fstatat\n");
	statbuf.st_mode = 0;
	ret = fstatat(AT_FDCWD, "/", &statbuf, 0);
	assert(statbuf.st_mode == S_IFDIR);
	printf("after fstatat\n");

	printf("before fstat\n");
	statbuf.st_mode = 0;
	ret = fstat(fd, &statbuf);
	assert(statbuf.st_mode == S_IFREG);
	printf("after fstat\n");

	printf("before statfs\n");
	statfsbuf.f_type = 0;
	ret = statfs("/", &statfsbuf);
	assert(ret == 0);
	assert(statfsbuf.f_type == 0xCCE7A9F5);
	printf("after statfs\n");

	printf("before fstatfs\n");
	statfsbuf.f_type = 0;
	ret = fstatfs(fd, &statfsbuf);
	assert(ret == 0);
	assert(statfsbuf.f_type == 0xCCE7A9F5);
	printf("after fstatfs\n");

	printf("before faccessat-1\n");
	ret = faccessat(/* a bad fd */ -1, "/test.txt", R_OK, 0);
	assert(ret == 0);
	printf("before faccessat-2\n");
	ret = faccessat(AT_FDCWD, "/no-file", F_OK, 0);
	assert(ret == -1);
	printf("after faccessat\n");

	printf("before access\n");
	ret = access("/hello.bin", X_OK);
	assert(ret == 0);
	printf("after access\n");

	/* printf("before statfs64\n"); */
	/* i = statfs64("/", (void *)buf); */
	/* printf("after statfs64\n"); */

	printf("before rename\n");
	i = rename("/", "/");
	printf("after rename\n");

	/* printf("before mkdir\n"); */
	/* i = mkdir("/", 0); */
	/* printf("after mkdir\n"); */

	/* TODO: a larger file is needed to test read/write */
	printf("before read\n");
	i = read(fd, buf, 10);
	buf[10] = '\0';
	printf("after read, buf=%s\n", buf);

	printf("before lseek\n");
	i = lseek(fd, 0, SEEK_SET);
	printf("after lseek\n");

	printf("before write\n");
	i = write(fd, "321", 3);
	printf("after write\n");

	printf("before lseek\n");
	i = lseek(fd, 0, SEEK_SET);
	printf("after lseek\n");

	printf("before read\n");
	i = read(fd, buf, 10);
	buf[10] = '\0';
	printf("after read, buf=%s\n", buf);

	fmap = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	printf("read via mmap, fmap=%p <", fmap, (char *)fmap);
	for (i = 0; i < 10; ++i)
		putchar(*((char *)fmap +i));
	printf(">\n");

	*(char *)fmap = '7';
	printf("read via mmap, fmap=%p <", fmap, (char *)fmap);
	for (i = 0; i < 10; ++i)
		putchar(*((char *)fmap +i));
	printf(">\n");

	i = lseek(fd, 0, SEEK_SET);
	i = read(fd, buf, 10);
	buf[10] = '\0';
	printf("read after mmap buf=%s\n", buf);


	printf("before symlink\n");
	i = symlink("/hello.bin", "/hello.sym");
	printf("after symlink\n");

	printf("before readlink\n");
	i = readlink("/hello.sym", buf_arr, 64);
	buf_arr[64] = '\0';
	printf("  link read:%s\n", buf_arr);
	assert(i == 10);
	printf("after readlink\n");

	printf("before rename 2\n");
	i = rename("/hello.sym", "/hello.sym.renamed");
	printf("after rename 2\n");

	printf("before mkdir\n");
	i = mkdir("/a", 0);
	printf("after mkdir\n");

	printf("before unlink\n");
	i = unlink("/test.txt");
	printf("after unlink\n");

	printf("before open2\n");
	fd = open("/test.txt", O_CREAT);
	printf("after open2 (ret=%d)\n", fd);

	random_char(buf1, FILE_SIZE);
	/* printf("buf1: "); */
	/* xm(buf1, FILE_SIZE); */
	/* printf("buf2: "); */
	/* xm(buf2, FILE_SIZE); */

	printf("before write\n");
	i = write(fd, buf1, FILE_SIZE);
	printf("after write, ret=%d\n", i);

	printf("before lseek\n");
	i = lseek(fd, 0, SEEK_SET);
	printf("after lseek\n");

	printf("before read\n");
	i = read(fd, buf2, FILE_SIZE);
	printf("after read, ret=%d\n", i);

	/* printf("buf1: "); */
	/* xm(buf1, FILE_SIZE); */
	/* printf("buf2: "); */
	/* xm(buf2, FILE_SIZE); */
	if (memcmp(buf1, buf2, FILE_SIZE - 1))
		printf("\033[31mread/write bug\033[0m\n");
	else
		printf("\033[32mread/write test pass\033[0m\n");

	test_cwd();


	printf("before rmdir\n");
	i = rmdir("/a");
	printf("after rmdir\n");

	printf("before getcwd\n");
	getcwd(buf, 10);
	printf("after getcwd: cwd=%s\n", buf);

	printf("before fcntl\n");
	fd_dup = fcntl(fd, F_DUPFD, 10);
	assert(fd_dup >= 10);
	printf("after fcntl-F_DUPFD: fd_dup=%i\n", fd_dup);

	printf("before open for fd2\n");
	fd2 = open("/creat.txt", O_CREAT);
	assert(fd2 >= 0);
	printf("after open for fd2\n");

	printf("before writev\n");
	iov[0].iov_base = "fuck ";
	iov[0].iov_len = 5;
	iov[1].iov_base = "them";
	iov[1].iov_len = 4;
	iovcnt = sizeof(iov) / sizeof(struct iovec);
	i = writev(fd2, iov, iovcnt);
	if (i < 0) {
		printf("writev failed?\n");
	} else {
		printf("i=%i\n", i);
	}
	printf("after writev\n");

	printf("before lseek\n");
	i = lseek(fd, 0, SEEK_SET);
	assert(i == 0);
	printf("after lseek\n");

	printf("before readv\n");
	iov[0].iov_base = buf1;
	iov[0].iov_len = 10;
	iov[1].iov_base = buf2;
	iov[1].iov_len = 10;
	iovcnt = sizeof(iov) / sizeof(struct iovec);
	i = readv(fd2, iov, iovcnt);
	if (i < 0) {
		printf("readv cannot get result?\n");
	} else {
		printf("i=%i\n", i);
		buf1[5] = '\0';
		buf2[5] = '\0';
		printf("buf1=%s\n", buf1);
		printf("buf2=%s\n", buf2);
	}
	printf("after readv\n");

	/* printf("before getdents\n"); */
	/* i = getdents(fd, (void *)buf, 0); */
	/* printf("after getdents\n"); */

	/* printf("before getdents64\n"); */
	/* i = getdents64(fd, (void *)buf, 0); */
	/* printf("after getdents64\n"); */

	close(fd);
	info("FS test finished!\n");

	return 0;
}

void xm(char *p, int len)
{
	int i;
	for (i = 0; i < len; i++)
		putchar(*(p + i));
	putchar('\n');
}

void random_char(char *p, int len)
{
	int i;
	for (i = 0; i < len; i++)
		*(p + i) = random() % 26 + 0x41;
}
