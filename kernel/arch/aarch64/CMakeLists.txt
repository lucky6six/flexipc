cmake_minimum_required(VERSION 3.14)

add_library(${PROJECT_NAME}-arch OBJECT
    head.S
    main.c
    tools.S
    mm/page_table.c
    mm/page_table.S
    mm/memcpy.S
    mm/memmove.S
    # remove memset.S since it requires FPU
    #mm/memset.S
    mm/memset.c
    mm/bzero.S
    mm/cache.c
    mm/mmparse.c
    mm/uaccess.c
    mm/vmspace.c
    machine/smp.c
    machine/pmu.c
    irq/irq_entry.c
    irq/irq_entry.S
    irq/irq.S
    irq/pgfault.c
    irq/ipi.c
    sched/sched.c
    sched/context.c
    sched/fpu.c
    sched/idle.S
    sync/ticket.c
    sync/rwlock.c
)
