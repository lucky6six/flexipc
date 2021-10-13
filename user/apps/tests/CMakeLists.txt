cmake_minimum_required(VERSION 3.11)

add_executable(test_sched.bin "test_sched.c")

add_executable(test_net_server.bin "test_net_server.c")

add_executable(test_net_client.bin "test_net_client.c")

add_executable(poll_client.bin "poll_client.c")

add_executable(poll_server.bin "poll_server.c")

add_executable(epoll_server.bin "poll_server.c")
target_compile_options(epoll_server.bin PRIVATE -DUSE_EPOLL)

add_executable(test_malloc.bin "test_malloc.c")

add_executable(test_fs.bin "test_fs.c")
target_compile_options(test_fs.bin PUBLIC -g3 -ggdb)

add_executable(test_fpu.bin "test_fpu.c")

add_executable(test_poll.bin "test_poll.c")

add_executable(test_notifc.bin "test_notifc.c")

add_executable(test_pthread.bin "test_pthread.c")

add_executable(test_argv.bin "test_argv.c")

add_executable(test_time.bin "test_time.c")

add_executable(test_eventfd.bin "test_eventfd.c")

add_executable(test_sleep.bin "test_sleep.c")

add_executable(fsstress.bin "fsstress/fsstress.c")
