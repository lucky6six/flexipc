#include <irq/irq.h>

void x2apic_send_ipi_single(u32, u32);

void arch_send_ipi(u32 cpu, u32 ipi)
{
	/*
	 * Use x2apic for sending IPI on x86.
	 * x2apic_send_ipi_single is used for interrupt a single core.
	 */
	x2apic_send_ipi_single(cpu, ipi);
}
