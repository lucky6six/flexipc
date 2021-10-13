#pragma once

#include <arch/sched/arch_sched.h>
#include <common/types.h>
#include <common/list.h>
#include <common/kprint.h>

// TODO: detect CPU num on boot time
#include <machine.h>

struct thread;

/* Timer ticks in system */
#if LOG_LEVEL == DEBUG
/* BUDGET represents the number of TICKs */
#define DEFAULT_BUDGET	1
#define TICK_MS		3000
#else
/* BUDGET represents the number of TICKs */
#define DEFAULT_BUDGET	1
#define TICK_MS		10
#endif

#define MAX_PRIO	255
#define MIN_PRIO	0
#define PRIO_NUM	(MAX_PRIO + 1)

#define NO_AFF		-1

/* Data structures */

#define	STATE_STR_LEN	20
enum thread_state {
	TS_INIT	= 0, /* For debug now: mark the thread as in creating */
	TS_READY, /* The thread is in the ready queue (runnable) */
	TS_INTER, /* For debug only: intermediate stat used by sched */
	TS_RUNNING, /* The thread is running now */
	TS_EXIT,
	TS_WAITING, /* waiting IPC or etc */
	TS_EXITING, /* waiting to be exited */
};

enum thread_special_state {
	KS_FREE = 0,
	KS_LOCKED
};

#define TYPE_STR_LEN	20
enum thread_type {
	/* 
	 * Kernel-level threads
	 * 1. Without FPU states
	 * 2. Won't change vmspace
	 * 3. Won't swap gs/fs
	 */
	TYPE_IDLE = 0,		/* IDLE thread dose not have stack, pause cpu */
	TYPE_KERNEL = 1, 	/* KERNEL thread has stack */

	/*
	 * User-level threads
	 * Should be larger than TYPE_KERNEL!
	 */
	TYPE_USER = 2,
	TYPE_SHADOW = 3,	/* SHADOW thread is used to achieve migrate IPC */
	TYPE_TESTS = 4		/* TESTS thread is used by kernel tests */
};

typedef struct sched_cont {
	u32 budget;
	char pad[pad_to_cache_line(sizeof(u32))];
} sched_cont_t;

/* Must be 8-bit aligned */
struct thread_ctx {
	/* Executing Context */
	arch_exec_cont_t ec;
	/* FPU States */
	void *fpu_state;
	/* TLS Related States */
	u64 tls_base_reg[TLS_REG_NUM];
	/* Scheduling Context */
	sched_cont_t *sc;
	/* Thread Type */
	u32 type;
	/* Thread state */
	u32 state;
	/* Priority */
	u32 prio;
	/* SMP Affinity */
	s32 affinity;
	/* Current Assigned CPU */
	u32 cpuid;
	/* Thread kernel stack state */
	volatile u32 kernel_stack_state;

	/* Previous running thread's thread_ctx */
	// struct thread_ctx *prev_ctx;

} __attribute__((aligned(CACHELINE_SZ)));


/* Debug functions */
void print_thread(struct thread *thread);

extern char thread_type[][TYPE_STR_LEN];
extern char thread_state[][STATE_STR_LEN];

void arch_idle_ctx_init(struct thread_ctx *idle_ctx, void (*func)(void));
void arch_switch_context(struct thread *target);
u64 switch_context(void);
int sched_is_runnable(struct thread *target);
int sched_is_running(struct thread *target);
int switch_to_thread(struct thread *target);

/* Helper function to find next runnable thread in the RQ */
struct thread *find_next_runnable_thread_in_list(struct list_head *thread_list);

/* Global-shared kernel data */
extern struct list_head ready_queue[PLAT_CPU_NUM][PRIO_NUM];
extern struct thread *current_threads[PLAT_CPU_NUM];

/* Indirect function call may downgrade performance */
struct sched_ops {
	int (*sched_init)(void);
	int (*sched)(void);
	int (*sched_enqueue)(struct thread * thread);
	int (*sched_dequeue)(struct thread * thread);
	/* Debug tools */
	void (*sched_top)(void);
};

/* Provided Scheduling Policies */
extern struct sched_ops pbrr;	/* Priority Based Round Robin */
extern struct sched_ops rr;	/* Simple Round Robin */

/* Chosen Scheduling Policies */
extern struct sched_ops *cur_sched_ops;

int sched_init(struct sched_ops *sched_ops);

static inline int sched(void)
{
	return cur_sched_ops->sched();
}

static inline int sched_enqueue(struct thread *thread)
{
	return cur_sched_ops->sched_enqueue(thread);
}

static inline int sched_dequeue(struct thread *thread)
{
	return cur_sched_ops->sched_dequeue(thread);
}
