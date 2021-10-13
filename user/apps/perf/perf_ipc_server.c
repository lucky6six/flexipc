#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <stdio.h>
#include <string.h>
#include "perf.h"

void server_trampoline(ipc_msg_t *ipc_msg)
{
	/*
	 * This trampoline directly transfer the control
	 * back the client.
	 */
#ifdef CONFIG_ARCH_X86_64
	asm volatile ("syscall"
		      :
		      : "a"(CHCORE_SYS_ipc_return)
		      :
		     );
#else
	register long x8 __asm__("x8") = CHCORE_SYS_ipc_return;
	register long x0 __asm__("x0");
	asm volatile ("svc 0"
		      : "=r"(x0)
		      : "r"(x8), "0"(x0)
		      : "cc", "memory"
		      );
#endif
}

int main(int argc, char *argv[])
{
	void *token;

	if (argc == 1)
		goto out;

	token = strstr(argv[1], "TOKEN");
	if (!token)
		goto out;

	ipc_register_server(server_trampoline,
			    DEFAULT_CLIENT_REGISTER_HANDLER);

	// TODO: process quit
	while (1) {
		usys_yield();
	}
	return 0;
out:
	error("[IPC Server] no clients. Bye!\n");
	return 0;
}
