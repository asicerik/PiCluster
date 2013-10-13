#include "stddef.h"
#include "stdarg.h"
#include "string.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"

#include "GraphicsShim.h"
#include "ClusterElement.h"
#include "DialGuage.h"

DialGuage::DialGuage()
{
	mVisible		= true;
	mStateChanged	= true;
	mGradientAngle	= 0;
}

DialGuage::~DialGuage()
{
}

Region 
DialGuage::Update()
{
	//mBackgroundDirtyRegion.AddRect(mBoundingBox);
	return ClusterElement::Update();
}	

Region 
DialGuage::Draw()
{
	int max=360;
	static int x=0;
	Point origin;
	origin.x = mBoundingBox.x + mBoundingBox.w / 2 + 0;
	origin.y = mBoundingBox.y + mBoundingBox.h / 2 + 0;
	Region ret = mForegroundDirtyRegion;
	if (true || mStateChanged)
	{
		// Our state has changed, so redraw everything
		Color32 color;
		color.a = eOpaque;
		color.r = 255;
		color.g = color.b = 255;
		//memset(mGfx.GetSelectedFramebuffer(), 0, mGfx.GetFramebufferProperties().mStride * mGfx.GetFramebufferProperties().mGeometry.h);
		//mGfx.DrawLine(color, x, 0, 360-x, 360);
		mGfx.DrawTrapezoid(color, origin, x, 130, 150, 45, true);
		x += 90;
		if (x > 360)
		{
			x = x % 360;
		}
		Sleep(10);
		mStateChanged = false;
		Invalidate(mBoundingBox);
	}
	//if (!mForegroundDirtyRegion.GetDirtyRects().empty())
	//{
	//	// Copy the affected region to the primary surface
	//	mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects());
	//}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	return ret;
}	


