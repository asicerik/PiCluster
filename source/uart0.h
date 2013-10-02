#pragma once

#define UART0_BASE   0x20201000
#define UART0_DR     (UART0_BASE+0x00)
#define UART0_RSRECR (UART0_BASE+0x04)
#define UART0_FR     (UART0_BASE+0x18)
#define UART0_ILPR   (UART0_BASE+0x20)
#define UART0_IBRD   (UART0_BASE+0x24)
#define UART0_FBRD   (UART0_BASE+0x28)
#define UART0_LCRH   (UART0_BASE+0x2C)
#define UART0_CR     (UART0_BASE+0x30)
#define UART0_IFLS   (UART0_BASE+0x34)
#define UART0_IMSC   (UART0_BASE+0x38)
#define UART0_RIS    (UART0_BASE+0x3C)
#define UART0_MIS    (UART0_BASE+0x40)
#define UART0_ICR    (UART0_BASE+0x44)
#define UART0_DMACR  (UART0_BASE+0x48)
#define UART0_ITCR   (UART0_BASE+0x80)
#define UART0_ITIP   (UART0_BASE+0x84)
#define UART0_ITOP   (UART0_BASE+0x88)
#define UART0_TDR    (UART0_BASE+0x8C)

/* Data register bit defs */
#define UART_DR_CTS			0x000000001
#define UART_DR_RSV_DSR		0x000000002
#define UART_DR_RSV_DCD		0x000000004
#define UART_DR_BUSY		0x000000008
#define UART_DR_RX_EMPTY	0x000000010
#define UART_DR_TX_FULL		0x000000020
#define UART_DR_RX_FULL		0x000000040
#define UART_DR_TX_EMPTY	0x000000080
#define UART_DR_RSV_RI		0x000000100

/* Control register bit defs */
#define UART_CR_UART_EN		0x000000001
#define UART_CR_RSV_SIREN	0x000000002
#define UART_CR_RSV_SIRLP	0x000000004
#define UART_CR_LPBK_EN		0x000000080
#define UART_CR_TX_EN		0x000000100
#define UART_CR_RX_EN		0x000000200
#define UART_CR_RSV_DTR		0x000000400
#define UART_CR_RTS			0x000000800
#define UART_CR_RSV_OUT1	0x000001000
#define UART_CR_RSV_OUT2	0x000002000
#define UART_CR_RTS_EN		0x000004000
#define UART_CR_CTS_EN		0x000008000

/* Line Control register bit defs */
#define UART_LCRH_BREAK		0x000000001
#define UART_LCRH_PAR_EN	0x000000002
#define UART_LCRH_EVEN_PAR	0x000000004
#define UART_LCRH_TWO_STOP	0x000000008
#define UART_LCRH_FIFO_EN	0x000000010
#define UART_LCRH_5_BITS	0x000000000
#define UART_LCRH_6_BITS	0x000000020
#define UART_LCRH_7_BITS	0x000000040
#define UART_LCRH_8_BITS	0x000000060
#define UART_LCRH_STICK_PAR	0x000000080

/* Interrupt clear register bit defs */
#define UART_ICR_ALL		0x0000007f2

typedef enum
{
	eBase10,
	eBase16
} eIntegerBase;

void UartPutChar(char val);
char UartGetChar();
void UartFlushRxFifo();
void UartInit ();
int UartPrintf(const char *fmt, ...);
int UartPrintInt(int i, eIntegerBase base);
int UartPrintPtr(void* i);

