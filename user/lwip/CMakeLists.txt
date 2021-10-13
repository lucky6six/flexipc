cmake_minimum_required(VERSION 3.7)

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(src/Filelists.cmake)

set(LWIP_CFLAGS "-O3 -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LWIP_CFLAGS}")
string(REPLACE "-Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

file(
    GLOB
        lwip_source_files
        "main.c"
        "${LWIP_DIR}/ports/chos/sys_arch.c"
        "${LWIP_DIR}/ports/chos/debug_flags.c"
)

add_executable(lwip.srv
        ${lwip_source_files}
        ${lwipnoapps_SRCS}
)

file(
    GLOB
        tcp_source_files
        "tcpip_example.c"
        "${LWIP_DIR}/ports/chos/sys_arch.c"
        "${LWIP_DIR}/ports/chos/debug_flags.c"
)

add_executable(tcp_example.bin
        ${tcp_source_files}
        ${lwipnoapps_SRCS}
)

include_directories(
        "${LWIP_DIR}/ports/chos/include"
        "${LWIP_DIR}/src/include"
        "${LWIP_DIR}/src/include/lwip"
)
