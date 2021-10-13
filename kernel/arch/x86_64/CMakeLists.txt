cmake_minimum_required(VERSION 3.14)

add_library(${PROJECT_NAME}-arch OBJECT
	main.c
	mm/page_table.c
	mm/memop.c
	mm/mmparse.c
	mm/uaccess.c
	mm/vmspace.c
	mm/tlb.c
	sched/context.c
	sched/extended_state.c
	sched/sched.c
	sched/idle.S
	machine/smp.c
	machine/uart.c
	machine/cpu.c
	machine/tsc.c
	irq/irq_entry.S
	irq/irq_entry.c
	irq/syscall.c
	irq/pgfault.c
	irq/x2apic.c
	irq/ipi.c
	sync/ticket.c
	sync/rwlock.c
)
