#include <irq/ipi.h>

void arch_send_ipi(u32 cpu, u32 ipi)
{
	plat_send_ipi(cpu, ipi);
}

int handle_ipi(u32 ipi)
{
	switch (ipi) {
	case IPI_RESCHEDULE:
		/* Reschedule is done in handle_irq. */
		return 0;
	default:
		kwarn("Unknow IPI %d\n", ipi);
		return -1;
	}
}
