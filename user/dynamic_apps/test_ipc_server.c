#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <stdio.h>
#include <string.h>

void server_trampoline(ipc_msg_t *ipc_msg)
{
	/*
	 * This trampoline directly transfer the control
	 * back the client.
	 */
	ipc_return(ipc_msg, 0);
}

int main(int argc, char *argv[], char *envp[])
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
	printf("[IPC Server] no clients. Bye!\n");
	return 0;
}
