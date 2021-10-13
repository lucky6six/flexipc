#pragma once

/*
 * IMPORTANT!
 * These macros are ONLY used to interpret return values from chcore syscalls.
 * Please use macros provided by muclibc for most common cases.
 */
#define CHCORE_EPERM            1      /* Operation not permitted */
#define CHCORE_EAGAIN           2      /* Try again */
#define CHCORE_ENOMEM           3      /* Out of memory */
#define CHCORE_EACCES           4      /* Permission denied */
#define CHCORE_EINVAL           5      /* Invalid argument */
#define CHCORE_EFBIG            6      /* File too large */
#define CHCORE_ENOSPC           7      /* No space left on device */
#define CHCORE_ENOSYS           8      /* Function not implemented */
#define CHCORE_ENODATA          9      /* No data available */
#define CHCORE_ETIME           10      /* Timer expired */
#define CHCORE_ECAPBILITY      11      /* Invalid capability */
#define CHCORE_ESUPPORT        12      /* Not supported */
#define CHCORE_EBADSYSCALL     13      /* Bad syscall number */
#define CHCORE_ENOMAPPING      14      /* Bad syscall number */
#define CHCORE_ENOENT          15      /* Entry does not exist */
#define CHCORE_EEXIST          16      /* Entry already exists */
#define CHCORE_ENOTEMPTY       17      /* Dir is not empty */
#define CHCORE_ENOTDIR         18      /* Does not refer to a directory */
