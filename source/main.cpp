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

// Image imports from binary
extern BMPImage			gCarTopViewRom;
extern BMPImage			gWaterTempImageRom;
extern BMPImage			gFuelImageRom;
extern BMPImage			gLeftArrowImageRom;
extern BMPImage			gRightArrowImageRom;

BMPImage*				gCarTopView		= &gCarTopViewRom;
BMPImage*				gWaterTempImage = &gWaterTempImageRom;
BMPImage*				gFuelImage		= &gFuelImageRom;
BMPImage*				gLeftArrowImage	= &gLeftArrowImageRom;
BMPImage*				gRightArrowImage= &gRightArrowImageRom;

// External functions
extern "C"
{
	int mmuInit ( void );
}

int main(int argc, char** argv)
{
	bcm2835_init();

	// Set pin 16 ('ACT' led) to output mode
	bcm2835_gpio_fsel(16, BCM2835_GPIO_FSEL_OUTP);
	// Turn off the led
	bcm2835_gpio_set(16);

	UartInit();

	dmaInit();

	int i=0;
	while (i-- > 0)
	{
		UartPrintf("\r%d    ", i);
		bcm2835_gpio_clr(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
		bcm2835_gpio_set(16);
		bcm2835_st_delay(bcm2835_st_read(), 500000);
	}
	UartPrintf("\n");

	// The BMP images have their R & B channels swapped, so reverse them
	GraphicsContextBase::PrepareBMP(gCarTopView);
	GraphicsContextBase::PrepareBMP(gWaterTempImage);
	GraphicsContextBase::PrepareBMP(gFuelImage);
	GraphicsContextBase::PrepareBMP(gLeftArrowImage);
	GraphicsContextBase::PrepareBMP(gRightArrowImage);

	UartPrintf("Calling cluster.Init()\n");
	InstrumentCluster cluster;
	FramebufferProperties properties;
	properties.mGeometry.x = properties.mGeometry.y = 0;
	properties.mGeometry.w	= 1280;
	properties.mGeometry.h	= 480;
	properties.mBitsPerPixel= 32;

	bool res;
	GraphicsContextPi ctx;
	res = cluster.Init(properties.mGeometry);
	ctx = cluster.GetPrimarySurface().GetGraphicsContext();
	if (res)
	{
		UartPrintf("Cluster.Init() succeeded\n");
	}
	else
	{
		UartPrintf("Cluster.Init() failed!\n");
	}

	// MMU init
	UartPrintf("Initializing MMU\n");
	if (mmuInit())
	{
		UartPrintf("MMU failed!\n");
	}
	else
	{
		UartPrintf("MMU init succeeded!\n");
	}

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
		bool on = false;
		int i=0;

		int frames = 0;
		uint64_t lastUpdate = bcm2835_st_read();

		UartPrintf("\n");

		// Turn on the led
		bcm2835_gpio_clr(16);

		while (res)
		{
			cluster.Update();

			cluster.Draw();

			// Wait for the vertical blank so we don't get tearing
			ctx.WaitForVSync();

			// Copy to the screen
			std::vector<Rect>& dirty = cluster.mDirty.GetDirtyRects();
			std::vector<Rect>::iterator iter = dirty.begin();
			for (; iter != dirty.end(); iter++)
				cluster.GetPrimarySurface().GetGraphicsContext().CopyBackToFront(*iter);

			Color32* ptr = ctx.GetFrontBuffer();

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
