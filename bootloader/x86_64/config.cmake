cmake_minimum_required(VERSION 3.14)

set(INIT_PATH       "${BOOTLOADER_PATH}/init")

file(
    GLOB
        tmpfiles
        "${INIT_PATH}/header.S"
        "${INIT_PATH}/gdt.c"
)

list(APPEND files ${tmpfiles})
