#include "stdint.h"
#include <cstddef>
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"
#include "uart0.h"

#ifdef WIN32
bool
GraphicsContextWin::AllocatePrimaryFramebuffer(FramebufferProperties& properties)
{
	bool res = false;
	do
	{
		FreeFramebuffer();

		res = AllocateWindowsBitmap(properties, mPrimaryDC, mPrimaryBitmapInfo, mPrimaryBitmap);

	} while (false);
	return res;
}

bool
GraphicsContextWin::AllocateFramebuffer(FramebufferProperties& properties)
{
	bool res = false;
	do
	{
		FreeFramebuffer();

		res = AllocateWindowsBitmap(properties, mDC, mBitmapInfo, mBitmap);

	} while (false);
	return res;
}

bool
GraphicsContextWin::AllocateWindowsBitmap(FramebufferProperties& properties, HDC& dc, BITMAPINFO& info, HBITMAP& bitmap)
{
	bool res = false;
	do
	{
		FreeFramebuffer();
		mDC = CreateCompatibleDC(NULL);

		if (mDC == NULL)
		{
			break;
		}

		if ((properties.mGeometry.w < 1) || (properties.mGeometry.h < 1) || (properties.mBitsPerPixel != 32))
			break;

		mBitmapInfo.bmiHeader.biSize		= sizeof( mBitmapInfo.bmiHeader );
		mBitmapInfo.bmiHeader.biWidth		= properties.mGeometry.w;
		mBitmapInfo.bmiHeader.biHeight		= -properties.mGeometry.h; // -ive draws top down
		mBitmapInfo.bmiHeader.biPlanes		= 1;
		mBitmapInfo.bmiHeader.biCompression	= BI_RGB;
		mBitmapInfo.bmiHeader.biBitCount	= properties.mBitsPerPixel;
		mBitmapInfo.bmiHeader.biSizeImage	= (properties.mBitsPerPixel/8)*properties.mGeometry.w*properties.mGeometry.h; 

		// Create the front buffer bitmap
		// TODO : double buffer
		mBitmap = CreateDIBSection( mDC, &mBitmapInfo, DIB_RGB_COLORS, (void **)&mFrontBufferPtr, NULL, 0 );

		// Now select the buffer into the device context so that we can use the Windows graphics calls.
		SelectObject( mDC, mBitmap );

		res = true;
	} while (false);
	return res;
}

void 
GraphicsContextWin::FreeFramebuffer()
{
	DeleteObject(mBitmap);
	mFrontBufferPtr = NULL;
	mBackBufferPtr = NULL;
}

void 
GraphicsContextWin::FillRectangle(Rect rect, Color32 argb)
{
	RECT winRect;
	HBRUSH brush;
	winRect.left	= rect.x;
	winRect.right	= rect.x + rect.w;
	winRect.top		= rect.y;
	winRect.bottom	= rect.y + rect.h;
	brush = CreateSolidBrush(RGB(argb.r, argb.g, argb.b));
	FillRect(mDC, &winRect, brush);
	DeleteObject(brush);
}

void
GraphicsContextWin::GradientRectangle(int16_t angle, std::vector<GradientStop>& stops)
{
	if (stops.empty())
		return;

	std::vector<int16_t>  yStops;
	std::vector<Color32f> colorDeltas;
	for (size_t i=0; i<stops.size(); i++)
	{
		int16_t deltaY = (int16_t)(stops[i].mPosition * mFBProperties.mGeometry.h);
		yStops.push_back(deltaY);
		if ((i+1) < stops.size())
		{
			Color32f delta;
			delta.a = (stops[i+1].mColor.a - stops[i].mColor.a) / (float)deltaY;
			delta.r = (stops[i+1].mColor.r - stops[i].mColor.r) / (float)deltaY;
			delta.g = (stops[i+1].mColor.g - stops[i].mColor.g) / (float)deltaY;
			delta.b = (stops[i+1].mColor.b - stops[i].mColor.b) / (float)deltaY;
			colorDeltas.push_back(delta);
		}
	}

	for (int16_t y=0; y<mFBProperties.mGeometry.h;y++)
	{
		for (int16_t x=0; x<mFBProperties.mGeometry.w;x++)
		{
		}
	}
}

#else

GraphicsContextPi::GraphicsContextPi()
{
}

GraphicsContextPi::~GraphicsContextPi()
{
	FreeFramebuffer();
}

bool
GraphicsContextPi::AllocatePrimaryFramebuffer(FramebufferProperties& properties)
{
	bool res = false;
	do
	{
		FreeFramebuffer();
		res = true;
	} while (false);
	return res;
}

bool
GraphicsContextPi::AllocateFramebuffer(FramebufferProperties& properties)
{
	bool res = false;
	do
	{
		FreeFramebuffer();
		res = true;
	} while (false);
	return res;
}

void
GraphicsContextPi::FreeFramebuffer()
{
	mFrontBufferPtr = NULL;
	mBackBufferPtr = NULL;
}

void
GraphicsContextPi::FillRectangle(Rect rect, Color32 argb)
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
