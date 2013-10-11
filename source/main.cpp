#include "stddef.h"
#include "stdlib.h"
#include "stdio.h"
#include "uart0.h"
#include "bcm2835.h"
#include "GraphicsShim.h"
#include "ClusterElement.h"
#include "InstrumentCluster.h"

int main(int argc, char** argv)
{
	bcm2835_init();

	// Set pin 16 ('ACT' led) to output mode
	bcm2835_gpio_fsel(16, BCM2835_GPIO_FSEL_OUTP);
	// Turn on the led
	bcm2835_gpio_clr(16);

	UartInit();

	int i=5;
	while (i-- > 0)
	{
		UartPrintf("read=%x, st[0]=%x, st[1]=%x, st[2]=%x, st[3]=%x\n", (uint32_t)(bcm2835_st_read() & 0xffffffff), bcm2835_st[0], bcm2835_st[1], bcm2835_st[2], bcm2835_st[3]);
		bcm2835_gpio_clr(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		bcm2835_gpio_set(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
	}
	InstrumentCluster cluster;
	GraphicsContextPi ctx;
	FramebufferProperties properties;
	properties.mGeometry.x = properties.mGeometry.y = 0;
	properties.mGeometry.w	= 1280;
	properties.mGeometry.h	= 480;
	properties.mBitsPerPixel= 32;
	if (true)
	{
		UartPrintf("Calling cluster.Init()\n");
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		cluster.Init(properties.mGeometry);
		cluster.GetPrimarySurface().Invalidate(properties.mGeometry);
		bool on = true;
		while (1)
		{
			if (on)
				bcm2835_gpio_clr(16);
			else
				bcm2835_gpio_set(16);

			//UartPrintf("Calling cluster.Update()\n");
			//bcm2835_st_delay(bcm2835_st_read(), 500000);
			cluster.Update();

			//UartPrintf("Calling cluster.Draw()\n");
			//bcm2835_st_delay(bcm2835_st_read(), 500000);
			cluster.Draw();

			on = !on;
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
