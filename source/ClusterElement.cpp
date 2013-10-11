#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"

#include "GraphicsShim.h"
#include "ClusterElement.h"

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
ClusterElement::Init(const Rect& box, bool isPrimary)
{
	bool res = false;
	do
	{
		mBoundingBox = box;

		UartPrintf("box = %d,%d - %d,%d\n",
				box.x,
				box.y,
				box.w,
				box.h);
		UartPrintf("mBoundingBox = %d,%d - %d,%d\n",
				mBoundingBox.x,
				mBoundingBox.y,
				mBoundingBox.w,
				mBoundingBox.h);
		Invalidate(box);
		FramebufferProperties properties;
		properties.mBitsPerPixel = 32;
		properties.mDoubleBuffer = false;
		properties.mGeometry = box;
		if (isPrimary)
			mGfx.AllocatePrimaryFramebuffer(properties);
		else
			mGfx.AllocateFramebuffer(properties);

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
	mBackgroundDirtyRegion.AddRect(mBoundingBox);

	mBoundingBox.x = loc.x;
	mBoundingBox.y = loc.y;
	mGfx.SetOffset(loc);

	// Invalidate the foreground for the new location
	mForegroundDirtyRegion.Clear();
	mForegroundDirtyRegion.AddRect(mBoundingBox);
	UartPrintf("mBoundingBox = %d,%d - %d,%d\n",
			mBoundingBox.x,
			mBoundingBox.y,
			mBoundingBox.w,
			mBoundingBox.h);
}

void 
ClusterElement::Invalidate(const Rect& box)
{
	Region boxRegion(box);
	mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, boxRegion, Region::eOr);
	UartPrintf("Invalidate(Rect) : box = %d,%d - %d,%d, dirty = %d,%d - %d,%d\n",
			mBoundingBox.x,
			mBoundingBox.y,
			mBoundingBox.w,
			mBoundingBox.h,
			mForegroundDirtyRegion.GetDirtyRect().x,
			mForegroundDirtyRegion.GetDirtyRect().y,
			mForegroundDirtyRegion.GetDirtyRect().w,
			mForegroundDirtyRegion.GetDirtyRect().h
			);

}

void 
ClusterElement::Invalidate(const Region& region)
{
	mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, (Region&)region, Region::eOr);
	UartPrintf("Invalidate(Region) : dirty = %d,%d - %d,%d\n",
			mForegroundDirtyRegion.GetDirtyRect().x,
			mForegroundDirtyRegion.GetDirtyRect().y,
			mForegroundDirtyRegion.GetDirtyRect().w,
			mForegroundDirtyRegion.GetDirtyRect().h
			);
}

Region 
ClusterElement::Update()
{
	// Update our internal state, and flag foreground/background if
	// any redraws are needed

	// We return any background area that needs to be redrawn
	return mBackgroundDirtyRegion;
}	

Region 
ClusterElement::Draw()
{
	Region ret = mForegroundDirtyRegion;
	if (mStateChanged)
	{
		// Our state has changed, so redraw everything
		mGfx.GradientRectangle(mGradientAngle, mGradientStops);
		mStateChanged = false;
	}
	if (!mForegroundDirtyRegion.GetDirtyRects().empty())
	{
		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects());
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
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

