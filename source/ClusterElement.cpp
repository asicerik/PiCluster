#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"

#include "GraphicsShim.h"
#include "Region.h"
#include "ClusterElement.h"

extern FontDatabaseFile*	gFontErasDemi18;		// 18 point Eras Demi ITC

/*
  The ClusterElement base class will draw a basic rectangle.
*/
ClusterElement::ClusterElement()
{
	mVisible		= true;
	mStateChanged	= true;
	mGradientAngle	= 0;
}

ClusterElement::~ClusterElement()
{
}

bool
ClusterElement::Init(const Rect& box, bool isPrimary, bool doubleBuffer)
{
	bool res = false;
	do
	{
		mClientRect = box;

		UartPrintf("&box = %p, box = %d,%d - %d,%d\n",
				&box,
				box.x,
				box.y,
				box.w,
				box.h);
		UartPrintf("&mClientRect = %p, mClientRect = %d,%d - %d,%d\n",
				&mClientRect,
				mClientRect.x,
				mClientRect.y,
				mClientRect.w,
				mClientRect.h);
		Invalidate(box);
		FramebufferProperties properties;
		properties.mBitsPerPixel = 32;
		properties.mDoubleBuffer = doubleBuffer;
		properties.mGeometry = box;

		if (isPrimary)
			mGfx.AllocatePrimaryFramebuffer(properties);
		else
			mGfx.AllocateFramebuffer(properties);

		// Mark the background/foregraond invalid so that they will get drawn
		mBackgroundDirtyRegion.AddRect(mClientRect);
		mForegroundDirtyRegion.AddRect(mClientRect);

		// Fill the framebuffers with magenta
		// This is needed because our fills will fail if they encounter the same
		// background color. So, let's pick a color that hopefully will not
		// show up anywhere
		if (!isPrimary)
			mGfx.FillRectangle(box, GraphicsContextBase::kMagenta);

		res = true;
	} while (false);
	return res;
}

void 
ClusterElement::SetVisible(bool visible)
{
	mVisible = visible;
}

void 
ClusterElement::SetLocation(const Point& loc)
{
	// Invalidate the background for the old location
	mScreenDirtyRegion.AddRect(mGfx.ClientToScreen(mClientRect));

	mGfx.SetScreenOffset(loc);

	// Invalidate the foreground for the new location
	mForegroundDirtyRegion.Clear();
	mForegroundDirtyRegion.AddRect(mClientRect);
	UartPrintf("mClientRect = %d,%d - %d,%d\n",
			mClientRect.x,
			mClientRect.y,
			mClientRect.w,
			mClientRect.h);
}

void 
ClusterElement::Invalidate(const Rect& box)
{
	// Translate from screen to client coordinates
	Region boxRegion(mGfx.ScreenToClient(box));

	mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, boxRegion, Region::eOr);
//	UartPrintf("Invalidate(Rect) : box = %d,%d - %d,%d, dirty = %d,%d - %d,%d\n",
//			mClientRect.x,
//			mClientRect.y,
//			mClientRect.w,
//			mClientRect.h,
//			mForegroundDirtyRegion.GetDirtyRect().x,
//			mForegroundDirtyRegion.GetDirtyRect().y,
//			mForegroundDirtyRegion.GetDirtyRect().w,
//			mForegroundDirtyRegion.GetDirtyRect().h
//			);

}

void 
ClusterElement::Invalidate(const Region& inRegion)
{
	Region& region = (Region&)inRegion;
	std::vector<Rect>& rects = region.GetDirtyRects();
	for (size_t i=0; i<rects.size(); i++)
	{
		// Translate from screen to client coordinates
		Region boxRegion(mGfx.ScreenToClient(rects[i]));
		mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, (Region&)boxRegion, Region::eOr);
	}
//	UartPrintf("Invalidate(Region) : dirty = %d,%d - %d,%d\n",
//			mForegroundDirtyRegion.GetDirtyRect().x,
//			mForegroundDirtyRegion.GetDirtyRect().y,
//			mForegroundDirtyRegion.GetDirtyRect().w,
//			mForegroundDirtyRegion.GetDirtyRect().h
//			);
}

Region 
ClusterElement::Update()
{
	// Update our internal state, and flag foreground/background if
	// any redraws are needed

	// We return any area that we did not draw so the object below us will draw it
	return mScreenDirtyRegion;
}	

Region 
ClusterElement::Draw()
{
	Region ret = mForegroundDirtyRegion;
	if (mStateChanged)
	{
		// Our state has changed, so redraw everything
		mGfx.GradientRectangle(mClientRect, mGradientAngle, mGradientStops);
		mStateChanged = false;
	}
	if (!mForegroundDirtyRegion.GetDirtyRects().empty())
	{
		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects(), false);
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	mScreenDirtyRegion.Clear();
	return ret;
}	

void 
ClusterElement::AddGradientStop(float position, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	GradientStop stop;
	stop.mPosition = position;
	stop.mColor.r = r;
	stop.mColor.g = g;
	stop.mColor.b = b;
	stop.mColor.a = a;
	mGradientStops.push_back(stop);
}

void 
ClusterElement::SetGradientAngle(int16_t angle)
{
	mGradientAngle = angle;
}

