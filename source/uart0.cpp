#include "stdint.h"
#include "stdarg.h"
#include "uart0.h"
#include "bcm2835.h"

int32_t* uart0_dr		= (int32_t*)UART0_DR;
int32_t* uart0_rsrecr	= (int32_t*)UART0_RSRECR;
int32_t* uart0_fr		= (int32_t*)UART0_FR;
int32_t* uart0_ilpr		= (int32_t*)UART0_ILPR;
int32_t* uart0_ibrd		= (int32_t*)UART0_IBRD;
int32_t* uart0_fbrd		= (int32_t*)UART0_FBRD;
int32_t* uart0_lcrh		= (int32_t*)UART0_LCRH;
int32_t* uart0_cr		= (int32_t*)UART0_CR;
int32_t* uart0_ifls		= (int32_t*)UART0_IFLS;
int32_t* uart0_imsc		= (int32_t*)UART0_IMSC;
int32_t* uart0_ris		= (int32_t*)UART0_RIS;
int32_t* uart0_mis		= (int32_t*)UART0_MIS;
int32_t* uart0_icr		= (int32_t*)UART0_ICR;
int32_t* uart0_dmacr	= (int32_t*)UART0_DMACR;
int32_t* uart0_itcr		= (int32_t*)UART0_ITCR;
int32_t* uart0_itip		= (int32_t*)UART0_ITIP;
int32_t* uart0_itop		= (int32_t*)UART0_ITOP;
int32_t* uart0_tdr		= (int32_t*)UART0_TDR;

void UartPutChar(char val)
{
	/* Wait for some space to open up in the tx fifo */
	while (*uart0_fr & UART_DR_TX_FULL);

	/* Write the value to the data register (bits 7:0 only) */
	*uart0_dr = ((uint32_t)val) & 0x000000ff;
}

char UartGetChar()
{
	/* Wait for some data to show up in the rx fifo */
	while (*uart0_fr & UART_DR_RX_EMPTY);

	/* Read the value from the data register (bits 7:0 only) */
	return (uint8_t)(*uart0_dr & 0x000000ff);
}

void UartFlushRxFifo()
{
	/* Wait for some data to show up in the rx fifo */
	while (*uart0_fr & ~UART_DR_RX_EMPTY)
	{
		/* Read the value from the data register (bits 7:0 only) */
		*uart0_dr & 0x000000ff;
	}
}

void UartInit ()
{
	/* Set pins 14 and 15 to alt function 0 */
	bcm2835_gpio_fsel(14, BCM2835_GPIO_FSEL_ALT0);
	bcm2835_gpio_fsel(15, BCM2835_GPIO_FSEL_ALT0);

	/* Disable the uart for now */
	*uart0_cr = 0;

	/* Disable pull up/pull down resistors */
	bcm2835_gpio_pud(BCM2835_GPIO_PUD_OFF);
	bcm2835_gpio_pudclk(14, 1);
	bcm2835_delay(100);
	bcm2835_gpio_pudclk(15, 0);

	/* Clear any interrupts */
	*uart0_icr = UART_ICR_ALL;

	/* Set up the integer and fractional clock settings */
	/*
		From the BCM2835 Peripherals Guide
		http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

		(Note : this is for uart 0)
		GPIO pin 14 is TXD0 using alt function 0
		GPIO pin 15 is RXD0 using alt function 0

		"The baud rate divisor is calculated as follows: 
		Baud rate divisor BAUDDIV = (FUARTCLK/(16 Baud rate))
		where FUARTCLK is the UART reference clock frequency. 
		The BAUDDIV is comprised of the integer value IBRD and the fractional value FBRD. 
		NOTE: The contents of the IBRD and FBRD registers are not updated until 
		transmission or reception of the current character is complete."

		FBRD is a 6 bit number (0-63) to represent the fractional divisor.

		The uart clock is 3MHz, and we are going to use a fixed, 115,200 baud rate. So...
		BAUDDIV = 3,000,000 / (16 * 115,200) = 1.628
		IBRD = floor(1.628) = 1
		FBRD = 0.628 * 64   = 40
	*/
	*uart0_ibrd = 1;
	*uart0_fbrd = 40;

	/* Set the uart to 8 bits, no parity, 1 stop bit (8N1) and enable the fifos */
	*uart0_lcrh = UART_LCRH_FIFO_EN | UART_LCRH_8_BITS;

	/* Finally, enable the uart for send and receive */
	*uart0_cr = UART_CR_UART_EN | UART_CR_TX_EN | UART_CR_RX_EN;
}

int UartPrintf(const char *fmt, ...)
{
	int ret = 0;
	va_list args;
	char* ptr = (char*)fmt;

	va_start(args, fmt);
	while (true)
	{
		char c = *ptr++;
		if (c == '\0')
			break;

		if (c == '%')
		{
			// Get the next character to see what type of output is requested
			c = *ptr++;
			switch (c)
			{
				case '%':
					ret++;
					UartPutChar(c);
					break;
				case 's':
				{
					const char *str = va_arg(args, const char *);
					while (str)
					{
						ret++;
						UartPutChar(*str);
						str++;
					}
					break;
				}
				case 'd':
				{
					int i = va_arg(args, int);
					ret += UartPrintInt(i, eBase10);
					break;
				}
				case 'x':
				{
					int i = va_arg(args, int);
					ret += UartPrintInt(i, eBase16);
					break;
				}
				case 'p':
				{
					void* i = va_arg(args, void*);
					ret += UartPrintPtr(i);
					break;
				}
				default:
					ret++;
					UartPutChar(c);
					break;
			}
		}
		else if (c == '\n')
		{
			ret++;
			UartPutChar('\r');
			UartPutChar('\n');
		}
		else
		{
			ret++;
			UartPutChar(c);
		}
	}
	va_end(args);
	return ret;
}

int UartPrintInt(int i, eIntegerBase base)
{
	int ret = 0;
	char text[32];
	switch (base)
	{
		case eBase10:
		{
			if (i < 0)
			{
				i = -i;
				UartPutChar('-');
			}
			do
			{
				int curr = i % 10;
				text[ret++] = '0' + curr;
				i = i / 10;
			} while (i > 0);
			break;
		}
		case eBase16:
		{
			do
			{
				uint32_t curr = (uint32_t)i % 16;
				if (curr < 10)
					text[ret++] = '0' + curr;
				else
					text[ret++] = 'a' + (curr - 10);
				i = i / 16;
			} while (i > 0);
			break;
		}
		default:
			return -1;
	}
	if (ret > 0)
	{
		int i;
		text[ret] = '\0';
		for (i=ret-1;i>=0;i--)
			UartPutChar(text[i]);
	}
	return ret;
}

int UartPrintPtr(void* i)
{
	int ret = 0;
	uint32_t val = (uint32_t)i;
	char text[32];
	do
	{
		uint32_t curr = val % 16;
		if (curr < 10)
			text[ret++] = '0' + curr;
		else
			text[ret++] = 'a' + (curr - 10);
		val = val / 16;
	} while (val > 0);

	if (ret > 0)
	{
		int i;
		text[ret] = '\0';
		for (i=ret-1;i>=0;i--)
			UartPutChar(text[i]);
	}
	return ret;
}
