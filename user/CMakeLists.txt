cmake_minimum_required(VERSION 3.14)

# set(CMAKE_VERBOSE_MAKEFILE on)

project(user C ASM)

set(CROSS_COMPILE "${CMAKE_SOURCE_DIR}/musl-1.1.24/build/bin/musl-")

set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_ASM_COMPILER "${CROSS_COMPILE}gcc")


# For aarch64, gcc options may have effect on performance.
# More details can refer to the following link:
# https://gcc.gnu.org/onlinedocs/gcc-6.1.0/gcc/AArch64-Options.html
# Currently, we define marh to armv8-a (considering both raspberry pi 3
# and hikey970).
if (CHCORE_MARCH STREQUAL "armv8-a")
	# Uncomment the following line to disable floating point usage.
	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+nofp")
endif ()

add_subdirectory(dynamic_apps)

set(LINK_FLAGS "-static")
add_subdirectory(tmpfs)
add_subdirectory(fsm)
add_subdirectory(procmgr)
add_subdirectory(drivers)
add_subdirectory(lwip)
add_subdirectory(apps)
