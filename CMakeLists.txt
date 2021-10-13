cmake_minimum_required (VERSION 3.14)

set(CMAKE_VERBOSE_MAKEFILE on)

include("${CMAKE_SOURCE_DIR}/config.cmake")

if (CHCORE_ARCH STREQUAL "aarch64")
	set(CROSS_COMPILE "${CHCORE_ARCH}-linux-gnu-")
else ()
	set(CROSS_COMPILE "")
endif ()

set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_ASM_COMPILER "${CROSS_COMPILE}gcc")
execute_process (
	COMMAND  ${CMAKE_C_COMPILER} -print-file-name=include
	OUTPUT_STRIP_TRAILING_WHITESPACE
	OUTPUT_VARIABLE C_COMPILER_INCLUDE
)

#set(C_COMPILER_NOSTDINC "-nostdinc")

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

# Disable FPU in kernel on both x86_64 and aarch64
# ChCore does not use FPU
if (CHCORE_ARCH STREQUAL "aarch64")
	if (FBINFER)
		set(DISABLE_FPU "")
	else ()
		set(DISABLE_FPU "-march=armv8-a+nofp")
	endif ()

	set(ARCH_SPECIFIC "")
else ()
	set(DISABLE_FPU "-mno-sse")
	set(ARCH_SPECIFIC "-DARCH_X86_64 -mfsgsbase")
endif ()

set(CMAKE_C_FLAGS
    "-Wall -fPIC -nostdlib -nostartfiles -ffreestanding -Werror \
    -DCHCORE -nostdinc ${DISABLE_FPU} ${ARCH_SPECIFIC}")

project (chos C ASM)

set(BOOTLOADER_PATH     "bootloader/${CHCORE_ARCH}")
#message(STATUS "#### BOOTLOADER_PATH: ${BOOTLOADER_PATH}")

set(KERNEL_PATH         "${CMAKE_CURRENT_SOURCE_DIR}/kernel")
set(USER_PATH           "${CMAKE_CURRENT_SOURCE_DIR}/user")

set(BOOTLOADER_DEVICE_PATH     "${BOOTLOADER_PATH}/plat/${CHCORE_PLAT}")
set(BOOTLOADER_INIT_PATH       "${BOOTLOADER_PATH}/init")
set(BOOTLOADER_LIB_PATH        "${BOOTLOADER_PATH}/lib")

set(KERNEL_ARCH_PATH        "${KERNEL_PATH}/arch/${CHCORE_ARCH}")
# NOTE(MK): We have moved the drivers to the user space. Don't know whether we
# need to move some of them back in the future.
#set(KERNEL_DRIVER_PATH      "${KERNEL_PATH}/driver")
set(KERNEL_TESTS_PATH         "${KERNEL_PATH}/tests")
set(KERNEL_IPC_PATH         "${KERNEL_PATH}/ipc")
set(KERNEL_LIB_PATH         "${KERNEL_PATH}/lib")
set(KERNEL_SYSCALL_PATH     "${KERNEL_PATH}/syscall")
set(KERNEL_IRQ_PATH         "${KERNEL_PATH}/irq")
set(KERNEL_MM_PATH          "${KERNEL_PATH}/mm")
set(KERNEL_OBJECT_PATH      "${KERNEL_PATH}/object")
set(KERNEL_PLAT_PATH        "${KERNEL_PATH}/arch/${CHCORE_ARCH}/plat/${CHCORE_PLAT}")
set(KERNEL_SCHED_PATH       "${KERNEL_PATH}/sched")
set(KERNEL_INCLUDE_PATH     "${KERNEL_PATH}/include")

include_directories("${KERNEL_INCLUDE_PATH}")
include_directories("${KERNEL_INCLUDE_PATH}/arch/${CHCORE_ARCH}/")
if (CHCORE_ARCH STREQUAL "aarch64")
	include_directories("${KERNEL_INCLUDE_PATH}/arch/${CHCORE_ARCH}/plat/${CHCORE_PLAT}/")
else ()
endif ()

set(files "")
include("${BOOTLOADER_PATH}/config.cmake")

add_subdirectory("${KERNEL_ARCH_PATH}")
add_subdirectory("${KERNEL_TESTS_PATH}")
add_subdirectory("${KERNEL_IPC_PATH}")
add_subdirectory("${KERNEL_LIB_PATH}")
add_subdirectory("${KERNEL_MM_PATH}")
add_subdirectory("${KERNEL_OBJECT_PATH}")
add_subdirectory("${KERNEL_PLAT_PATH}")
add_subdirectory("${KERNEL_SYSCALL_PATH}")
add_subdirectory("${KERNEL_IRQ_PATH}")
add_subdirectory("${KERNEL_SCHED_PATH}")

set(BINARY_KERNEL_IMG_PATH "CMakeFiles/kernel.img.dir")

#message(STATUS "the memory layout of bootloader is defined accroding to the order of files")
if (CHCORE_ARCH STREQUAL "aarch64")
    set(init_object
        "${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/header.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/efi_entry.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/efi_entry.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/start.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/mmu.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/irq.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/irq.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/tools.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/init_c.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/macro.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_DEVICE_PATH}/uart.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_DEVICE_PATH}/smp.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_LIB_PATH}/printf.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_LIB_PATH}/efi.c.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_LIB_PATH}/sort.c.o"
        )
else ()
    set(init_object
        "${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/header.S.o
         ${BINARY_KERNEL_IMG_PATH}/${BOOTLOADER_INIT_PATH}/gdt.c.o"
        )
endif ()

set(link_script "linker.lds")
if (CHCORE_ARCH STREQUAL "aarch64")
	configure_file("./scripts/linker-aarch64.lds.in" "linker.lds.S")
else ()
	configure_file("./scripts/linker-x86_64.lds.in" "linker.lds.S")
endif ()

#message(STATUS "${files}")

add_executable(kernel.img ${files} ${USER_PATH}/binary_include.S
    $<TARGET_OBJECTS:${PROJECT_NAME}-arch>
    #$<TARGET_OBJECTS:${PROJECT_NAME}-driver>
    $<TARGET_OBJECTS:${PROJECT_NAME}-ipc>
    $<TARGET_OBJECTS:${PROJECT_NAME}-lib>
    $<TARGET_OBJECTS:${PROJECT_NAME}-mm>
    $<TARGET_OBJECTS:${PROJECT_NAME}-object>
    $<TARGET_OBJECTS:${PROJECT_NAME}-plat>
    $<TARGET_OBJECTS:${PROJECT_NAME}-syscall>
    $<TARGET_OBJECTS:${PROJECT_NAME}-sched>
    $<TARGET_OBJECTS:${PROJECT_NAME}-tests>
    $<TARGET_OBJECTS:${PROJECT_NAME}-irq>
)

target_include_directories(
    kernel.img
    PRIVATE
        "${BOOTLOADER_PATH}/include"
        "${BOOTLOADER_DEVICE_PATH}/include"
        "${BOOTLOADER_INIT_PATH}/include"
        "${BOOTLOADER_LIB_PATH}/include"
)
if (CHCORE_ARCH STREQUAL "aarch64")
set_property(
    TARGET kernel.img
    APPEND_STRING
    PROPERTY
        LINK_FLAGS
        "-T ${CMAKE_CURRENT_BINARY_DIR}/${link_script} -e _start"
)
else ()
set_property(
    TARGET kernel.img
    APPEND_STRING
    PROPERTY
        LINK_FLAGS
        "-T ${CMAKE_CURRENT_BINARY_DIR}/${link_script} -e start -m elf_x86_64 -z max-page-size=4096"
)
endif ()

set_property(
    TARGET kernel.img
    APPEND_STRING
    PROPERTY
        DEPENDS
        "${link_script}"
)

add_custom_command(
    TARGET kernel.img
    PRE_LINK
	COMMAND ${CMAKE_C_COMPILER} -E -x c
	-I${CMAKE_SOURCE_DIR}/${BOOTLOADER_PATH}/include ./linker.lds.S | grep -v "^#" > linker.lds
)
