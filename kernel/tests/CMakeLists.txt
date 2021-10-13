cmake_minimum_required(VERSION 3.14)

add_library(${PROJECT_NAME}-tests OBJECT 
		tests.c
		tst_malloc.c
		tst_mutex.c
		tst_rwlock.c
		tst_sched.c
)
