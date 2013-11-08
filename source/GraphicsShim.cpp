#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "malloc.h"
#include "Trig.h"
#include <cstddef>
#include <list>
#ifdef WIN32
#include "windows.h"
#include "winsock.h"
#include <string>
#else
#include "bcm2835.h"
extern "C"
{
	extern void flush_writeback ( uintptr_t start, uintptr_t end );
}
#endif

#include "GraphicsShim.h"
#include "Region.h"
#include "uart0.h"


// Static vars
GraphicsContextBase*		GraphicsContextBase::mPrimaryContext = NULL;
Color32						GraphicsContextBase::kMagenta(202, 31, 123, eTransparent);
ProfileData					GraphicsContextBase::mProfileData;

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
	mScreenOffset.x = mScreenOffset.y = 0;
	mDirtyRegion		= NULL;
	mGlobalAlpha		= eOpaque;
}

GraphicsContextBase* 
GraphicsContextBase::SetSurfaceSelection(SurfaceSelection selection)
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

Point
GraphicsContextBase::ClientToScreen(const Point& clientPos)
{
	Point ret;
	ret.x = clientPos.x + mScreenOffset.x;
	ret.y = clientPos.y + mScreenOffset.y;
	return ret;
}

Rect
GraphicsContextBase::ClientToScreen(const Rect& clientRect)
{
	Rect ret = clientRect;
	ret.x = clientRect.x + mScreenOffset.x;
	ret.y = clientRect.y + mScreenOffset.y;
	return ret;
}

Point
GraphicsContextBase::ScreenToClient(const Point& screenPos)
{
	Point ret;
	ret.x = screenPos.x - mScreenOffset.x;
	ret.y = screenPos.y - mScreenOffset.y;
	return ret;
}

Rect
GraphicsContextBase::ScreenToClient(const Rect& screenRect)
{
	Rect ret = screenRect;
	ret.x = screenRect.x - mScreenOffset.x;
	ret.y = screenRect.y - mScreenOffset.y;
	return ret;
}


void
GraphicsContextBase::GradientRectangle(Rect location, int16_t angle, std::vector<GradientStop>& stops)
{
	//UartPrintf("GradientRectangle()\n");
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

	size_t stopIndex = 0;
	location.x = Clip(location.x, 0, mFBProperties.mGeometry.w-1);
	location.y = Clip(location.y, 0, mFBProperties.mGeometry.h-1);
	int16_t right  = location.x + location.w - 1;
	int16_t bottom = location.y + location.h - 1;
	right  = Clip(right, 0, mFBProperties.mGeometry.w-1);
	bottom = Clip(bottom, 0, mFBProperties.mGeometry.h-1);

//	UartPrintf("GradientRectangle() : Rect=%d,%d - %d,%d. angle=%d,stops=%d\n",
//			location.x,
//			location.y,
//			right,
//			bottom,
//			(int)angle,
//			(int)stops.size());

	for (int16_t y=location.y; y<=bottom;)
	{
		GradientLine(currColor.ToColor32(), noColorDelta, location.x, right, y);
		
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
GraphicsContextBase::FillRectangle(Rect rect, Color32 argb)
{
	for (int16_t y=rect.y; y < rect.y + rect.h; y++)
	{
		Color32* ptr = mCurrBufferPtr + rect.x + (y * mFBProperties.mStride/4);
		for (int16_t x=0; x < rect.w; x++)
		{
			*ptr++ = argb;
		}
	}
}

void
GraphicsContextBase::PrepareBMP(BMPImage* image)
{
#ifndef WIN32
	if (image == NULL)
		return;
	for (int16_t y = 0; y < image->height; y++)
	{
		Color32* src = &(image->pixelArray) + (image->height - y - 1) * (image->width);
		for (int16_t x = 0; x < image->width; x++)
		{
			src->SwapRGBChannels();
			src++;
		}
	}
#endif
}

void
GraphicsContextBase::DrawBMP(Point& dstLoc, BMPImage* image, bool alphaBlend)
{
	if (image == NULL)
		return;
	for (int16_t y = 0; y < image->height; y++)
	{
		Color32* src = &(image->pixelArray) + (image->height - y - 1) * (image->width);
		Color32* dst = mCurrBufferPtr + dstLoc.x + (y + dstLoc.y) * (mFBProperties.mStride >> 2);
		if (alphaBlend)
		{
			for (int16_t x = 0; x < image->width; x++)
			{
				*dst = src->AlphaBlend(*dst, mGlobalAlpha);
				src++;
				dst++;
			}
		}
		else
		{
			memcpy(dst, src, (image->width) * 4);
		}
	}
}

void
GraphicsContextBase::CopyToPrimary(std::vector<Rect> rects, bool alphaBlend)
{
	// We can't copy to ourselves
	if (!mPrimaryContext)
		return;

	PROFILE_START(mProfileData.mCopyToPrimary)

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
		// NOTE : The rectangles are in client coordinates
		// Account for the screen coordinates and make sure we are within the bounds
		// of our source framebuffer
		Rect srcRect = *iter;
		if (srcRect.x < 0)
		{
			srcRect.w += srcRect.x;
			srcRect.x = 0;
		}
		if (srcRect.y < 0)
		{
			srcRect.h += srcRect.y;
			srcRect.y = 0;
		}
		if (mScreenOffset.x < 0)
		{
			srcRect.x += mScreenOffset.x;
			srcRect.w += mScreenOffset.x;
		}
		else if (mScreenOffset.x >= mFBProperties.mGeometry.w)
		{
//			srcRect.w -= (mScreenOffset.x - mFBProperties.mGeometry.w);
		}
		if (mScreenOffset.y < 0)
		{
			srcRect.y += mScreenOffset.y;
			srcRect.h += mScreenOffset.y;
		}
		else if (mScreenOffset.y >= mFBProperties.mGeometry.h)
		{
//			srcRect.h -= (mScreenOffset.y - mFBProperties.mGeometry.h);
		}
		srcRect.x = Clip(srcRect.x, 0, (mFBProperties.mGeometry.w-1));
		srcRect.y = Clip(srcRect.y, 0, (mFBProperties.mGeometry.h-1));
		srcRect.w = Clip(srcRect.w, 0, mFBProperties.mGeometry.w);
		srcRect.h = Clip(srcRect.h, 0, mFBProperties.mGeometry.h);

		//UartPrintf("CopyToPrimary: srcRect=%d,%d : %d,%d\n", srcRect.x, srcRect.y, srcRect.w, srcRect.h);
		//UartPrintf("CopyToPrimary: iter=%d,%d : %d,%d\n", iter->x, iter->y, iter->w, iter->h);

		Rect dstRect = *iter;
		FramebufferProperties primaryFbProps = mPrimaryContext->GetFramebufferProperties();

		if (dstRect.x < 0)
		{
			dstRect.w += dstRect.x;
			dstRect.x = 0;
		}
		if (dstRect.y < 0)
		{
			dstRect.h += dstRect.y;
			dstRect.y = 0;
		}

		if (mScreenOffset.x < 0)
		{
			dstRect.w += mScreenOffset.x;
		}
		else
		{
			dstRect.x += mScreenOffset.x;
			if (mScreenOffset.x >= primaryFbProps.mGeometry.w)
				dstRect.w -= mScreenOffset.x;
		}
		if (mScreenOffset.y < 0)
		{
			dstRect.h += mScreenOffset.y;
		}
		else
		{
			dstRect.y += mScreenOffset.y;
			if (mScreenOffset.y >= primaryFbProps.mGeometry.h)
				dstRect.h -= mScreenOffset.y;
		}
		dstRect.x = Clip(dstRect.x, 0, (primaryFbProps.mGeometry.w-1));
		dstRect.y = Clip(dstRect.y, 0, (primaryFbProps.mGeometry.h-1));
		dstRect.w = Clip(dstRect.w, 0, primaryFbProps.mGeometry.w);
		dstRect.h = Clip(dstRect.h, 0, primaryFbProps.mGeometry.h);

		//UartPrintf("CopyToPrimary: dstRect=%d,%d : %d,%d\n", dstRect.x, dstRect.y, dstRect.w, dstRect.h);

#ifdef WIN32
		for (int16_t y = 0; y < srcRect.h; y++)
		{
			Color32* src = mCurrBufferPtr + srcRect.x + (y + srcRect.y) * (mFBProperties.mStride >> 2);
			Color32* dst = mPrimaryContext->GetSelectedFramebuffer() + dstRect.x + (y + dstRect.y) * (mPrimaryContext->GetFramebufferProperties().mStride >> 2);
			//UartPrintf("CopyToPrimary: src=%p, dst=%p\n", src,dst);
			if (alphaBlend && (mGlobalAlpha != eOpaque))
			{
				for (int16_t x = 0; x < srcRect.w; x++)
				{
					*dst = src->AlphaBlend(*dst, mGlobalAlpha);
					src++;
					dst++;
					Sleep(0);
				}
			}
			else if (alphaBlend)
			{
				for (int16_t x = 0; x < srcRect.w; x++)
				{
					*dst = src->AlphaBlend(*dst);
					src++;
					dst++;
					Sleep(0);
				}
			}
			else
			{
				memcpy(dst, src, (srcRect.w) * 4);
			}
			if ((rand() % 1000) < 50)
				Sleep(1);
		}
#else
		if (alphaBlend)
		{
			for (int16_t y = 0; y < srcRect.h; y++)
			{
				Color32* src = mCurrBufferPtr + srcRect.x + (y + srcRect.y) * (mFBProperties.mStride >> 2);
				Color32* dst = mPrimaryContext->GetSelectedFramebuffer() + dstRect.x + (y + dstRect.y) * (mPrimaryContext->GetFramebufferProperties().mStride >> 2);
				for (int16_t x = 0; x < srcRect.w; x++)
				{
					*dst = src->AlphaBlend(*dst, mGlobalAlpha);
					src++;
					dst++;
				}
			}

		}
		else
		{
			// Use hardware DMA transfer
			Color32* src = mCurrBufferPtr;
			Color32* dst = mPrimaryContext->GetSelectedFramebuffer();
			Rect newDstRect;
			newDstRect.x = dstRect.x;
			newDstRect.y = dstRect.y;
			newDstRect.w = srcRect.w;
			newDstRect.h = srcRect.h;
			dmaBitBlt(dst, newDstRect, mPrimaryContext->GetFramebufferProperties().mStride, src, srcRect, mFBProperties.mStride);
		}
#endif
	}
	PROFILE_STOP(mProfileData.mCopyToPrimary)
}

void
GraphicsContextBase::CopyBackToFront(Rect& rect)
{
	PROFILE_START(mProfileData.mCopyBackToFront)
	//UartPrintf("CopyBackToFront: src=%p, dst=%p\n", mBackBufferPtr, mFrontBufferPtr);
#ifdef WIN32
	for (int16_t y = 0; y < mFBProperties.mGeometry.h; y++)
	{
		Color32* src = mBackBufferPtr + y * (mFBProperties.mStride >> 2);
		Color32* dst = mFrontBufferPtr + y * (mFBProperties.mStride >> 2);
		memcpy(dst, src, mFBProperties.mStride);
		if ((rand() % 1000) < 10)
			Sleep(1);
	}
#else
	for (int16_t y = rect.y; y < (rect.y + rect.h); y++)
	{
		Color32* src = mBackBufferPtr + rect.x + y * (mFBProperties.mStride >> 2);
		Color32* dst = mFrontBufferPtr + rect.x +  y * (mFBProperties.mStride >> 2);
		for (int16_t x = 0; x < rect.w; x++)
		{
			*dst++ = *src++;
		}
	}
#endif
	PROFILE_STOP(mProfileData.mCopyBackToFront)
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
		startX	+= mScreenOffset.x;
		endX	+= mScreenOffset.x;
		y		+= mScreenOffset.y;
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
GraphicsContextBase::DrawLine(Color32 color, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
							  AntiAliasLineMode aaMode) 
{
	int16_t dummy;
	DrawLine(color, x0, y0, x1, y1, NULL, dummy, aaMode);
}

void
GraphicsContextBase::DrawLine(Color32 color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
							  Point* points, int16_t& pointsSize,
							  AntiAliasLineMode aaMode)
{
	PROFILE_START(mProfileData.mDrawLine)
	const int8_t scale = 8;
	const int32_t divisor = 1 << scale;
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
	{
		PROFILE_STOP(mProfileData.mDrawLine)
		return;
	}

	int16_t xDelta = ((x1 - x0) << scale) / steps;
	int16_t yDelta = ((y1 - y0) << scale) / steps;
	int32_t x = x0 << scale;
	int32_t y = y0 << scale;
	int16_t savedPoints = 0;
	for (int16_t s=0;s<steps;s++)
	{
		Color32* ptr = mCurrBufferPtr + ((x + 128) >> scale) + (((y + 128) >> scale) * props.mStride/4);
		if (points && savedPoints < pointsSize)
		{
			points->x = (int16_t)((x + 128) >> scale);
			points->y = (int16_t)((y + 128) >> scale);
			points++;
			savedPoints++;
		}
		if (mAntiAlias && aaMode != eAntiAliasLineModeNone)
		{
			int16_t errorX = (x + 128) % divisor;
			int16_t errorY = (y + 128) % divisor;
			int16_t error = (errorX + errorY) / 2;
			if (aaMode == eAntiAliasLineModeAlpha1)
				color.a = Clip(255-error, 0, 255);
			else if (aaMode == eAntiAliasLineModeAlpha2)
				color.a = Clip(error, 0, 255);
		}
		// If drawing directly to the screen, we need to alpha blend
		if (mSurfaceSelection == ePrimaryFront || mSurfaceSelection == ePrimaryBack)
		{
			*ptr = color;//.AlphaBlend(*ptr);	// This breaks fill due to the boundary not being the proper color ;(
		}
		else
		{
			*ptr = color;
		}
		x += xDelta;
		y += yDelta;
	}
	if (points)
		pointsSize = savedPoints;
#ifdef WIN32
	if ((rand() % 1000) < 100)
		Sleep(1);
#endif
	PROFILE_STOP(mProfileData.mDrawLine)
}

void
GraphicsContextBase::DrawTrapezoid(Color32 color, Point origin, int32_t angleWide, int16_t innerRadius, 
								   int16_t outerRadius, int32_t startArcWide, int32_t endArcWide, bool fill,
								   AntiAliasEdges aaEdges)
{
	PROFILE_START(mProfileData.mDrawTrapezoid)

	int32_t clipped = (int32_t)Trig::ClipWideDegree(angleWide);
	bool aaMode1 = ((clipped >= 0) && (clipped < 540)) ? true : 
		((clipped > 1260) && (clipped < 1440)) ? true : false;
	bool aaMode2 = ((clipped >= 0) && (clipped < 180)) ? true : 
		((clipped > 900) && (clipped < 1440)) ? true : false;

	// We need to calculate four points
	// We subtract half the arc at the inner radius for point 0
	// We subtract half the arc at the outer radius for point 1
	// We add half the arc at the outer radius for point 2
	// We add half the arc at the inner radius for point 3
	Point p0, p1, p2, p3;
	int16_t p0ArraySize, p1ArraySize, p2ArraySize, p3ArraySize;
	p0ArraySize = p1ArraySize = p2ArraySize = p3ArraySize = (int16_t)((startArcWide + endArcWide) >> 3) + 2 * (outerRadius - innerRadius);

//	UartPrintf("DrawTrapezoid ArraySize=%d, startArcWide=%d, endArcWide=%d, outerRadius=%d, innerRadius=%d\n",
//			p0ArraySize, startArcWide, endArcWide, outerRadius, innerRadius);

	Point* p0Array = NULL;
	Point* p1Array = NULL;
	Point* p2Array = NULL;
	Point* p3Array = NULL;

	try
	{
		p0Array = new Point[p0ArraySize];
		p1Array = new Point[p1ArraySize];
		p2Array = new Point[p2ArraySize];
		p3Array = new Point[p3ArraySize];
	}
	catch (...)
	{
		UartPrintf("new() failed in DrawTrapezoid. ArraySize=%d", p0ArraySize);
		return;
	}
	
//	UartPrintf("p0Array=%p, p1Array=%p, p02rray=%p, p3Array=%p\n", p0Array, p1Array, p2Array, p3Array);

	clipped = (int32_t)Trig::ClipWideDegree(angleWide - (startArcWide >> 1));
	p0.x = origin.x + (int16_t)((Trig::mSinWideInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p0.y = origin.y + (int16_t)((-Trig::mCosWideInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	clipped = (int32_t)Trig::ClipWideDegree(angleWide - (endArcWide >> 1));
	p1.x = origin.x + (int16_t)((Trig::mSinWideInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p1.y = origin.y + (int16_t)((-Trig::mCosWideInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	// Draw the first segment
	DrawLine(color, p0.x, p0.y, p1.x, p1.y, p0Array, p0ArraySize, 
		(aaEdges & eAntiAliasS0) ? (aaMode1 ? eAntiAliasLineModeAlpha1 : eAntiAliasLineModeAlpha2) : eAntiAliasLineModeNone);

	// Jump to the outer arc
	clipped = (int32_t)Trig::ClipWideDegree(angleWide + (endArcWide >> 1));
	p2.x = origin.x + (int16_t)((Trig::mSinWideInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p2.y = origin.y + (int16_t)((-Trig::mCosWideInt[clipped] * outerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	// Draw the second segment
	DrawLine(color, p1.x, p1.y, p2.x, p2.y, p1Array, p1ArraySize,
		(aaEdges & eAntiAliasS1) ? (aaMode2 ? eAntiAliasLineModeAlpha1 : eAntiAliasLineModeAlpha2) : eAntiAliasLineModeNone);
	
	clipped = (int32_t)Trig::ClipWideDegree(angleWide + (startArcWide >> 1));
	p3.x = origin.x + (int16_t)((Trig::mSinWideInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	p3.y = origin.y + (int16_t)((-Trig::mCosWideInt[clipped] * innerRadius - (Trig::kTrigScale >> 2)) >> Trig::kTrigShift);
	
	// Draw the third segment
	DrawLine(color, p2.x, p2.y, p3.x, p3.y, p2Array, p2ArraySize,
		(aaEdges & eAntiAliasS2) ? (aaMode1 ? eAntiAliasLineModeAlpha2 : eAntiAliasLineModeAlpha1) : eAntiAliasLineModeNone);

	// Draw the fourth and last segment back to the beginning
	DrawLine(color, p3.x, p3.y, p0.x, p0.y, p3Array, p3ArraySize,
		(aaEdges & eAntiAliasS3) ? (aaMode2 ? eAntiAliasLineModeAlpha2 : eAntiAliasLineModeAlpha1) : eAntiAliasLineModeNone);

	// Are we tracking dirty rects?
	if (mDirtyRegion)
	{
		// Calculate a bounding box for the whole thing
		Rect rect ALIGN;
		int32_t clipped = (int32_t)Trig::ClipWideDegree(angleWide) >> 2;
		if (clipped < 90)
		{
			rect.x = p0.x;
			rect.y = p1.y;
			rect.w = p2.x - p0.x + 1;
			rect.h = p3.y - p1.y + 1;
		}
		else if (clipped < 180)
		{
			rect.x = p3.x;
			rect.y = p0.y;
			rect.w = p1.x - p3.x + 1;
			rect.h = p2.y - p0.y + 1;
		}
		else if (clipped < 270)
		{
			rect.x = p2.x;
			rect.y = p3.y;
			rect.w = p0.x - p2.x + 1;
			rect.h = p1.y - p3.y + 1;
		}
		else
		{
			rect.x = p1.x;
			rect.y = p2.y;
			rect.w = p3.x - p1.x + 1;
			rect.h = p0.y - p2.y + 1;
		}
		int16_t length = outerRadius - innerRadius;
//		if (length > 0)
//		{
//			//int16_t thresh = 4 * maxEnd;	// If the box is wider/taller than this, break it up
//			//int16_t wCuts = rect.w / thresh;
//			//int16_t hCuts = rect.h / thresh;
//			//if (wCuts > 1 && hCuts > 1)
//			//{
//			//}
//			int16_t maxEnd = p1ArraySize > p3ArraySize ? p1ArraySize : p3ArraySize;
//			int16_t minSize = 16;
//			int16_t wCuts = rect.w / minSize;
//			int16_t hCuts = rect.h / minSize;
//			int16_t cuts  = wCuts > hCuts ? wCuts : hCuts;
//			if (wCuts < 1 || hCuts < 1)
//			{
//				mDirtyRegion->AddRect(rect);
//			}
//			else
//			{
//				cuts = cuts > 3 ? 3 : cuts;
//				Rect cutRect = rect;
//				cutRect.w = rect.w / (cuts);
//				cutRect.h = rect.h / (cuts);
//				int16_t xInc = cutRect.w;
//				int16_t yInc = cutRect.h;
//				if (clipped < 90 || (clipped > 180 && clipped < 270))
//				{
//					cutRect.y = (rect.y + rect.h) - cutRect.h;
//					yInc = -cutRect.h;
//				}
//				cutRect.w += cutRect.w/2;
//				cutRect.h += cutRect.h/2;
//				for (int16_t cut=0; cut<cuts; cut++)
//				{
//					mDirtyRegion->AddRect(cutRect);
//					cutRect.x += xInc;
//					cutRect.y += yInc;
//				}
//			}
//		}
//		else
			mDirtyRegion->AddRect(rect);
	}

	if (fill)
	{
		uint32_t colorInt = *((uint32_t*)&color);
#if FLOOD
		int16_t x = (p0.x + p1.x + p2.x + p3.x) / 4;
		int16_t y = (p0.y + p1.y + p2.y + p3.y) / 4;
		FloodFill(x, y, colorInt, colorInt);
#else
		if (p0.x <= p1.x)
		{
			if (p0.y > p1.y)
			{
				FloodFillRight(colorInt, p0Array, p0ArraySize);
			}
			else 
			{
				//FloodFillLeft(colorInt, p0Array+1, p0ArraySize-1);
			}
			if (p3.y > p0.y)
			{
				if (p3.y < p2.y)
					FloodFillRight(colorInt, p3Array+0, p3ArraySize-0);
				else
					FloodFillRight(colorInt, p3Array+1, p3ArraySize-1);
			}
			else
			{
				//FloodFillLeft(colorInt, p3Array+0, p3ArraySize-0);
			}
			if (p2.y > p3.y)
			{
				FloodFillRight(colorInt, p2Array+1, p2ArraySize-1);
			}
			else
			{
				//FloodFillLeft(colorInt, p3Array+1, p3ArraySize-1);
			}
		}
		else
		{
			if (p3.y > p2.y)
			{
				FloodFillLeft(colorInt, p2Array+1, p2ArraySize-1);
			}
			else 
			{
				//FloodFillLeft(colorInt, p0Array+1, p0ArraySize-1);
			}
			if (p3.y < p0.y)
			{
				if (p3.y < p2.y)
					FloodFillLeft(colorInt, p3Array+1, p3ArraySize-1);
				else
					FloodFillLeft(colorInt, p3Array+0, p3ArraySize-0);
			}
			else
			{
				//FloodFillLeft(colorInt, p3Array+0, p3ArraySize-0);
			}
			if (p0.y < p1.y)
			{
				FloodFillLeft(colorInt, p0Array+0, p0ArraySize-0);
			}
			else
			{
				//FloodFillLeft(colorInt, p3Array+1, p3ArraySize-1);
			}
		}
#endif
	}

	delete [] p0Array;
	delete [] p1Array;
	delete [] p2Array;
	delete [] p3Array;

	PROFILE_STOP(mProfileData.mDrawTrapezoid)

}

void
GraphicsContextBase::FloodFill(int16_t x, int16_t y, uint32_t borderColor, uint32_t fillColor)
{
	PROFILE_START(mProfileData.mFloodFill)

		std::list<Point> nodes;
	const FramebufferProperties& props = GetSelectedSurface()->GetFramebufferProperties();
	uint32_t* ptr = (uint32_t*)GetSelectedSurface()->mCurrBufferPtr + x + (y * props.mStride / 4);

	if ((x < props.mGeometry.x) || (x >= (props.mGeometry.x + props.mGeometry.w)) || 
		(y < props.mGeometry.y) || (y >= (props.mGeometry.y + props.mGeometry.h)))
	{
		PROFILE_STOP(mProfileData.mFloodFill)
		return;
	}
	if (*ptr == borderColor)
	{
		PROFILE_STOP(mProfileData.mFloodFill)
		return;
	}

	Point point;
	Point pointAdd;
	point.x = x;
	point.y = y;
	nodes.push_back(point);
	while (!nodes.empty())// && nodes.size() < 100)
	{
		point = nodes.back();
		nodes.pop_back();
		ptr = (uint32_t*)GetSelectedSurface()->mCurrBufferPtr + point.x + (point.y * props.mStride / 4);
		if (*ptr != borderColor)
		{
			// Sweep left and right  to find the horizontal edges of the fill area
			int16_t xMin = point.x;
			int16_t xMax = point.x;
			while (xMin >= props.mGeometry.x)
			{
				if (*(--ptr) == borderColor)
					break;
				xMin--;
			}
			ptr = (uint32_t*)GetSelectedSurface()->mCurrBufferPtr + point.x + (point.y * props.mStride / 4);
			while (xMax < (props.mGeometry.x + props.mGeometry.w))
			{
				if (*(++ptr) == borderColor)
					break;
				xMax++;
			}
			ptr = (uint32_t*)GetSelectedSurface()->mCurrBufferPtr + xMin + (point.y * props.mStride / 4);
			for (int16_t currX=(xMin+0) ; currX <= xMax ; currX++)
			{
				*ptr = fillColor;
				if (*(ptr - (props.mStride / 4)) != borderColor)
				{
					pointAdd.x = currX;
					pointAdd.y = point.y - 1;
					nodes.push_back(pointAdd);
				}
				if (*(ptr + (props.mStride / 4)) != borderColor)
				{
					pointAdd.x = currX;
					pointAdd.y = point.y + 1;
					nodes.push_back(pointAdd);
				}
				ptr++;
			}
		}
	}
	PROFILE_STOP(mProfileData.mFloodFill)
}

void
GraphicsContextBase::FloodFillLeft(uint32_t intColor, Point* start, int16_t arraySize)
{
	PROFILE_START(mProfileData.mFloodFill)

	Point*  curr = start;
	int16_t left = arraySize;
	const FramebufferProperties& props = GetSelectedSurface()->GetFramebufferProperties();
	Color32* base = GetSelectedSurface()->mCurrBufferPtr;
	int16_t maxFill;
	while (left--)
	{
		int16_t x = curr->x - 1;
		uint32_t* ptr = (uint32_t*)(base + x + curr->y * props.mStride / 4);
		maxFill = 200;
		while ((*ptr & 0x00ffffff) != (intColor & 0x00ffffff) && maxFill--)
		{
			*ptr-- = intColor;
		}
		curr++;
	}

#ifdef WIN32
	if ((rand() % 1000) < 100)
		Sleep(1);
#endif

	PROFILE_STOP(mProfileData.mFloodFill)
}

void
GraphicsContextBase::FloodFillRight(uint32_t intColor, Point* start, int16_t arraySize)
{
	PROFILE_START(mProfileData.mFloodFill)
	
	Point*  curr = start;
	int16_t left = arraySize;
	const FramebufferProperties& props = GetSelectedSurface()->GetFramebufferProperties();
	Color32* base = GetSelectedSurface()->mCurrBufferPtr;
	int16_t maxFill;
	while (left--)
	{
		int16_t x = curr->x + 1;
		uint32_t* ptr = (uint32_t*)(base + x + curr->y * props.mStride / 4);
		maxFill = 200;
		while ((*ptr & 0x00ffffff) != (intColor & 0x00ffffff) && maxFill--)
		{
			*ptr++ = intColor;
		}
		curr++;
	}

#ifdef WIN32
	if ((rand() % 1000) < 100)
		Sleep(1);
#endif
	
	PROFILE_STOP(mProfileData.mFloodFill)
}

void
GraphicsContextBase::DrawArc(Color32 color, Point origin, int16_t startWideAngle, int16_t endWideAngle, int16_t radius)
{
	int16_t x0, y0, x1, y1;
	for (int32_t angle = startWideAngle; angle < endWideAngle; angle++)
	{
		int32_t clipped	= Trig::ClipWideDegree(angle);
		x0 = origin.x + (int16_t)((Trig::mCosWideInt[clipped] * radius) >> Trig::kTrigShift);
		y0 = origin.y + (int16_t)((Trig::mSinWideInt[clipped] * radius) >> Trig::kTrigShift);
		clipped	= Trig::ClipWideDegree(++angle);
		x1 = origin.x + (int16_t)((Trig::mCosWideInt[clipped] * radius) >> Trig::kTrigShift);
		y1 = origin.y + (int16_t)((Trig::mSinWideInt[clipped] * radius) >> Trig::kTrigShift);
		DrawLine(color, x0, y0, x1, y1);
	}
}

void
GraphicsContextBase::DrawCircle(Color32 color, Point origin, int16_t radius, bool fill)
{
	int16_t x0, y0, x1, y1;
	int32_t angleInc = 32;
	for (int32_t angle = 0; angle < 1440; angle += angleInc)
	{
		int32_t clipped	= Trig::ClipWideDegree(angle);
		x0 = origin.x + (int16_t)((Trig::mCosWideInt[clipped] * radius) >> Trig::kTrigShift);
		y0 = origin.y + (int16_t)((Trig::mSinWideInt[clipped] * radius) >> Trig::kTrigShift);
		clipped	= Trig::ClipWideDegree(angle + angleInc);
		x1 = origin.x + (int16_t)((Trig::mCosWideInt[clipped] * radius) >> Trig::kTrigShift);
		y1 = origin.y + (int16_t)((Trig::mSinWideInt[clipped] * radius) >> Trig::kTrigShift);
		DrawLine(color, x0, y0, x1, y1);
	}
	if (fill)
	{
		uint32_t uColor = *((uint32_t*)&color);
		FloodFill(origin.x, origin.y, uColor, uColor);
	}
}

int16_t	
GraphicsContextBase::GetTextDrawnLength(FontDatabaseFile* font, char* text)
{
	std::string textString(text);
	return GetTextDrawnLength(font, textString);
}

int16_t	
GraphicsContextBase::GetTextDrawnLength(FontDatabaseFile* font, std::string& text)
{
	int16_t length = 0;
	for (size_t i = 0; i < (uint16_t)text.length(); i++)
	{
		uint8_t offset = text[i] - font->startChar;
		length += (int16_t)font->widthArray[offset];
	}
	return length;
}

void
GraphicsContextBase::DrawText(FontDatabaseFile* font, char* text, Point& loc, Color32& colorIn, bool alphaBlend, TextAlignment align)
{
	std::string textString(text);
	DrawText(font, textString, loc, colorIn, alphaBlend, align);
}

void
GraphicsContextBase::DrawText(FontDatabaseFile* font, std::string& text, Point& loc, Color32& colorIn, bool alphaBlend, TextAlignment align)
{
	//UartPrintf("font=%p, fileSize=%d, cols=%d, width=%p, height=%p\n", font, font->fileSize, font->columns, font->cellWidth, font->cellHeight);

	if (!font)
		return;

	PROFILE_START(mProfileData.mDrawText)

	// Calculate the extents of the text we are about to draw
	int16_t  width		= GetTextDrawnLength(font, text);
	Point textLoc		= loc;
	textLoc.y += font->fontDescent;

	if (align == eAlignCenter)
		textLoc.x -= width / 2;
	else if (align == eAlignRight)
		textLoc.x -= width;
	
	Color32 color = colorIn;
	uint32_t fontStride = font->cellWidth * font->columns;
	int16_t  dstX		= textLoc.x;

	for (size_t i = 0; i < (uint16_t)text.length(); i++)
	{
		uint8_t offset = text[i] - font->startChar;
		uint8_t* currChar = &font->alphaArray;								// start from the beginning of the array
		currChar += (offset % font->columns) * font->cellWidth;				// adjust for the column
		currChar += (offset/font->columns) * font->cellHeight * fontStride;	// adjust for the row

		//UartPrintf("offset=%d, currChar=%p\n", offset, currChar);
		for (int16_t y = 0; y < (uint16_t)font->cellHeight; y++)
		{
			uint8_t* src = currChar + y * fontStride;
			Color32* dst = mCurrBufferPtr + dstX + (textLoc.y + y) * (mFBProperties.mStride >> 2);
			for (int16_t x = 0; x < font->cellWidth; x++)
			{
				if (*src)
				{
					color.a = *src;
					if (alphaBlend)
						*dst = color.AlphaBlend(*dst);
					else
						*dst = color;
				}
				src++;
				dst++;
			}
	#ifdef WIN32
			if ((y % 8) == 0)
				Sleep(1);
	#endif
		}
		dstX += font->widthArray[offset];
	}

	PROFILE_STOP(mProfileData.mDrawText)
}

void 
GraphicsContextBase::DrawEmboss(Color32 colorMax, Point origin, int16_t innerRadius, int16_t outerRadius, 
					  int32_t startAngleWide, int32_t endAngleWide, int32_t peakAngleWide, int32_t stepAngleWide)
{
	Color32 color;
	color.a = eOpaque;
	int32_t angleInc = startAngleWide < endAngleWide ? stepAngleWide : -stepAngleWide;
	for (int32_t angleWide = startAngleWide; (stepAngleWide > 0 ? (angleWide <= endAngleWide) : (angleWide >= endAngleWide)); angleWide += angleInc)
	{
		// Simple hack that keeps color ranges from overflowing into the next region
		if (angleInc > 0)
		{
			if ((angleWide + stepAngleWide) > (endAngleWide + 1))
				stepAngleWide = 4;
		}
		int32_t offset = Trig::ClipWideDegree(angleWide - peakAngleWide);
		int32_t shift  = offset < 720 ? (720 - offset) : (offset - 720);
		color.r = (uint8_t)(colorMax.r * shift / 720);
		color.g = (uint8_t)(colorMax.g * shift / 720);
		color.b = (uint8_t)(colorMax.b * shift / 720);

		DrawTrapezoid(color, origin, Trig::ClipWideDegree(angleWide + stepAngleWide/2), innerRadius, outerRadius, 
			stepAngleWide, stepAngleWide, true,
			eAntiAliasS1S3	// Only anti-alias side 1 and 3 (top/bottom). If we do it on the sides, we will see a boundary
		);
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
	//RECT winRect;
	//HBRUSH brush;
	//winRect.left	= rect.x;
	//winRect.right	= rect.x + rect.w;
	//winRect.top		= rect.y;
	//winRect.bottom	= rect.y + rect.h;
	//brush = CreateSolidBrush(RGB(argb.r, argb.g, argb.b));
	//FillRect(mDC, &winRect, brush);
	//DeleteObject(brush);
	GraphicsContextBase::FillRectangle(rect, argb);
}

bool
GraphicsContextWin::CreateFontDatabase(std::string fontName, int16_t height)
{
	bool res = false;
	FILE* fp = NULL;

	do 
	{
		Color32 black;
		black.a = eOpaque;
		black.r = black.g = black.b = 0;

		int16_t columns	= 32;
		int16_t rows		= 3;
		int16_t colWidth	= height;		// assume a square font element
		int16_t rowHeight	= height;

		// Allocate a big bitmap that should have no problem holding our font
		mFBProperties.mBitsPerPixel = 32;
		mFBProperties.mDoubleBuffer = false;
		mFBProperties.mGeometry.x = mFBProperties.mGeometry.y = 0;
		mFBProperties.mGeometry.w = columns * colWidth;
		mFBProperties.mGeometry.h = rows    * rowHeight;

		res = AllocateWindowsBitmap(mFBProperties, mDC, mBitmapInfo, mBitmap, mCurrBufferPtr);

		if (!res)
			break;

		TEXTMETRIC metric;
		HFONT font = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Eras Demi ITC");
		
		SelectObject(mDC, font);
		SetTextColor(mDC, RGB(255,255,255));
		SetBkMode(mDC,TRANSPARENT);

		BOOL winRes = GetTextMetrics(mDC, &metric);

		FillRectangle(mFBProperties.mGeometry, black);

		Rect maxSize(0,0,0,0);
		RECT winRect;
		SetRect(&winRect, 0, 0, colWidth, rowHeight);

		// Go through the ASCII table, and render each character
		char startChar = ' ';
		char endChar = '~';
		uint8_t widths[256];

		for (char character = startChar; character <= endChar; character++)
		{
			DrawTextA(mDC, &character, 1, &winRect, DT_LEFT | DT_BOTTOM);

			SIZE size;
			GetTextExtentPointA(mDC, &character, 1, &size);

			// Track each character's width
			widths[character - startChar] = size.cx;

			if ((character % 32) == 31)
			{
				if (winRect.right > maxSize.w)
					maxSize.w = (int16_t)winRect.right;

				winRect.top		+= rowHeight;
				winRect.bottom	+= rowHeight;
				winRect.left	= 0;
				winRect.right	= colWidth;
			}
			else
			{
				winRect.left	+= colWidth;
				winRect.right	+= colWidth;
			}
		}
		maxSize.h = (int16_t)winRect.bottom;

		// Now, convert the non-white pixels to their alpha blended equivalents
		for (int16_t y = 0; y < mFBProperties.mGeometry.h; y++)
		{
			for (int16_t x = 0; x < mFBProperties.mGeometry.w; x++)
			{
				Color32* ptr = mCurrBufferPtr + x + (y * mFBProperties.mStride / 4);
				int16_t luma = (ptr->r + ptr->g + ptr->b) / 3;
				luma = Clip(luma, 0, 255);
				ptr->r = ptr->g = ptr->b = (uint8_t)(luma == 0 ? 0 : 255);
				ptr->a = (uint8_t)luma;
			}
		}

		// Finally, write the output file
		std::string filename = fontName + ".bin";
		if (fopen_s(&fp, filename.c_str(), "wb"))
			break;

		// Create our database entry
		size_t dbSize = sizeof(FontDatabaseFile) + maxSize.w * maxSize.h;
		FontDatabaseFile* db = (FontDatabaseFile*)malloc(dbSize);
		memset(db, 0, sizeof(FontDatabaseFile));
		db->fileSize	= dbSize;
		memcpy(&db->fontName, fontName.c_str(), fontName.length());
		db->fontHeight	= (uint8_t)metric.tmHeight;
		db->fontAscent	= (uint8_t)metric.tmAscent;
		db->fontDescent	= (uint8_t)metric.tmDescent;
		db->startChar	= (uint8_t)startChar;
		db->endChar		= (uint8_t)endChar;
		db->columns		= (uint8_t)columns;
		db->rows		= (uint8_t)rows;
		db->cellWidth	= (uint8_t)colWidth;
		db->cellHeight	= (uint8_t)rowHeight;
		memcpy(&db->widthArray, widths, (endChar - startChar) * sizeof(char));

		// Write out the alpha values
		uint8_t* dst = &db->alphaArray;
		for (int16_t y = 0; y < maxSize.h; y++)
		{
			for (int16_t x = 0; x < maxSize.w; x++)
			{
				Color32* src = mCurrBufferPtr + x + (y * maxSize.w);
				*dst++ = src->a;
			}
		}

		if (fwrite(db, 1, dbSize, fp) != dbSize)
		{
			break;
		}
		free(db);

		res = true;
	} while (false);

	if (fp)
		fclose(fp);

	return res;
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

void __attribute__((optimize("O0")))
GraphicsContextPi::WaitForVSync()
{
	// There is a hack to get a vsync interrupt for the Pi using fake_vsync_isr=1 in config.txt
	// This will set the SMI interrupt when vsync happens.
	// The SMI interrupt is GPU interrupt 48
	// First, clear the SMI interrupt
	uint32_t* smi        = (uint32_t*)0x20600000;
	*smi = 0;

//	uint32_t* intBase = (uint32_t*)(BCM2835_INTERRUPT_BASE);
//	for (int i=0;i<9;i++)
//	{
//		UartPrintf("interrupt register @ address=%p. Val=%p\n", intBase+i, *(intBase+i));
//	}
	uint32_t* intPending = (uint32_t*)(BCM2835_INTERRUPT_BASE + BCM2835_IRQ_PENDING_2);
	//UartPrintf("Waiting for vsync interrupt @ address=%p. Val=%p. smi=%p, *smi=%p\n", intPending, *intPending, smi, *smi);
	while ((*intPending & (1 << (48-32))) == 0)
	{
		//UartPrintf("Waiting for vsync interrupt @ address=%p. Val=%p\n", intPending, *intPending);
	}

}

bool
GraphicsContextPi::AllocatePrimaryFramebuffer(FramebufferProperties& properties)
{
	mPrimaryContext = this;
	mFBProperties = properties;

	UartPrintf("AllocatePrimaryFramebuffer() this=%p\n", this);

	// Set the vsync interrupt while we are here
	// There is a hack to do this for the Pi using fake_vsync_isr=1 in config.txt
	// This will set the SMI interrupt when vsync happens.
	// The SMI interrupt is GPU interrupt 48
	uint32_t* intEn = (uint32_t*)(BCM2835_INTERRUPT_BASE + BCM2835_IRQ_EN_IRQ_2);
	*intEn = 1 << (48-32);

	UartPrintf("Enabling vsync interrupt @ address=%p. Val=%p\n", intEn, *intEn);

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
			mFrontBufferPtr = (Color32*)((uintptr_t)fbDesc.buffer & (uintptr_t)~kVideoCoreMailboxDisableCache);
			mCurrBufferPtr  = mFrontBufferPtr;
			if (properties.mDoubleBuffer)
			{
				// Double-buffer was requested, so allocate the back buffer now
				if (!AllocateFramebuffer(properties))
				{
					UartPrintf("Allocate primary rear framebuffer failed\n");
					res = false;
					break;
				}
				// AllocateFramebuffer will set the latest buffer to mFrontBuffer, so undo that operation
				mBackBufferPtr = mFrontBufferPtr;
				mFrontBufferPtr = (Color32*)((uintptr_t)fbDesc.buffer & (uintptr_t)~kVideoCoreMailboxDisableCache);
			}
		}
//		UartPrintf("Allocate primary framebuffer : front = %x\n", mFrontBufferPtr);
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

//		UartPrintf("Allocate framebuffer w=%d, h=%d, bpp=%d, stride=%d\n", properties.mGeometry.w, properties.mGeometry.h,
//			properties.mBitsPerPixel, properties.mStride);
		size_t bufferSize = properties.mStride * properties.mGeometry.h;
		mFrontBufferPtr = new Color32[bufferSize/4];
		res = mFrontBufferPtr != NULL;
//		UartPrintf("Allocate framebuffer size=%d, res=%d: front = %x\n", bufferSize, (int)res, mFrontBufferPtr);
		mCurrBufferPtr  = mFrontBufferPtr;

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
//	UartPrintf("FillRectangle: rect=%d,%d : %d,%d. stride=%d\n", rect.x, rect.y, rect.w, rect.h, mFBProperties.mStride);
	GraphicsContextBase::FillRectangle(rect, argb);
}

void
GraphicsContextPi::CopyBackToFront(Rect& rect)
{
	PROFILE_START(mProfileData.mCopyBackToFront)

	Color32* src = mBackBufferPtr;
	Color32* dst = mFrontBufferPtr;
	dmaBitBlt(dst, rect, mFBProperties.mStride, src, rect, mFBProperties.mStride);

	PROFILE_STOP(mProfileData.mCopyBackToFront)
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
		//UartPrintf("Success, framebuffer address is %p\n", fbDesc.buffer);
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
