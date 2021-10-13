#pragma once

int chcore_chdir(const char *path);
int chcore_fchdir(int fd);
int chcore_getcwd(char *buf, size_t size);
int chcore_ftruncate(int fd, off_t length);
int chcore_lseek(int fd, off_t offset, int whence);
int chcore_mkdirat(int dirfd, const char *pathname, mode_t mode);
int chcore_unlinkat(int dirfd, const char *pathname, int flags);
int chcore_symlinkat(const char *target, int newdirfd, const char *linkpath);
int chcore_getdents64(int fd, char *buf, int count);
int chcore_fcntl(int fd, int cmd, int arg);
int chcore_readlinkat(int dirfd, const char *pathname, char *buf,
		      size_t bufsiz);
int chcore_renameat(int olddirfd, const char *oldpath,
		    int newdirfd, const char *newpath);
int chcore_faccessat(int dirfd, const char *pathname, int mode, int flags);
int chcore_fallocate(int fd, int mode, off_t offset, off_t len);
int chcore_statx(int dirfd, const char *pathname, int flags,
		 unsigned int mask, char *buf);
int chcore_fsync(int fd);
int chcore_openat(int dirfd, const char *pathname, int flags, mode_t mode);
