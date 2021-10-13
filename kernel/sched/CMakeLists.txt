cmake_minimum_required(VERSION 3.14)

add_library(${PROJECT_NAME}-sched OBJECT 
		sched.c
		context.c
		policy_pbrr.c
		policy_rr.c
)
