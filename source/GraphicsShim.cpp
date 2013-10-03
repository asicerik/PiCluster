#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"
#include "uart0.h"

#ifdef WIN32
GraphicsContext* CreateGraphicsContext()
{
	GraphicsContext* ctx = new GraphicsContext();

	if (ctx != NULL)
	{
		ctx->mDC = CreateCompatibleDC(NULL);
	}
	return ctx;
}

void FreeGraphicsContext(GraphicsContext* ctx)
{
	if (ctx != NULL)
	{
		DeleteDC(ctx->mDC);
		free(ctx);
	}
}

int32_t AllocateFrameBuffer(GraphicsContext* ctx)
{
	if (ctx == NULL || ctx->mDC == NULL || (ctx->mWidth < 1) || (ctx->mHeight < 1) || (ctx->mBitsPerPixel != 32))
		return -1;

	ctx->mBitmapInfo.bmiHeader.biSize				= sizeof( ctx->mBitmapInfo.bmiHeader );
	ctx->mBitmapInfo.bmiHeader.biWidth				= ctx->mWidth;
	ctx->mBitmapInfo.bmiHeader.biHeight				= -ctx->mHeight;
	ctx->mBitmapInfo.bmiHeader.biPlanes				= 1;
	ctx->mBitmapInfo.bmiHeader.biCompression		= BI_RGB;
	ctx->mBitmapInfo.bmiHeader.biBitCount			= ctx->mBitsPerPixel;
	ctx->mBitmapInfo.bmiHeader.biSizeImage			= (ctx->mBitsPerPixel/8)*ctx->mWidth*ctx->mHeight; 

	// Create the source bitmap.
	ctx->mBitmap = CreateDIBSection( ctx->mDC, &ctx->mBitmapInfo, DIB_RGB_COLORS, (void **)&ctx->mABGR, NULL, 0 );

	// Now select the back buffer.
	SelectObject( ctx->mDC, ctx->mBitmap );

	return 0;
}

void FreeFrameBuffer(GraphicsContext* ctx)
{
	DeleteObject(ctx->mBitmap);
	ctx->mBitmap = NULL;
	ctx->mBitmap = NULL;
}

void FillRectangle(GraphicsContext* ctx, Rect rect, Color32 argb)
{
	RECT winRect;
	HBRUSH brush;
	winRect.left	= rect.x;
	winRect.right	= rect.x + rect.w;
	winRect.top		= rect.y;
	winRect.bottom	= rect.y + rect.h;
	brush = CreateSolidBrush(RGB(argb.r, argb.g, argb.b));
	FillRect(ctx->mDC, &winRect, brush);
	DeleteObject(brush);
}

#else
GraphicsContext* CreateGraphicsContext()
{
	return 0;
}

void FreeGraphicsContext(GraphicsContext* ctx)
{
}

int32_t AllocateFrameBuffer(GraphicsContext* ctx)
{
	return -1;
}

void FreeFrameBuffer(GraphicsContext* ctx)
{
}

bool MailboxWrite(uintptr_t message, enum VideoCoreChannel channel)
{
	bool res = false;
	UartPrintf("VideoCoreMailboxPeek is %p\n", VideoCoreMailboxPeek);
	UartPrintf("VideoCoreMailboxStatus is %p\n", VideoCoreMailboxStatus);
	UartPrintf("VideoCoreMailboxWrite is %p\n", VideoCoreMailboxWrite);
	UartPrintf("VideoCoreMailboxPeek is %p\n", *VideoCoreMailboxPeek);
	UartPrintf("VideoCoreMailboxStatus is %p\n", *VideoCoreMailboxStatus);
	UartPrintf("VideoCoreMailboxWrite is %p\n", *VideoCoreMailboxWrite);
	do
	{
		if ((message & 0xf) != 0)
		{
			UartPrintf("Error, message must be 16 byte aligned\n");
			break;
		}
		if (channel > eVCTouchscreenChannel)
		{
			UartPrintf("Error, channel was invalid\n");
			break;
		}
		while ((*VideoCoreMailboxStatus & VideoCoreMailboxStatusWriteReady) != 0)
		{
			// sit here until the mailbox opens up
			UartPrintf("VideoCoreMailboxStatus is %p\n", *VideoCoreMailboxStatus);
		}

		// Write the message and channel to the mailbox write port
		*VideoCoreMailboxWrite = message + channel;

		res = true;
	} while (false);
	return res;
}

bool MailboxRead(uintptr_t& message, enum VideoCoreChannel channel)
{
	bool res = false;
	do
	{
		if (channel > eVCTouchscreenChannel)
		{
			UartPrintf("Error, channel was invalid\n");
			break;
		}
		// We may see a message on a channel other than ours apparently
		// So, wait until ours shows up
		uint32_t readChannel = 0xF;	// an invalid channel
		while (readChannel != channel)
		{
			while ((*VideoCoreMailboxStatus & VideoCoreMailboxStatusReadReady) != 0)
			{
				// sit here until the mailbox has something in it
				UartPrintf("VideoCoreMailboxStatus is %p\n", *VideoCoreMailboxStatus);
			}
			// Mask off the channel
			readChannel = *VideoCoreMailboxRead & 0xf;
			message = *VideoCoreMailboxRead & 0xffffffff0;
		}

		res = true;
	} while (false);
	return res;
}

bool CreatePrimaryFramebuffer(VideoCoreFramebufferDescriptor& fbDesc)
{
	bool res = false;
	do
	{
		if (fbDesc.width > kMaxFramebufferWidth || fbDesc.height > kMaxFramebufferHeight ||
			fbDesc.depth > kMaxFramebufferDepth)
		{
			UartPrintf("Error, framebuffer properties invalid\n");
			break;
		}

		// We need to use a cache coherent address so that we don't have to wait
		// for the VPU to flush its cache
		uintptr_t message = ((uintptr_t)&fbDesc) | kVideoCoreMailboxDisableCache;

		if (!MailboxWrite(message, eVCFramebufferChannel))
		{
			UartPrintf("Error, MailboxWrite() failed\n");
			break;
		}

		uintptr_t readValue;
		if (!MailboxRead(readValue, eVCFramebufferChannel))
		{
			UartPrintf("Error, MailboxWrite() failed\n");
			break;
		}

		if (readValue != 0)
		{
			UartPrintf("Error, framebuffer allocation failed\n");
			break;
		}

		while (fbDesc.buffer == 0)
		{
			// We may have to wait for the framebuffer address to show up
			UartPrintf("fbDesc.buffer is %p\n", fbDesc.buffer);
		}
		res = true;
		UartPrintf("Success, framebuffer address is %p\n", fbDesc.buffer);
	} while (false);
	return res;
}

#endif
