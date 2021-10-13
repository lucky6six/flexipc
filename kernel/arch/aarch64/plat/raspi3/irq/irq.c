#include <irq/irq.h>
#include <common/kprint.h>
#include <irq/timer.h>
#include <io/uart.h>
#include <machine.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/bitops.h>
#include <arch/machine/smp.h>
#include <arch/tools.h>
#include <irq/ipi.h>

/* Per core IRQ SOURCE MMIO address */
u64 core_irq_source[PLAT_CPU_NUM] = {
	CORE0_IRQ_SOURCE,
	CORE1_IRQ_SOURCE,
	CORE2_IRQ_SOURCE,
	CORE3_IRQ_SOURCE
};

static void bcm2836_ipi_init()
{
	u32 cpuid;
	cpuid = smp_get_cpu_id();
	/* enable the 1st mailbox as ipi irq */
	put32(CORE_MBOX_CTL(cpuid), BIT(0));
}

void plat_enable_irqno(int irq)
{
	/* TODO */
	BUG("not impl.");
}

void plat_disable_irqno(int irq)
{
	/* TODO */
	BUG("not impl.");
}

void plat_interrupt_init(void)
{
	bcm2836_ipi_init();
}

void plat_send_ipi(u32 cpu, u32 ipi)
{
	BUG_ON(cpu >= PLAT_CPU_NUM);
	BUG_ON(ipi >= 32);
	put32(CORE_MBOX_SET(cpu, 0), 1 << ipi);
}

void plat_handle_ipi()
{
	u32 cpuid, mbox, ipi;

	cpuid = smp_get_cpu_id();
	mbox = get32(CORE_MBOX_RDCLR(cpuid, 0));
	ipi = ctzl(mbox);
	put32(CORE_MBOX_RDCLR(cpuid, 0), 1 << ipi);
	handle_ipi(ipi);
}

/* TODO: irq notification in raspi3 */
void plat_ack_irq(int irq)
{
	/* empty */
}

void plat_handle_irq(void)
{
	u32 cpuid = 0;
	unsigned int irq_src, irq;

	cpuid = smp_get_cpu_id();
	irq_src = get32(core_irq_source[cpuid]);

	/* Handle one irq one time. Handle ipi first */
	if (irq_src & INT_SRC_MBOX0) {
		plat_handle_ipi();
		return;
	}

	irq = 1 << ctzl(irq_src);
	switch (irq) {
	case INT_SRC_TIMER3:
		handle_timer_irq();
		break;
	default:
		kinfo("Unsupported IRQ %d\n", irq);
	}
	return;
}
