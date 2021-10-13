# Hikey970 support

## Flash partitions

| Partition name | Device        | Size         | Contents/Notes                                               |
| -------------- | ------------- | ------------ | ------------------------------------------------------------ |
| splash1        |               | 307200 bytes | splash screen image.                                         |
| misc           | /dev/mtd/mtd0 | 262k         | misc - has apparently some flags used for controlling device mode |
| recovery       | /dev/mtd/mtd1 | 5.2M         | kernel, initrd with rootfs (for alternate boot)              |
| boot           | /dev/mtd/mtd2 | 2.6M         | kernel, initrd with rootfs (for default boot)                |
| system         | /dev/mtd/mtd3 | 70M          | yaffs2 file system, mounted read-only at /system - has the bulk of the Android system, including system libraries, Dalvik and pre-installed applications. |
| cache          | /dev/mtd/mtd4 | 70M          | yaffs2 file system, mounted at /cache - only used on G1 for over-the-air updates. This partition can be used to store temporary data. |
| userdata       | /dev/mtd/mtd5 | 78M          | yaffs2 file system, mounted at /data - contains user-installed applications and data, including customization data |

Boot刷的是grubaa64的efi，可以从源码编译获得。

System里刷的是rootfs，包含grub.cfg和启动的kernel image。

96boards提供了[OpenPlatformPkg](https://github.com/96boards-hikey/OpenPlatformPkg/tree/hikey970_v1.0)用于在edk2中提供hikey970相关的信息。