#include <ipc/notification.h>
#include <irq/irq.h>
#include <object/irq.h>
#include <common/errno.h>

struct irq_notification *irq_notifcs[MAX_IRQ_NUM];
int user_handle_irq(int irq)
{
	BUG_ON(!irq_notifcs[irq]);
	signal_notific(&irq_notifcs[irq]->notifc, 1);
	return 0;
}

void irq_deinit(void *irq_ptr)
{
	struct irq_notification *irq_notifc = irq_ptr;
	int irq;

	irq = irq_notifc->intr_vector;
	irq_handle_type[irq] = HANDLE_KERNEL;
	smp_mb();
	irq_notifcs[irq] = NULL;
}

int sys_irq_register(int irq)
{
	struct irq_notification *irq_notifc = NULL;
	int irq_notifc_cap = 0;
	int ret = 0;

	if (irq < 0 || irq >= MAX_IRQ_NUM)
		return -EINVAL;

	irq_notifc = obj_alloc(TYPE_IRQ, sizeof(*irq_notifc));
	if (!irq_notifc) {
		ret = -ENOMEM;
		goto out_fail;
	}
	irq_notifc->intr_vector = irq;
	init_notific(&irq_notifc->notifc);

	irq_notifc_cap = cap_alloc(current_cap_group, irq_notifc, 0);
	if (irq_notifc_cap < 0) {
		ret = irq_notifc_cap;
		goto out_free_obj;
	}

	irq_notifcs[irq] = irq_notifc;
	smp_mb();
	irq_handle_type[irq] = HANDLE_USER;

	arch_enable_irqno(irq);

	return irq_notifc_cap;
out_free_obj:
	obj_free(irq_notifc);
out_fail:
	return ret;
}

int sys_irq_wait(int irq_cap, bool is_block)
{
	struct irq_notification *irq_notifc = NULL;
	int ret;
	irq_notifc = obj_get(current_thread->cap_group, irq_cap,
			     TYPE_IRQ);
	if (!irq_notifc) {
		ret = -ECAPBILITY;
		goto out;
	}
	ret = wait_notific(&irq_notifc->notifc, is_block, NULL);
	obj_put(irq_notifc);
out:
	return ret;
}

int sys_irq_ack(int irq_cap)
{
	struct irq_notification *irq_notifc = NULL;
	int ret = 0;
	irq_notifc = obj_get(current_thread->cap_group, irq_cap,
			     TYPE_IRQ);
	if (!irq_notifc) {
		ret = -ECAPBILITY;
		goto out;
	}
	plat_ack_irq(irq_notifc->intr_vector);
	obj_put(irq_notifc);
out:
	return ret;
}
