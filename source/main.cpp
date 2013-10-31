#include "stddef.h"
#include "stdlib.h"
#include "stdio.h"
#include "uart0.h"
#include "bcm2835.h"
#include "dma.h"
#include "GraphicsShim.h"
#include "ClusterElement.h"
#include "InstrumentCluster.h"
#include "Tests.h"

// Font import
extern FontDatabaseFile	gFontErasDemi18Rom;		// 18 point Eras Demi ITC
FontDatabaseFile*		gFontErasDemi18 = &gFontErasDemi18Rom;

int main(int argc, char** argv)
{
	bcm2835_init();

	// Set pin 16 ('ACT' led) to output mode
	bcm2835_gpio_fsel(16, BCM2835_GPIO_FSEL_OUTP);
	// Turn on the led
	bcm2835_gpio_clr(16);

	UartInit();
	dmaInit();

	UartPrintf("Calling cluster.Init()\n");
	InstrumentCluster cluster;
	FramebufferProperties properties;
	properties.mGeometry.x = properties.mGeometry.y = 0;
	properties.mGeometry.w	= 1280;
	properties.mGeometry.h	= 480;
	properties.mBitsPerPixel= 32;
	bool res = cluster.Init(properties.mGeometry);
	GraphicsContextPi ctx = cluster.GetPrimarySurface().GetGraphicsContext();
	if (res)
	{
		UartPrintf("Cluster.Init() succeeded\n");
	}
	else
	{
		UartPrintf("Cluster.Init() failed!\n");
	}

	int i=7;
	while (i-- > 0)
	{
		UartPrintf("\r%d    ", i);
		bcm2835_gpio_clr(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		bcm2835_gpio_set(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
	}
	UartPrintf("\n");

	/*
	Tests tests;
	if (!tests.RunAllTests())
	{
		i = 5000;
		while (i-- > 0)
		{
			UartPrintf("\rTest fail. Spinning - %d    ", i);
			bcm2835_gpio_clr(16);
			bcm2835_st_delay(bcm2835_st_read(), 500000);
			bcm2835_gpio_set(16);
			bcm2835_st_delay(bcm2835_st_read(), 500000);
		}
		UartPrintf("\n");
	}
	*/

	if (res)
	{
		cluster.GetPrimarySurface().Invalidate(properties.mGeometry);
		bool on = true;
		int i=0;

		int frames = 0;
		uint64_t lastUpdate = bcm2835_st_read();

		UartPrintf("\n");
		Rect srcRect(1,1,360,360);
		Rect dstRect(640,0,360,360);
		while (res)
		{
			cluster.Update();

			cluster.Draw();

			// Copy to the screen
			Rect dirty = cluster.mDirty.GetDirtyRect();
			cluster.GetPrimarySurface().GetGraphicsContext().CopyBackToFront(dirty);

			Color32* ptr = ctx.GetFrontBuffer();
//			uint32_t stride = 1280*4;
//			dstRect.x = 640;
//			dstRect.y = 0;
//			for (i=0;i<50;i++)
//			{
//				dmaBitBlt(ptr, dstRect, stride, ptr, srcRect, stride);
//				dstRect.x++;
//			}
//			for (i=0;i<50;i++)
//			{
//				dmaBitBlt(ptr, dstRect, stride, ptr, srcRect, stride);
//				dstRect.y++;
//			}


			frames++;
			// Update fps once per second (st_read is in microsecs)
			if ((bcm2835_st_read() - lastUpdate) > 1000000)
			{
				if (on)
					bcm2835_gpio_clr(16);
				else
					bcm2835_gpio_set(16);
				on = !on;

				float fps = (float)frames / ((bcm2835_st_read() - lastUpdate) / 1000000.0);
				UartPrintf("FPS = %d\n", (int32_t)fps);
				frames = 0;
				lastUpdate = bcm2835_st_read();
				ctx.mProfileData.Snapshot(true);
				UartPrintf("Update:          %d\n", (uint32_t)ctx.mProfileData.mUpdate.GetSnapshot() / 1000);
				UartPrintf("Draw:            %d\n", (uint32_t)ctx.mProfileData.mDraw.GetSnapshot() / 1000);
				UartPrintf("CopyToPrimary:   %d\n", (uint32_t)ctx.mProfileData.mCopyToPrimary.GetSnapshot() / 1000);
				UartPrintf("CopyBackToFront: %d\n", (uint32_t)ctx.mProfileData.mCopyBackToFront.GetSnapshot() / 1000);
				UartPrintf("DMA:             %d\n", (uint32_t)ctx.mProfileData.mDMA.GetSnapshot() / 1000);
				UartPrintf("DrawTrapezoid:   %d\n", (uint32_t)ctx.mProfileData.mDrawTrapezoid.GetSnapshot() / 1000);
				UartPrintf("DrawLine:        %d\n", (uint32_t)ctx.mProfileData.mDrawLine.GetSnapshot() / 1000);
				UartPrintf("DrawText:        %d\n", (uint32_t)ctx.mProfileData.mDrawText.GetSnapshot() / 1000);
				UartPrintf("FloodFill:       %d\n", (uint32_t)ctx.mProfileData.mFloodFill.GetSnapshot() / 1000);
				UartPrintf("FillRectangle:   %d\n", (uint32_t)ctx.mProfileData.mFillRectangle.GetSnapshot() / 1000);
				UartPrintf("Region:          %d\n", (uint32_t)ctx.mProfileData.mRegion.GetSnapshot() / 1000);
			}
		}
	}
	while (1)
	{
		UartPrintf("Cluster Init() failed\n");
		bcm2835_gpio_set(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		bcm2835_gpio_clr(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
	}
	return 0;
}
