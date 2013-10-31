#include "stdint.h"
#include "stdio.h"
#include "stdarg.h"
#ifdef WIN32
#include "Windows.h"
#include <conio.h>


#else
#include "bcm2835.h"
#include "uart0.h"
#include "dma.h"
#include "GraphicsShim.h"


// We will use DMA channel 14 for our ops. Just in case something else is using channel 0, etc.
uint8_t*  dma0	 				= (uint8_t*)BCM2835_DMA_BASE;
uint8_t*  dma14 				= (uint8_t*)BCM2835_DMA_BASE + 0 * BCM2835_DMA_REGISTER_SPAN;
uint32_t* dma14ControlStatus	= (uint32_t*)(dma14 + BCM2835_DMA_CS);
uint32_t* dma14ControlBlockAddr	= (uint32_t*)(dma14 + BCM2835_DMA_CONBLK_AD);
uint32_t* dma14TransferInfo		= (uint32_t*)(dma14 + BCM2835_DMA_TI);
uint32_t* dma14SrcAddress		= (uint32_t*)(dma14 + BCM2835_DMA_SOURCE_AD);
uint32_t* dma14DestAddress		= (uint32_t*)(dma14 + BCM2835_DMA_DEST_AD);
uint32_t* dma14TransferLen		= (uint32_t*)(dma14 + BCM2835_DMA_TXFR_LEN);
uint32_t* dma14Stride			= (uint32_t*)(dma14 + BCM2835_DMA_STRIDE);
uint32_t* dma14NextControlBlock	= (uint32_t*)(dma14 + BCM2835_DMA_NEXTCONBK);
uint32_t* dma14Debug			= (uint32_t*)(dma14 + BCM2835_DMA_DEBUG);

uint32_t* dmaInterruptStatus	= (uint32_t*)(dma0 + BCM2835_DMA_INT_STATUS);
uint32_t* dmaEnable				= (uint32_t*)(dma0 + BCM2835_DMA_ENABLE);

bool dmaInit()
{
	// Reset our DMA controller
	*dma14ControlStatus			= DMA_CS_RESET;

	// Enable our DMA controller
	*dmaEnable					= 1;

	return true;
}

// 2D data transfer using DMA (Direct Memory Access)
// Due to the overhead of setting up and processing the DMA transfer, this should not be used for small
// transfers.
bool __attribute__((optimize("O0"))) dmaBitBlt(Color32* dst, Rect& dstRect, uint32_t dstStride, Color32* src, Rect& srcRect, uint32_t srcStride)
{
	PROFILE_START(GraphicsContextBase::mProfileData.mDMA)

	bool res = false;
	uint32_t w = dstRect.w < srcRect.w ? dstRect.w : srcRect.w;
	uint32_t h = dstRect.h < srcRect.h ? dstRect.h : srcRect.h;

	// The DMA controller does not use normal stride. It wants to know how far to advance
	// the address to the next location. NOT the distance between rows of data
	// So, we will adjust it here
	uint32_t dmaSrcStride = srcStride - w * sizeof(uint32_t);
	uint32_t dmaDstStride = dstStride - w * sizeof(uint32_t);

//	UartPrintf("srcRect=%d,%d:%d,%d, dstRect=%d,%d:%d,%d\n",
//			srcRect.x, srcRect.y, srcRect.w, srcRect.h,
//			dstRect.x, dstRect.y, dstRect.w, dstRect.h);
//	UartPrintf("w,h=%d,%d, src/dstStride=%d,%d\n", w, h, srcStride, dstStride);

	do
	{
		// Any stride over 16k is probably a bug
		if (dmaSrcStride > 16384 || dmaDstStride > 16384)
			break;

		if ((srcRect.x + w) > 1280 || (srcRect.x + w) < 1 || (srcRect.y + h) > 480 || (srcRect.y + h) < 1)
			break;

		if ((dstRect.x + w) > 1280 || (dstRect.x + w) < 1 || (dstRect.y + h) > 480 || (dstRect.y + h) < 1)
			break;

		DmaControlBlockData cb;
		cb.SourceAddress		= (uint32_t)(src + srcRect.x + (srcRect.y * srcStride / 4));
		cb.DestinationAddress	= (uint32_t)(dst + dstRect.x + (dstRect.y * dstStride / 4));
		cb.TransferLength		= (h << 16) | (w * sizeof(uint32_t));
		cb.Stride2D				= ((dmaDstStride & 0x7ffff) << 16) | (dmaSrcStride & 0x7fff);
		cb.NextControlBlock		= 0;
		cb.reserved0			= 0;
		cb.reserved1			= 0;
		// Configure the transfer
		cb.TransferInformation	=
				  //DMA_TI_WAIT_RESP  |
				  DMA_TI_TD_MODE	|	// 2d mode
				  DMA_TI_DEST_INC	|	// increment the dest addr after each transfer
				  DMA_TI_DEST_WIDTH	|	// use 128 bit transfers
				  DMA_TI_SRC_INC	|	// increment the source addr after each transfer
				  DMA_TI_SRC_WIDTH		// use 128 bit transfers
		;

		// To start the dma transfer, first write the address of our control block
		*dma14ControlBlockAddr		= (uint32_t)&cb;

		// Now tell the DMA engine to start the transfer
		*dma14ControlStatus			= DMA_CS_ACTIVE | DMA_CS_DISABLE_DEBUG;

		// We poll on the DMA control/status register to wait for completion
		while (*dma14ControlStatus & DMA_CS_ACTIVE)
		{
			//UartPrintf("DMA transfer pending. CS=%p, txfr_len=%p\n", *dma14ControlStatus, *dma14TransferLen);
		}
	//	UartPrintf("DMA transfer done. CS=%p, txfr_len=%p, txfr_info=%p, debug=%p\n", *dma14ControlStatus, *dma14TransferLen, *dma14TransferInfo, *dma14Debug);
	//	UartPrintf("int_status=%p, enable=%p\n", *dmaInterruptStatus, *dmaEnable);

		// Clear the control register
		*dma14ControlStatus			= 0;
		*dma14ControlBlockAddr		= 0;

		res = true;
	} while (false);

	PROFILE_STOP(GraphicsContextBase::mProfileData.mDMA)

	return res;
}

void dmaDumpAll()
{
	uint8_t* base = (uint8_t*)BCM2835_DMA_BASE;

	for (uint8_t channel=0;channel<16;channel++)
	{
		UartPrintf("DMA Channel %d (base=%p)\n", channel, base);
		if (channel == 14)
			base = (uint8_t*)BCM2835_DMA15_BASE;
		else
			base += BCM2835_DMA_REGISTER_SPAN;
		dmaDumpChannel(base);
	}
}

void dmaDumpChannel(uint8_t* base)
{
	UartPrintf("  Control and Status		%p\n", *(base + BCM2835_DMA_CS));
	UartPrintf("  Control Block Address		%p\n", *(base + BCM2835_DMA_CONBLK_AD));
	UartPrintf("  Transfer Information		%p\n", *(base + BCM2835_DMA_TI));
	UartPrintf("  Source Address		%p\n", *(base + BCM2835_DMA_SOURCE_AD));
	UartPrintf("  Destination Address		%p\n", *(base + BCM2835_DMA_DEST_AD));
	UartPrintf("  Transfer Length		%p\n", *(base + BCM2835_DMA_TXFR_LEN));
	UartPrintf("  2D Stride			%p\n", *(base + BCM2835_DMA_STRIDE));
	UartPrintf("  Next Control Block		%p\n", *(base + BCM2835_DMA_NEXTCONBK));
	UartPrintf("  Debug				%p\n", *(base + BCM2835_DMA_DEBUG));
}

#endif
