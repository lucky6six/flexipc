#include <irq/ipi.h>
#include <machine.h>
#include <common/kprint.h>

/* Global IPI_data shared among all the CPUs */
struct ipi_data global_ipi_data[PLAT_CPU_NUM];

/* Invoked once during the kernel boot */
void init_ipi_data(void)
{
	int i;
	struct ipi_data *data;

	for (i = 0; i < PLAT_CPU_NUM; ++i) {
		data = &(global_ipi_data[i]);
		lock_init(&(data->lock));
		data->start = 0;
		data->finish = 0;
	}
}

void ipi_reschedule(u32 cpu)
{
	arch_send_ipi(cpu, IPI_RESCHEDULE);
}

/*
 * Interfaces for inter-cpu communication (named IPI_transaction).
 * IPI-based message sending.
 */

/* Lock the target buffer */
void prepare_ipi_tx(u32 target_cpu)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	lock(&(data->lock));
}

/* Set argments */
void set_ipi_tx_arg(u32 target_cpu, u32 arg_index, u64 val)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	data->args[arg_index] = val;
}

/*
 * Start IPI-based transaction (tx).
 *
 * ipi_vector can be encoded into the physical interrupt (as IRQ number),
 * which can be used to implement some fast-path (simple) communication.
 *
 * Nevertheless, we support sending information with IPI.
 * So, actually, we can use one ipi_vector to distinguish different IPIs.
 */
void start_ipi_tx(u32 target_cpu, u32 ipi_vector)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	/* Mark the arguments are ready (set_ipi_tx_arg before) */
	data->start = 1;

	/* Send physical IPI to interrupt the target CPU */
	arch_send_ipi(target_cpu, ipi_vector);
}

/* Wait and unlock */
void wait_finish_ipi_tx(u32 target_cpu)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	/* Wait untill finish */
	while (data->finish != 1) {}

	/* Reset start/finish */
	data->start = 0;
	data->finish = 0;

	data = &(global_ipi_data[target_cpu]);
	unlock(&(data->lock));
}

/* Unlock only */
void finish_ipi_tx(u32 target_cpu)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	unlock(&(data->lock));
}


/*
 * Receiver side interfaces.
 * Note that target_cpu is the receiver itself.
 */

/* Get argments */
u64 get_ipi_tx_arg(u32 target_cpu, u32 arg_index)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	return data->args[arg_index];
}

/* Mark the receiver (i.e., target_cpu) has handled the tx */
void mark_finish_ipi_tx(u32 target_cpu)
{
	struct ipi_data *data;

	data = &(global_ipi_data[target_cpu]);
	data->finish = 1;
}

