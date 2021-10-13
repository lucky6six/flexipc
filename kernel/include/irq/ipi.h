#pragma once

#include <irq/irq.h>
#include <common/types.h>
#include <common/lock.h>

/*
 * See irq_entry.h for x86_64.
 * TODO: does aarch64 allow to define ipi_vector as follows?
 */
#define IPI_TLB_SHOOTDOWN 60
#define IPI_RESCHEDULE    61

/* TODO: should separate plat/arch */
void ipi_reschedule(u32 cpu);
int handle_ipi(u32 ipi);


/* 7 u64 arg and 2 u32 (start/finish) occupy one cacheline */
#define IPI_DATA_ARG_NUM (7)

struct ipi_data {
	/* Grab this lock before writing to this ipi_data */
	struct lock lock;

	/* start  <- 1: the ipi_data (argments) is ready */
	volatile u32 start;
	/* finish <- 1: the ipi_data (argments) is handled */
	volatile u32 finish;

	u64 args[IPI_DATA_ARG_NUM];
};

void init_ipi_data(void);

/* IPI interfaces for achieving cross-core communication */

/* Sender side */
void prepare_ipi_tx(u32 target_cpu);
void set_ipi_tx_arg(u32 target_cpu, u32 arg_index, u64 val);
void start_ipi_tx(u32 target_cpu, u32 ipi_vector);
void wait_finish_ipi_tx(u32 target_cpu);

/* Receiver side */
u64 get_ipi_tx_arg(u32 target_cpu, u32 arg_index);
void mark_finish_ipi_tx(u32 target_cpu);

