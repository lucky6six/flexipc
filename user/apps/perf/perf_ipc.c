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
#include "perf.h"

#define IPC_NUM                  1000000

#ifndef FBINFER

void perf_ipc_routine(int server_cap)
{
	ipc_struct_t *client_ipc_struct;
	ipc_msg_t *ipc_msg;
	int i;
	u64 start, end;

	/* register IPC client */
	client_ipc_struct = ipc_register_client(server_cap);
	if (client_ipc_struct == NULL) {
		error("ipc_register_client failed\n");
		usys_exit(-1);
	}

	// TODO: cannot be removed ecause of the stupid impl of lwt.
	ipc_msg = ipc_create_msg(client_ipc_struct, 0, 0);

	/* warm up */
	for(i = 0; i < 1000; i++) {
		ipc_call(client_ipc_struct, ipc_msg);
	}

	register u64 ipc_conn_cap = client_ipc_struct->conn_cap;
#ifdef CONFIG_ARCH_X86_64
	register u64 ret;

	start = pmu_read_real_cycle();
	for(i = 0; i < IPC_NUM; i++) {
		asm volatile ("syscall"
			      : "=a"(ret)
			      : "a"(CHCORE_SYS_ipc_call),
			      "D"(ipc_conn_cap),
			      "S"(0)
			      : "rcx", "r11", "memory"
			     );
	}
	end = pmu_read_real_cycle();
#else
	start = pmu_read_real_cycle();
	for(i = 0; i < IPC_NUM; i++) {
		register long x8 __asm__("x8") = CHCORE_SYS_ipc_call;
		register long x0 __asm__("x0") = ipc_conn_cap;
		register long x1 __asm__("x1") = 0;
		register long x2 __asm__("x2") = 0;
		asm volatile ("svc 0"
			      : "=r"(x0)
			      : "r"(x8), "0"(x0), "r"(x1), "r"(x2)
			      : "cc", "memory"
			      );
	}
	end = pmu_read_real_cycle();
#endif
	info("IPC: %ld cycles\n", (end-start) / IPC_NUM);

	ipc_destroy_msg(ipc_msg);
}

int main(int argc, char *argv[], char* envp[])
{
	cpu_set_t mask;

	char *args[2];
	struct new_process_caps new_process_caps;
	int server_cap;

#ifdef CONFIG_ARCH_X86_64
	printf("Perf IPC on x86_64\n");
#else
	printf("Perf IPC on aarch64\n");
#endif

        CPU_ZERO(&mask);
        CPU_SET(2, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) != 0)
                error("sched_setaffinity failed!\n");
        sched_yield();

	args[0] = "/perf_ipc_server.bin";
	args[1] = "TOKEN";
	create_process(2, args, &new_process_caps);
	server_cap = new_process_caps.mt_cap;

	info("Start measure IPC\n");

	/* Perf test start */
	perf_ipc_routine(server_cap);

	// TODO: how to shutdown the server process
	// server_exit_flag = 1;

	return 0;
}
#endif
