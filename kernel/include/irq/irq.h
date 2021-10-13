#pragma once

#include <common/types.h>
#include <object/thread.h>

/* TODO: Proper defination of MAX_IRQ_NUM. */
#define MAX_IRQ_NUM	4096
#define HANDLE_KERNEL	0
#define HANDLE_USER	1
extern u8 irq_handle_type[MAX_IRQ_NUM];

/* TODO: split the plat/arch declaration */
void eret_to_thread(u64 sp); /* in arch/xxx/irq/irq_entry.S */
void arch_interrupt_init(void); /* in arch/xxx/irq/irq_entry.c */
void arch_interrupt_init_per_cpu(void); /* in arch/xxx/irq/irq_entry.c */
void arch_enable_irqno(int irqno);

/* XXX: only needed in ARM */
void plat_handle_irq(void);     /* in arch/xxx/plat/xxx/irq/irq.c */
void plat_interrupt_init(void); /* in arch/xxx/plat/xxx/irq/irq.c */
void plat_disable_timer(void);
void plat_enable_timer(void);
void plat_enable_irqno(int irq);

void plat_ack_irq(int irq);

/* do not declare here */
void arch_send_ipi(u32 cpu, u32 ipi);
void plat_send_ipi(u32 cpu, u32 ipi);
