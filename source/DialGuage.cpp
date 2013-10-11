#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"

#include "GraphicsShim.h"
#include "ClusterElement.h"
#include "DialGuage.h"

ClusterElement::ClusterElement()
{
	mVisible		= true;
	mStateChanged	= true;
	mGradientAngle	= 0;
}

ClusterElement::~ClusterElement()
{
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


