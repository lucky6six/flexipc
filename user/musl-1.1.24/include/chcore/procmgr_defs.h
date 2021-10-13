#pragma once

#include <sys/types.h>

enum PROC_REQ {
	PROC_REQ_FORK = 1,
	PROC_REQ_CLONE,
	PROC_REQ_SPAWN,
	PROC_REQ_NEWPROC,
	PROC_REQ_WAIT,
	PROC_REQ_GET_MT_CAP,
	PROC_REQ_INIT,
	PROC_REQ_MAX
};

#define PROC_REQ_NAME_LEN (255)
#define PROC_REQ_TEXT_SIZE (256)
#define PROC_REQ_ARGC_MAX 255
struct proc_request {
	enum PROC_REQ req;
	union {
		int argc; // Used by IPC client when creating new process
		pid_t pid; // Used by IPC client when wait on a process
		int exitstatus; // As the reply of process wait
	};
	// The following three are used by IPC client when creating new process.
	char name[PROC_REQ_NAME_LEN];
	off_t argv[PROC_REQ_ARGC_MAX];
	char text[PROC_REQ_TEXT_SIZE];
};
