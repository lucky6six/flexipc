#include "printf.h"
#include "irq.h"

const char *entry_error_messages[] =
{
	"SYNC_EL1t",
	"IRQ_EL1t",
	"FIQ_EL1t",
	"ERROR_EL1T",

	"SYNC_EL1h",
	"IRQ_EL1h",
	"FIQ_EL1h",
	"ERROR_EL1h",
};

void show_entry_message(int type, unsigned long esr, unsigned long address)
{
	printf("interrupt type: %s, ESR: 0x%lx, fault address: 0x%lx\r\n",
	       entry_error_messages[type], esr, address);
}
