#pragma once

#include <ipc/notification.h>
#include <object/object.h>

struct irq_notification {
	u32 intr_vector;
	u32 status;
	struct notification notifc;
};

int user_handle_irq(int irq);
void irq_deinit(void *irq_ptr);
