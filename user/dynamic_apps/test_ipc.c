#define _GNU_SOURCE

#include <chcore/syscall.h>
#include <stdio.h>
#include <chcore/type.h>
#include <chcore/defs.h>
#include <malloc.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <sched.h>
#include <chcore/pmu.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

#define IPC_NUM                  1000

#ifndef FBINFER

int server_cap;

volatile int finish_flag;

#define PLAT_CPU_NUM 4

void *perf_ipc_routine(void *arg)
{
	ipc_struct_t *client_ipc_struct;
	int i;

	cpu_set_t mask;
	int tid = (int) (long) arg;
	int ret;

	CPU_ZERO(&mask);
	CPU_SET(tid % PLAT_CPU_NUM, &mask);
	ret = sched_setaffinity(0, sizeof(mask), &mask);
	assert(ret == 0);
	sched_yield();

	/* register IPC client */
	for (i = 0; i < 20; ++i) {
		//printf("[client] thread %d ipc_register_client NO.%d\n",
		 //      tid, i);

		client_ipc_struct = ipc_register_client(server_cap);
		if (client_ipc_struct == NULL) {
			printf("ipc_register_client failed\n");
			usys_exit(-1);
		}

	}

	ipc_msg_t *ipc_msg;

	// TODO: cannot be removed ecause of the stupid impl @lwt
	ipc_msg = ipc_create_msg(client_ipc_struct, 0, 0);

	printf("[client] thread %d after register \n", tid);
	for(i = 0; i < IPC_NUM; i++)
		ipc_call(client_ipc_struct, ipc_msg);

	ipc_destroy_msg(ipc_msg);

	printf("[client] thread %d finished.\n", tid);

	__atomic_add_fetch(&finish_flag, 1, __ATOMIC_SEQ_CST);

	return NULL;
}

int main(int argc, char *argv[])
{
	cpu_set_t mask;
	char *args[2];
	struct new_process_caps new_process_caps;

	int i;
	int thread_num = 32;
	pthread_t thread_id[32];

        CPU_ZERO(&mask);
        CPU_SET(2, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) != 0)
                printf("sched_setaffinity failed!\n");
        sched_yield();

	args[0] = "test_ipc_server.bin";
	args[1] = "TOKEN";
	create_process(2, args, &new_process_caps);
	server_cap = new_process_caps.mt_cap;

	printf("Start testing concurrent IPC\n");

	finish_flag = 0;

	if (argc == 2)
		thread_num = atoi(argv[1]);

	for (i = 0; i < thread_num; ++i) {
		pthread_create(&thread_id[i], NULL, perf_ipc_routine,
			       (void *)(unsigned long)i);
	}

	printf("[client] create %d threads done\n", thread_num);

	while (finish_flag != thread_num) ;

	printf("[client] all %d threads done\n", thread_num);

	// TODO: how to shutdown the server process
	// server_exit_flag = 1;

	return 0;
}
#endif
