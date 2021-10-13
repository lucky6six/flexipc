#include "uart.h"

void uart_send_string(char *str)
{
	for (int i = 0; str[i] != '\0'; i++) 
		uart_putc((char)str[i]);
}

void uart_putc(char ch)
{
	/* Wait until there is space in the FIFO or device is disabled */
	while (early_get32((unsigned long)PL011_UART6_BASE + UART_FR)
	       & UART_FR_TXFF) {
	}
	/* Send the character */
	early_put32((unsigned long)PL011_UART6_BASE + UART_DR, (unsigned int)ch);
}

void early_uart_init(void)
{
	unsigned long base = PL011_UART6_BASE;

	/* Clear all errors */
	early_put32(base + UART_RSR_ECR, 0);
	/* Disable everything */
	early_put32(base + UART_CR, 0);
	if (CONSOLE_BAUDRATE) {
		unsigned int divisor =
		    (CONSOLE_UART_CLK_IN_HZ * 4) / CONSOLE_BAUDRATE;
		early_put32(base + UART_IBRD, divisor >> 6);
		early_put32(base + UART_FBRD, divisor & 0x3f);
	}
	/* Configure TX to 8 bits, 1 stop bit, no parity, fifo disabled. */
	early_put32(base + UART_LCR_H, UART_LCRH_WLEN_8);

	/* Enable interrupts for receive and receive timeout */
	early_put32(base + UART_IMSC, UART_IMSC_RXIM | UART_IMSC_RTIM);

	/* Enable UART and RX/TX */
	early_put32(base + UART_CR, UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
}
