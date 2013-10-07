#include "stddef.h"
#include "stdlib.h"
#include "stdio.h"
#include "uart0.h"
#include "bcm2835.h"
#include "GraphicsShim.h"

int main(int argc, char** argv)
{
	// Set pin 16 ('ACT' led) to output mode
	bcm2835_gpio_fsel(16, BCM2835_GPIO_FSEL_OUTP);
	// Turn on the led
	bcm2835_gpio_clr(16);

	UartInit();

	bcm2835_init();

	int i=5;
	while (i-- > 0)
	{
		//UartPrintf("read=%x, st[0]=%x, st[1]=%x, st[2]=%x, st[3]=%x\n", (uint32_t)(bcm2835_st_read() & 0xffffffff), bcm2835_st[0], bcm2835_st[1], bcm2835_st[2], bcm2835_st[3]);
		bcm2835_gpio_set(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		bcm2835_gpio_clr(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
	}
	GraphicsContextPi ctx;
	FramebufferProperties properties;
	properties.mGeometry.w	= 1280;
	properties.mGeometry.h	= 480;
	properties.mBitsPerPixel= 32;
	if (ctx.AllocatePrimaryFramebuffer(properties))
	{
		uint32_t color = 0xff3f0000;
		{
			for (int y=0;y<properties.mGeometry.h;y++)
			{
				uint32_t* ptr = (uint32_t*)ctx.GetFrontBuffer() + y * (properties.mStride >> 2);
				for (int x=0;x<properties.mGeometry.w;x++)
				{
					*ptr++ = color;
				}
			}
		}
	}
	while (1)
	{
		//UartPrintf("read=%x, st[0]=%x, st[1]=%x, st[2]=%x, st[3]=%x\n", (uint32_t)(bcm2835_st_read() & 0xffffffff), bcm2835_st[0], bcm2835_st[1], bcm2835_st[2], bcm2835_st[3]);
		bcm2835_gpio_set(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		bcm2835_gpio_clr(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
	}
	return 0;
}
