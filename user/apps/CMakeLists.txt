cmake_minimum_required(VERSION 3.11)

add_executable(client.bin "client.c")

add_executable(server.bin "server.c")

add_executable(hello.bin "hello.c")

add_executable(hello_libc.bin "hello_libc.c")

# A special one: the first user process created by the kernel
add_executable(init.bin "init.c")

add_executable(irq.bin "irq.c")

add_executable(pipe.bin "pipe.c")

add_executable(ttcp.bin "ttcp.c")

add_executable(multicast.bin "multicast.c")

add_executable(cat.bin "cat.c")

add_executable(diff.bin "diff.c")

add_executable(timerfd.bin "timerfd.c")

add_executable(timerfd_epoll.bin "timerfd_epoll.c")

add_executable(assert.bin "assert.c")
target_compile_options(assert.bin PUBLIC -g3 -ggdb)

add_executable(eventfd.bin "eventfd.c")

# In-system tests
add_subdirectory(tests)
# In-system perf
add_subdirectory(perf)

# Sqlite is not runnable now and costs much time for compiling.
# add_subdirectory(sqlite)
# include_directories("include")
