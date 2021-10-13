cmake_minimum_required (VERSION 3.11)

set(CMAKE_C_FLAGS "-O3 -Wall -Werror -fPIC")
set(LINK_FLAGS "-shared")

add_executable(hello_world.bin hello_world.c)

add_executable(test_ipc.bin test_ipc.c)
add_executable(test_ipc_server.bin test_ipc_server.c)

SET (LIBFOO_SRC foo.c)
ADD_LIBRARY (foo SHARED ${LIBFOO_SRC})

add_executable(hello_dl.bin hello_dl.c)
target_link_libraries(hello_dl.bin foo)

