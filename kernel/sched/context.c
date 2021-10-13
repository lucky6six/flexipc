#include <object/thread.h>
#include <sched/sched.h>
#include <mm/kmalloc.h>
#include <common/util.h>
#include <common/kprint.h>

/* Extern functions */
void arch_init_thread_fpu(struct thread_ctx *ctx);
void arch_free_thread_fpu(struct thread_ctx *ctx);

/*
 * The kernel stack for a thread looks like:
 *
 * ---------- thread_ctx end: kernel_stack_base + DEFAULT_KERNEL_STACK_SZ (0x1000)
 *    ...
 * other fields in the thread_ctx:
 * e.g., fpu_state, tls_state, sched_ctx
 *    ...
 * execution context (e.g., registers)
 * ---------- thread_ctx
 *    ...
 *    ...
 * stack in use for when the thread enters kernel
 *    ...
 *    ...
 * ---------- kernel_stack_base
 *
 */
struct thread_ctx *create_thread_ctx(void)
{
	void *kernel_stack;
	struct thread_ctx *ctx;

	kernel_stack = kzalloc(DEFAULT_KERNEL_STACK_SZ);
	if (kernel_stack == NULL) {
		kwarn("create_thread_ctx fails due to lack of memory\n");
		return NULL;
	}
	ctx = (struct thread_ctx *)(kernel_stack + DEFAULT_KERNEL_STACK_SZ
				    - sizeof(struct thread_ctx));
	arch_init_thread_fpu(ctx);
	return ctx;
}

void destroy_thread_ctx(struct thread *thread)
{
	void *kernel_stack;

	arch_free_thread_fpu(thread->thread_ctx);

	kfree(thread->thread_ctx->sc);
	BUG_ON(!thread->thread_ctx);
	kernel_stack = (void *)thread->thread_ctx - DEFAULT_KERNEL_STACK_SZ
			+ sizeof(struct thread_ctx);
	kfree(kernel_stack);
}
