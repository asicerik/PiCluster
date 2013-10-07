#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"
#include "ClusterElement.h"

/*
  The ClusterElement base class will draw a basic rectangle.
*/
ClusterElement::ClusterElement()
{
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

	// Invalidate the foreground for the new location
	mForegroundDirtyRegion.Clear();
	mForegroundDirtyRegion.AddRect(mBoundingBox);
}

void 
ClusterElement::Invalidate(const Rect& box)
{
	Region boundingBoxRegion(mBoundingBox);
	Region boxRegion(box);
	Region dirty = Region::CombineRegion(boundingBoxRegion, boxRegion, Region::eAnd);
	mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, dirty, Region::eOr);
}

void 
ClusterElement::Invalidate(const Region& region)
{
	Region boundingBoxRegion(mBoundingBox);
	Region dirty = Region::CombineRegion(boundingBoxRegion, (Region&)region, Region::eAnd);
	mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, dirty, Region::eOr);
}

Region 
ClusterElement::Update()
{
	if (true || !mForegroundDirtyRegion.GetDirtyRects().empty())
	{
		mGfx.GradientRectangle(mGradientAngle, mGradientStops);
	}
	mForegroundDirtyRegion.Clear();
	return mBackgroundDirtyRegion;
}	

void 
ClusterElement::AddGradientStop(float position, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	GradientStop stop = { position, b, g, r, a };
	mGradientStops.push_back(stop);
}

void 
ClusterElement::SetGradientAngle(int16_t angle)
{
	mGradientAngle = angle;
}

