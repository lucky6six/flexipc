#pragma once

#include <object/thread.h>
#include <sched/sched.h>

#define EAGER_FPU_MODE 0
#define LAZY_FPU_MODE  1

// #define FPU_SAVING_MODE EAGER_FPU_MODE
#define FPU_SAVING_MODE LAZY_FPU_MODE

void arch_init_thread_fpu(struct thread_ctx *ctx);

void save_fpu_state(struct thread *thread);
void restore_fpu_state(struct thread *thread);

#if FPU_SAVING_MODE == LAZY_FPU_MODE
void disable_fpu_usage(void);
void enable_fpu_usage(void);

/* Used when hanlding FPU traps */
void change_fpu_owner(void);
/* Used when migrating a thread from local CPU to other CPUs */
void save_and_release_fpu(struct thread *thread);
#endif
