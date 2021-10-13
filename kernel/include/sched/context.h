#pragma once

#include <sched/sched.h>

struct thread_ctx *create_thread_ctx(void);
void destroy_thread_ctx(struct thread *thread);
void init_thread_ctx(struct thread *thread, u64 stack, u64 func, u32 prio, u32 type, s32 aff);

void arch_set_thread_stack(struct thread *thread, u64 stack);
void arch_set_thread_return(struct thread *thread, u64 ret);
u64  arch_get_thread_stack(struct thread *thread);
void arch_set_thread_next_ip(struct thread *thread, u64 ip);
u64  arch_get_thread_next_ip(struct thread *thread);
void arch_set_thread_info_page(struct thread *thread, u64 info_page_addr);
void arch_set_thread_arg(struct thread *thread, u64 arg);
void arch_set_thread_tls(struct thread *thread, u64 tls);
void arch_set_thread_badge_arg(struct thread *thread, u64 badge);

void arch_enable_interrupt(struct thread *thread);
void arch_disable_interrupt(struct thread *thread);

void switch_tls_info(struct thread *from, struct thread *to);
