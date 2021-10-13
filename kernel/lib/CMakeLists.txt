cmake_minimum_required(VERSION 3.14)

add_library(${PROJECT_NAME}-lib OBJECT printk.c elf.c cpio/cpio.c radix.c)
