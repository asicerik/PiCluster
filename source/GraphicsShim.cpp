#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "malloc.h"
#include "Trig.h"
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

//	UartPrintf("CopyToPrimary: w=%d,h=%d,stride=%d,ptr=%p,pw=%d,ph=%d,pstride=%d,pptr=%p\n",
//			mFBProperties.mGeometry.w,
//			mFBProperties.mGeometry.h,
//			mFBProperties.mStride,
//			mCurrBufferPtr,
//			mPrimaryContext->GetFramebufferProperties().mGeometry.w,
//			mPrimaryContext->GetFramebufferProperties().mGeometry.h,
//			mPrimaryContext->GetFramebufferProperties().mStride,
//			mPrimaryContext->GetSelectedFramebuffer()
//	);

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

//		UartPrintf("CopyToPrimary: srcW=%d, srcH=%d, iter->w=%d, iter->h=%d\n", srcW, srcH, iter->w, iter->h);

		// Now check the destination buffer
		int16_t dstH = iter->y + srcH;
		dstH = Clip(dstH, 0, mPrimaryContext->GetFramebufferProperties().mGeometry.h);
		int16_t dstW = iter->x + srcW;
		dstW = Clip(dstW, 0, mPrimaryContext->GetFramebufferProperties().mGeometry.w);
//		UartPrintf("CopyToPrimary: dstW=%d, dstH=%d\n", dstW, dstH);

		int16_t h = ((iter->y + srcH) < dstH) ? (iter->y + srcH) : dstH;
		int16_t w = ((iter->x + srcW) < dstW) ? (iter->x + srcW) : dstW;

//		UartPrintf("CopyToPrimary: offset=%d,%d, iter->y=%d, h=%d, iter->x=%d, w=%d, stride=%d\n",
//			mOffset.x, mOffset.y,
//			iter->y, h, iter->x, w, mFBProperties.mStride);
		for (int16_t y = iter->y; y < (h); y++)
		{
			Color32* src = mCurrBufferPtr + srcX + (srcY) * (mFBProperties.mStride >> 2);
			Color32* dst = mPrimaryContext->GetSelectedFramebuffer() + iter->x + y * (mPrimaryContext->GetFramebufferProperties().mStride >> 2);
			//UartPrintf("CopyToPrimary: src=%p, dst=%p\n", src,dst);
			memcpy(dst, src, w * 4);
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
		}
	}
	else
	{
		for (int16_t x=startX; x<=endX; x++)
		{
			*ptr++ = currColor.ToColor32();
		}
	}
}

void
GraphicsContextBase::DrawLine(Color32 color, int16_t x0, int16_t y0, int16_t x1, int16_t y1) 
{
	int16_t dummy;
	DrawLine(color, x0, y0, x1, y1, NULL, dummy);
}

void
GraphicsContextBase::DrawLine(Color32 color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
							  Point* points, int16_t& pointsSize)
{
	const int8_t scale = 8;
	FramebufferProperties props = GetSelectedSurface()->GetFramebufferProperties();

	x0 = Clip(x0, props.mGeometry.x, props.mGeometry.x + props.mGeometry.w);
	x1 = Clip(x1, props.mGeometry.x, props.mGeometry.x + props.mGeometry.w);
	y0 = Clip(y0, props.mGeometry.y, props.mGeometry.y + props.mGeometry.h);
	y1 = Clip(y1, props.mGeometry.y, props.mGeometry.y + props.mGeometry.h);
	
	int16_t stepX = x1 - x0;
	int16_t stepY = y1 - y0;
	int16_t steps;
	if ((stepX >= 0) && (stepY >= 0))
		steps = (stepX > stepY) ? stepX : stepY;
	else if ((-stepX > 0) && (stepY > 0))
		steps = (-stepX > stepY) ? -stepX : stepY;
	else if ((stepX > 0) && (-stepY > 0))
		steps = (stepX > -stepY) ? stepX : -stepY;
	else
		steps = (-stepX > -stepY) ? -stepX : -stepY;
	if (steps == 0)
		return;

	int16_t xDelta = ((x1 - x0) << scale) / steps;
	int16_t yDelta = ((y1 - y0) << scale) / steps;
	int32_t x = x0 << scale;
	int32_t y = y0 << scale;
	int16_t savedPoints = 0;
	for (int16_t s=0;s<steps;s++)
	{
		Color32* ptr = mCurrBufferPtr + (x >> scale) + ((y >> scale) * props.mStride/4);
		if (points && savedPoints < pointsSize)
		{
			points->x = (int16_t)(x >> scale);
			points->y = (int16_t)(y >> scale);
			points++;
			savedPoints++;
		}
		*ptr = color;
		x += xDelta;
		y += yDelta;
		//Sleep(0);
	}
	if (points)
		pointsSize = savedPoints;
}

void
GraphicsContextBase::DrawTrapezoid(Color32 color, Point origin, int16_t angle, int16_t innerRadius, 
								   int16_t outerRadius, int16_t arc, bool fill)
{
	// We need to calculate four points
	// We subtract half the arc at the inner radius for point 0
	// We subtract half the arc at the outer radius for point 1
	// We add half the arc at the outer radius for point 2
	// We add half the arc at the inner radius for point 3
	Point p0, p1, p2, p3;
	int16_t p0ArraySize, p1ArraySize, p2ArraySize, p3ArraySize;
	p0ArraySize = p1ArraySize = p2ArraySize = p3ArraySize = 2 * (arc + outerRadius - innerRadius);
	Point* p0Array = new Point[p0ArraySize];
	Point* p1Array = new Point[p1ArraySize];
	Point* p2Array = new Point[p2ArraySize];
	Point* p3Array = new Point[p3ArraySize];
	
	int32_t clipped = (int32_t)Trig::ClipAngle(angle - (arc >> 1));
	p0.x = origin.x + (int16_t)((Trig::mSinInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p0.y = origin.y + (int16_t)((-Trig::mCosInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	p1.x = origin.x + (int16_t)((Trig::mSinInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p1.y = origin.y + (int16_t)((-Trig::mCosInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	// Draw the first segment
	DrawLine(color, p0.x, p0.y, p1.x, p1.y, p0Array, p0ArraySize);

	// Jump to the outer arc
	clipped = (int32_t)Trig::ClipAngle(angle + (arc >> 1));
	p2.x = origin.x + (int16_t)((Trig::mSinInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p2.y = origin.y + (int16_t)((-Trig::mCosInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	// Draw the second segment
	DrawLine(color, p1.x, p1.y, p2.x, p2.y, p1Array, p1ArraySize);
	
	p3.x = origin.x + (int16_t)((Trig::mSinInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p3.y = origin.y + (int16_t)((-Trig::mCosInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	// Draw the third segment
	DrawLine(color, p2.x, p2.y, p3.x, p3.y, p2Array, p2ArraySize);

	// Draw the fourth and last segment back to the beginning
	DrawLine(color, p3.x, p3.y, p0.x, p0.y, p3Array, p3ArraySize);

	if (fill)
	{
		int16_t startAngle = Trig::ClipAngle(angle - (arc >> 2));
		int16_t endAngle   = Trig::ClipAngle(angle + (arc >> 2));
		uint32_t intColor = (uint32_t)*(uint32_t*)&color;
		
		if (startAngle >= 0 && startAngle < 90)
		{
			Point* curr  = p1Array + 1;
			int16_t left = p1ArraySize - 1;
			FloodFillLeft(intColor, curr, left);
		}
		else if (startAngle >= 90 && startAngle < 180)
		{
			Point* curr  = p0Array + 1;
			int16_t left = p0ArraySize - 1;
			FloodFillLeft(intColor, curr, left);
		}
		else if (startAngle >= 180 && startAngle < 270)
		{
			Point* curr  = p0Array + 0;
			int16_t left = p0ArraySize - 0;
			FloodFillLeft(intColor, curr, left);
		}
		else
		{
			Point* curr  = p0Array + 1;
			int16_t left = p0ArraySize - 1;
			FloodFillRight(intColor, curr, left);
		}
		
		if (endAngle >= 0 && endAngle < 90)
		{
			Point* curr = p2Array + 0;
			int16_t left = p2ArraySize - 0;
			FloodFillLeft(intColor, curr, left);
		}
		else if (endAngle >= 90 && endAngle < 179)
		{
			Point* curr = p1Array + 0;
			int16_t left = p1ArraySize - 0;
			FloodFillLeft(intColor, curr, left);
		}
		else if (endAngle >= 179 && endAngle < 270)
		{
			Point* curr = p3Array + 1;
			int16_t left = p3ArraySize - 1;
			FloodFillLeft(intColor, curr, left);
		}
		else
		{
			Point* curr = p1Array + 0;
			int16_t left = p1ArraySize - 0;
			// We can get an artifact when the start angle causes us to
			// fill below the angle
			if (startAngle < 270)
			{
				curr++;
				left--;
			}
			FloodFillRight(intColor, curr, left);
		}
		
		
	}
	delete [] p0Array;
	delete [] p1Array;
	delete [] p2Array;
	delete [] p3Array;
}

void
GraphicsContextBase::FloodFillLeft(uint32_t intColor, Point* start, int16_t arraySize)
{
	Point*  curr = start;
	int16_t left = arraySize;
	const FramebufferProperties& props = GetSelectedSurface()->GetFramebufferProperties();
	Color32* base = GetSelectedSurface()->mCurrBufferPtr;
	int16_t maxFill;
	while (left--)
	{
		int16_t x = curr->x - 1;
		uint32_t* ptr = (uint32_t*)(base + x + curr->y * props.mStride / 4);
		if (*ptr != intColor)
		{
			maxFill = 2000;
			while (*ptr != intColor && maxFill--)
			{
				*ptr-- = intColor;
			}
		}
		curr++;
	}
}

void
GraphicsContextBase::FloodFillRight(uint32_t intColor, Point* start, int16_t arraySize)
{
	Point*  curr = start;
	int16_t left = arraySize;
	const FramebufferProperties& props = GetSelectedSurface()->GetFramebufferProperties();
	Color32* base = GetSelectedSurface()->mCurrBufferPtr;
	int16_t maxFill;
	while (left--)
	{
		int16_t x = curr->x + 1;
		uint32_t* ptr = (uint32_t*)(base + x + curr->y * props.mStride / 4);
		if (*ptr != intColor)
		{
			maxFill = 2000;
			while (*ptr != intColor && maxFill--)
			{
				*ptr++ = intColor;
			}
		}
		curr++;
	}
}

void
GraphicsContextBase::DrawArc(Color32 color, Point origin, int16_t startAngle, int16_t endAngle, int16_t radius)
{
	int16_t x0, y0, x1, y1;
	for (int32_t angle = startAngle; angle < endAngle; angle++)
	{
		int32_t clipped	= Trig::ClipAngle(angle);
		x0 = origin.x + (int16_t)((Trig::mCosInt[clipped] * radius) >> Trig::kTrigShift);
		y0 = origin.y + (int16_t)((Trig::mSinInt[clipped] * radius) >> Trig::kTrigShift);
		clipped	= Trig::ClipAngle(++angle);
		x1 = origin.x + (int16_t)((Trig::mCosInt[clipped] * radius) >> Trig::kTrigShift);
		y1 = origin.y + (int16_t)((Trig::mSinInt[clipped] * radius) >> Trig::kTrigShift);
		DrawLine(color, x0, y0, x1, y1);
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
		memset(ptr, 0, info.bmiHeader.biSizeImage);

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

bool __attribute__((optimize("O0")))
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

bool __attribute__((optimize("O0"))) MailboxWrite(uintptr_t message, enum VideoCoreChannel channel)
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

bool __attribute__((optimize("O0"))) MailboxRead(uintptr_t& message, enum VideoCoreChannel channel)
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
