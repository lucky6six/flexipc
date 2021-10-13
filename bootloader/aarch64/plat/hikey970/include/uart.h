#pragma once

#define CONSOLE_BAUDRATE        115200
#define CONSOLE_UART_CLK_IN_HZ  19200000
#define UARTDR      0x000
#define UARTFR      0x018
#define UARTFR_TXFF (1 << 5)
#define PL011_UART0_BASE    0xF8015000
#define PL011_UART1_BASE    0xF7111000
#define PL011_UART2_BASE    0xF7112000
#define PL011_UART3_BASE    0xF7113000
#define PL011_UART4_BASE    0xF7114000
#define PL011_UART6_BASE    0xFFF32000
#define PL011_REG_SIZE	0x1000

#define UART_PPTR           PL011_UART6_BASE
#define UART_REG(x) ((unsigned int *)(UART_PPTR + (x)))
#define UART_DR		0x00	/* data register */
#define UART_RSR_ECR	0x04	/* receive status or error clear */
#define UART_DMAWM	0x08	/* DMA watermark configure */
#define UART_TIMEOUT	0x0C	/* Timeout period */
/* reserved space */
#define UART_FR		0x18	/* flag register */
#define UART_ILPR	0x20	/* IrDA low-poer */
#define UART_IBRD	0x24	/* integer baud register */
#define UART_FBRD	0x28	/* fractional baud register */
#define UART_LCR_H	0x2C	/* line control register */
#define UART_CR		0x30	/* control register */
#define UART_IFLS	0x34	/* interrupt FIFO level select */
#define UART_IMSC	0x38	/* interrupt mask set/clear */
#define UART_RIS	0x3C	/* raw interrupt register */
#define UART_MIS	0x40	/* masked interrupt register */
#define UART_ICR	0x44	/* interrupt clear register */
#define UART_DMACR	0x48	/* DMA control register */

/* flag register bits */
#define UART_FR_RTXDIS	(1 << 13)
#define UART_FR_TERI	(1 << 12)
#define UART_FR_DDCD	(1 << 11)
#define UART_FR_DDSR	(1 << 10)
#define UART_FR_DCTS	(1 << 9)
#define UART_FR_RI	(1 << 8)
#define UART_FR_TXFE	(1 << 7)
#define UART_FR_RXFF	(1 << 6)
#define UART_FR_TXFF	(1 << 5)
#define UART_FR_RXFE	(1 << 4)
#define UART_FR_BUSY	(1 << 3)
#define UART_FR_DCD	(1 << 2)
#define UART_FR_DSR	(1 << 1)
#define UART_FR_CTS	(1 << 0)

/* transmit/receive line register bits */
#define UART_LCRH_SPS		(1 << 7)
#define UART_LCRH_WLEN_8	(3 << 5)
#define UART_LCRH_WLEN_7	(2 << 5)
#define UART_LCRH_WLEN_6	(1 << 5)
#define UART_LCRH_WLEN_5	(0 << 5)
#define UART_LCRH_FEN		(1 << 4)
#define UART_LCRH_STP2		(1 << 3)
#define UART_LCRH_EPS		(1 << 2)
#define UART_LCRH_PEN		(1 << 1)
#define UART_LCRH_BRK		(1 << 0)

/* control register bits */
#define UART_CR_CTSEN		(1 << 15)
#define UART_CR_RTSEN		(1 << 14)
#define UART_CR_OUT2		(1 << 13)
#define UART_CR_OUT1		(1 << 12)
#define UART_CR_RTS		(1 << 11)
#define UART_CR_DTR		(1 << 10)
#define UART_CR_RXE		(1 << 9)
#define UART_CR_TXE		(1 << 8)
#define UART_CR_LPE		(1 << 7)
#define UART_CR_OVSFACT		(1 << 3)
#define UART_CR_UARTEN		(1 << 0)

#define UART_IMSC_RTIM		(1 << 6)
#define UART_IMSC_RXIM		(1 << 4)

struct io_pa_va {
	unsigned long pa;
	unsigned long va;
};

struct serial_chip {
	const struct serial_ops *ops;
};

struct pl011_data {
	struct io_pa_va base;
	struct serial_chip chip;
};

extern void early_put32(unsigned long int addr, unsigned int ch);
extern unsigned int early_get32(unsigned long int addr);
extern void delay(unsigned long time);

void early_uart_init(void);
void uart_putc(char ch);
void uart_send_string(char *str);
