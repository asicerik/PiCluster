#include "stdint.h"
#include "stdlib.h"
#include "malloc.h"
#include <cstddef>
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"
#include "uart0.h"

GraphicsContextBase*	GraphicsContextBase::mPrimaryContext = NULL;

Color32f 
Color32::ToColor32f()
{
	Color32f out;
	out.a = (float)a;
	out.r = (float)r;
	out.g = (float)g;
	out.b = (float)b;
	return out;
}

Color32 
Color32f::ToColor32()
{
	Color32 out;
	out.a = SATURATE_FI(a);
	out.r = SATURATE_FI(r);
	out.g = SATURATE_FI(g);
	out.b = SATURATE_FI(b);
	return out;
}

GraphicsContextBase::GraphicsContextBase()
{
	mSelectedSurface	= NULL;
	mSurfaceSelection	= eUnknown;
	mCurrBufferPtr		= NULL;
	mFrontBufferPtr		= NULL;
	mBackBufferPtr		= NULL;
	mOffset.x = mOffset.y = 0;
}

GraphicsContextBase* 
GraphicsContextBase::SelectSurface(SurfaceSelection selection)
{
	mCurrBufferPtr = NULL;
	mSelectedSurface = NULL;
	mSurfaceSelection = selection;
	switch (selection)
	{
		case ePrimaryFront:
			mCurrBufferPtr   = (mPrimaryContext ? mPrimaryContext->GetFrontBuffer() : NULL);
			mSelectedSurface = mPrimaryContext;
			break;
		case ePrimaryBack:
			mCurrBufferPtr   = (mPrimaryContext ? mPrimaryContext->GetBackBuffer() : NULL);;
			mSelectedSurface = mPrimaryContext;
			break;
		case eFront:
			mCurrBufferPtr   = mFrontBufferPtr;
			mSelectedSurface = this;
			break;
		case eBack:
			mCurrBufferPtr   = mBackBufferPtr;
			mSelectedSurface = this;
			break;
	}
	return mSelectedSurface;
}

void
GraphicsContextBase::GradientRectangle(int16_t angle, std::vector<GradientStop>& stops)
{
	UartPrintf("GradientRectangle()\n");
	if (stops.empty())
		return;

	std::vector<int16_t>  yStops;
	std::vector<Color32f> colorDeltas;
	Color32f currColor		= stops[0].mColor.ToColor32f();
	Color32f noColorDelta   = { 0, 0, 0, 0 };
	Color32f currColorDelta = { 0, 0, 0, 0 };
	for (size_t i=0; i<stops.size(); i++)
	{
		if ((i+1) < stops.size())
		{
			Color32f delta;
			int16_t yCurr = (int16_t)(stops[i].mPosition * mFBProperties.mGeometry.h);
			int16_t yNext = (int16_t)(stops[i+1].mPosition * mFBProperties.mGeometry.h);
			int16_t deltaY = yNext - yCurr;
			yStops.push_back(deltaY);
			delta.a = (stops[i+1].mColor.a - stops[i].mColor.a) / (float)deltaY;
			delta.r = (stops[i+1].mColor.r - stops[i].mColor.r) / (float)deltaY;
			delta.g = (stops[i+1].mColor.g - stops[i].mColor.g) / (float)deltaY;
			delta.b = (stops[i+1].mColor.b - stops[i].mColor.b) / (float)deltaY;
			if (colorDeltas.empty())
				currColorDelta = delta;
			colorDeltas.push_back(delta);
		}
	}
	UartPrintf("GradientRectangle() : w=%d,h=%d,angle=%d,stops=%d\n",
			(int)mFBProperties.mGeometry.w,
			(int)mFBProperties.mGeometry.h,
			(int)angle,
			(int)stops.size());

	size_t stopIndex = 0;
	for (int16_t y=0; y<mFBProperties.mGeometry.h;)
	{
		GradientLine(currColor.ToColor32(), noColorDelta, 0, mFBProperties.mGeometry.w-1, y);
		
		if ((++y >= yStops[stopIndex]) && ((stopIndex + 1) < yStops.size()))
		{
			stopIndex++;
			if (stopIndex < colorDeltas.size())
				currColorDelta = colorDeltas[stopIndex];
			else
				currColorDelta = noColorDelta;
		}
		currColor.a += currColorDelta.a;
		currColor.r += currColorDelta.r;
		currColor.g += currColorDelta.g;
		currColor.b += currColorDelta.b;
	}
}

void
GraphicsContextBase::CopyToPrimary(std::vector<Rect> rects)
{
	if (!mPrimaryContext)
		return;

	UartPrintf("CopyToPrimary: w=%d,h=%d,stride=%d,ptr=%p,pw=%d,ph=%d,pstride=%d,pptr=%p\n",
			mFBProperties.mGeometry.w,
			mFBProperties.mGeometry.h,
			mFBProperties.mStride,
			mCurrBufferPtr,
			mPrimaryContext->GetFramebufferProperties().mGeometry.w,
			mPrimaryContext->GetFramebufferProperties().mGeometry.h,
			mPrimaryContext->GetFramebufferProperties().mStride,
			mPrimaryContext->GetSelectedFramebuffer()
	);

	std::vector<Rect>::iterator iter = rects.begin();
	for (; iter != rects.end(); iter++)
	{
		// Make sure we stay within the bounds of the source buffer
		int16_t srcY = iter->y - mOffset.y;
		int16_t srcH = (srcY + iter->h);
		srcH = Clip(srcH, 0, mFBProperties.mGeometry.h);
		srcH = Clip(srcH, 0, iter->h);
		int16_t srcX = iter->x - mOffset.x;
		int16_t srcW = (srcX + iter->w);
		srcW = Clip(srcW, 0, mFBProperties.mGeometry.w);
		srcW = Clip(srcW, 0, iter->w);

		UartPrintf("CopyToPrimary: srcW=%d, srcH=%d, iter->w=%d, iter->h=%d\n", srcW, srcH, iter->w, iter->h);

		// Now check the destination buffer
		int16_t dstH = iter->y + srcH;
		dstH = Clip(dstH, 0, mPrimaryContext->GetFramebufferProperties().mGeometry.h);
		int16_t dstW = iter->x + srcW;
		dstW = Clip(dstW, 0, mPrimaryContext->GetFramebufferProperties().mGeometry.w);
		UartPrintf("CopyToPrimary: dstW=%d, dstH=%d\n", dstW, dstH);

		int16_t h = ((iter->y + srcH) < dstH) ? (iter->y + srcH) : dstH;
		int16_t w = ((iter->x + srcW) < dstW) ? (iter->x + srcW) : dstW;

		UartPrintf("CopyToPrimary: offset=%d,%d, iter->y=%d, h=%d, iter->x=%d, w=%d, stride=%d\n",
			mOffset.x, mOffset.y,
			iter->y, h, iter->x, w, mFBProperties.mStride);
		for (int16_t y = iter->y; y < (h); y++)
		{
			Color32* src = mCurrBufferPtr + srcX + (srcY) * (mFBProperties.mStride >> 2);
			Color32* dst = mPrimaryContext->GetSelectedFramebuffer() + iter->x + y * (mPrimaryContext->GetFramebufferProperties().mStride >> 2);
			//UartPrintf("CopyToPrimary: src=%p, dst=%p\n", src,dst);
			for (int16_t x = iter->x; x < (w); x++)
			{
				*dst++ = *src++;
			}
			srcY++;
			if (srcY >= mFBProperties.mGeometry.h)
				break;
		}
	}
}
void
GraphicsContextBase::GradientLine(Color32 startColor, Color32f colorDelta, int16_t startX, int16_t endX, int16_t y)
{
	Color32f currColor = startColor.ToColor32f();
	if (mCurrBufferPtr == NULL)
		return;

	if ((mSurfaceSelection == ePrimaryFront) || (mSurfaceSelection == ePrimaryBack))
	{
		// Adjust x,y to position ourselves into the primary buffer
		startX	+= mOffset.x;
		endX	+= mOffset.x;
		y		+= mOffset.y;
	}

	Color32* ptr = mCurrBufferPtr + startX + y * GetSelectedSurface()->GetFramebufferProperties().mStride / sizeof(Color32);
	//UartPrintf("GradientLine() : ptr=%p, startX=%d, endX=%d, y=%d, stride=%d\n", ptr, (int)startX, (int)endX, (int)y, GetSelectedSurface()->GetFramebufferProperties().mStride);

	if (colorDelta.a == 0 && colorDelta.r == 0 && colorDelta.g == 0 && colorDelta.b == 0)
	{
		// Special case for no color delta
		for (int16_t x=startX; x<=endX; x++)
		{
			*ptr++ = startColor;
//#ifdef WIN32
//			//Sleep(0);
//#endif
		}
	}
	else
	{
		for (int16_t x=startX; x<=endX; x++)
		{
			*ptr++ = currColor.ToColor32();
//#ifdef WIN32
//			//Sleep(0);
//#endif
		}
	}
}

#ifdef WIN32
bool
GraphicsContextWin::AllocatePrimaryFramebuffer(FramebufferProperties& properties)
{
	mPrimaryContext = this;

	// For Windows, we don't need any special treatment for Windows
	// Blit to the Window surface is made outside this code. This really ends up
	// with one extra level of buffering. E.g. double or triple buffering.
	return AllocateFramebuffer(properties);
}

bool
GraphicsContextWin::AllocateFramebuffer(FramebufferProperties& properties)
{
	bool res = false;
	mFBProperties = properties;
	do
	{
		res = AllocateWindowsBitmap(mFBProperties, mDC, mBitmapInfo, mBitmap, mFrontBufferPtr);
		mCurrBufferPtr = mFrontBufferPtr;
		
		if (res && properties.mDoubleBuffer)
		{
			res = AllocateWindowsBitmap(mFBProperties, mDC, mBitmapInfo, mBackBufferBitmap, mBackBufferPtr);
			mCurrBufferPtr = mBackBufferPtr;
		}

	} while (false);
	return res;
}

bool
GraphicsContextWin::AllocateWindowsBitmap(FramebufferProperties& properties, HDC& dc, BITMAPINFO& info, HBITMAP& bitmap, Color32*& ptr)
{
	bool res = false;
	do
	{
		dc = CreateCompatibleDC(NULL);

		if (dc == NULL)
		{
			break;
		}

		if ((properties.mGeometry.w < 1) || (properties.mGeometry.h < 1) || (properties.mBitsPerPixel != 32))
			break;

		properties.mStride = properties.mGeometry.w * (properties.mBitsPerPixel / 8);

		info.bmiHeader.biSize		= sizeof( info.bmiHeader );
		info.bmiHeader.biWidth		= properties.mGeometry.w;
		info.bmiHeader.biHeight		= -properties.mGeometry.h; // -ive draws top down
		info.bmiHeader.biPlanes		= 1;
		info.bmiHeader.biCompression= BI_RGB;
		info.bmiHeader.biBitCount	= properties.mBitsPerPixel;
		info.bmiHeader.biSizeImage	= (properties.mStride)*properties.mGeometry.h; 

		// Create the front buffer bitmap
		bitmap = CreateDIBSection( dc, &info, DIB_RGB_COLORS, (void **)&ptr, NULL, 0 );
		memset(ptr, 128, info.bmiHeader.biSizeImage);

		// Now select the buffer into the device context so that we can use the Windows graphics calls.
		SelectObject( mDC, bitmap );

		res = true;
	} while (false);
	return res;
}

void 
GraphicsContextWin::FreeFramebuffer()
{
	DeleteObject(mBitmap);
	mFrontBufferPtr = NULL;
	if (mBackBufferPtr)
	{
		DeleteObject(mBackBufferBitmap);
		mBackBufferPtr = NULL;
	}
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


#else

// Allocate buffers from this mem
//static Color32* fbPtr = (Color32*)(32*1024*1024);

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
	mPrimaryContext = this;
	mFBProperties = properties;

	UartPrintf("AllocatePrimaryFramebuffer() this=%p\n", this);

	bool res = false;
	do
	{
		VideoCoreFramebufferDescriptor fbDesc;
		fbDesc.width  = fbDesc.virtualWidth  = properties.mGeometry.w;
		fbDesc.height = fbDesc.virtualHeight = properties.mGeometry.h;
		fbDesc.xOffset = fbDesc.yOffset = 0;
		fbDesc.depth = properties.mBitsPerPixel;
		fbDesc.pitch = properties.mGeometry.w * (properties.mBitsPerPixel / 8);
		res = CreatePrimaryFramebuffer(fbDesc);
		if (res)
		{
			mFBProperties.mStride = fbDesc.pitch;
			mFrontBufferPtr = (Color32*)fbDesc.buffer;
			mCurrBufferPtr  = mFrontBufferPtr;
		}
		UartPrintf("Allocate primary framebuffer : front = %x\n", mFrontBufferPtr);
	} while (false);
	return res;
}

bool
GraphicsContextPi::AllocateFramebuffer(FramebufferProperties& properties)
{
	UartPrintf("AllocateFramebuffer() this=%p\n", this);

	bool res = false;
	do
	{
		properties.mStride = properties.mGeometry.w * (properties.mBitsPerPixel / 8);

		UartPrintf("Allocate framebuffer w=%d, h=%d, bpp=%d, stride=%d\n", properties.mGeometry.w, properties.mGeometry.h,
			properties.mBitsPerPixel, properties.mStride);
		size_t bufferSize = properties.mStride * properties.mGeometry.h;
		mFrontBufferPtr = new Color32[bufferSize/4];
		res = mFrontBufferPtr != NULL;
		UartPrintf("Allocate framebuffer size=%d, res=%d: front = %x\n", bufferSize, (int)res, mFrontBufferPtr);

	} while (false);

	mFBProperties = properties;

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

bool
GraphicsContextPi::CreatePrimaryFramebuffer(VideoCoreFramebufferDescriptor& fbDesc)
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

bool MailboxWrite(uintptr_t message, enum VideoCoreChannel channel)
{
	bool res = false;
//	UartPrintf("VideoCoreMailboxPeek is %p\n", VideoCoreMailboxPeek);
//	UartPrintf("VideoCoreMailboxStatus is %p\n", VideoCoreMailboxStatus);
//	UartPrintf("VideoCoreMailboxWrite is %p\n", VideoCoreMailboxWrite);
//	UartPrintf("VideoCoreMailboxPeek is %p\n", *VideoCoreMailboxPeek);
//	UartPrintf("VideoCoreMailboxStatus is %p\n", *VideoCoreMailboxStatus);
//	UartPrintf("VideoCoreMailboxWrite is %p\n", *VideoCoreMailboxWrite);
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
			//UartPrintf("VideoCoreMailboxStatus is %p\n", *VideoCoreMailboxStatus);
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
				//UartPrintf("VideoCoreMailboxStatus is %p\n", *VideoCoreMailboxStatus);
			}
			// Mask off the channel
			readChannel = *VideoCoreMailboxRead & 0xf;
			message = *VideoCoreMailboxRead & 0xffffffff0;
		}

		res = true;
	} while (false);
	return res;
}

#endif
