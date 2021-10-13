cmake_minimum_required(VERSION 3.14)

set(DEVICE_PATH     "${BOOTLOADER_PATH}/plat/${CHCORE_PLAT}")
set(INIT_PATH       "${BOOTLOADER_PATH}/init")
set(LIB_PATH        "${BOOTLOADER_PATH}/lib")

file(
    GLOB
        tmpfiles
        "${DEVICE_PATH}/*.c"
        "${INIT_PATH}/*.c"
        "${INIT_PATH}/*.S"
        "${LIB_PATH}/*.c"
        "${LIB_PATH}/*.S"
)

list(APPEND files ${tmpfiles})
