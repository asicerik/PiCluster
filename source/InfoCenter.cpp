#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#include "stdio.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"

#include "GraphicsShim.h"
#include "Region.h"
#include "ClusterElement.h"
#include "InfoCenter.h"

extern FontDatabaseFile*	gFontErasDemi18;		// 18 point Eras Demi ITC
extern BMPImage*			gCarTopView;				// The image for our car

InfoCenter::InfoCenter()
{
	mMode = eInfoModeOff;
}

InfoCenter::~InfoCenter()
{
}

bool
InfoCenter::Init(const Rect& box, bool isPrimary, bool doubleBuffer)
{
	return ClusterElement::Init(box, isPrimary, doubleBuffer);
}

void
InfoCenter::SetMode(InfoCenterMode mode)
{
	if (mMode != mode)
	{
		mStateChanged = true;
		mMode = mode;
	}
}

Region 
InfoCenter::Update()
{
	mTempRegion.Clear();
	if (mStateChanged)
	{
		// Invalidate the entire background on a state change
		mTempRegion = mForegroundDirtyRegion;
		mTempRegion.OffsetRegion(mGfx.GetScreenOffset().x, mGfx.GetScreenOffset().y);
	}
	return mTempRegion;
}	

Region 
InfoCenter::Draw()
{
	Region ret = mForegroundDirtyRegion;
	if (mStateChanged)
	{
		// Our state has changed, so redraw everything
		// NOTE : this could be optimized to track dirty regions if we care
		mForegroundDirtyRegion.Clear();
		mForegroundDirtyRegion.AddRect(mClientRect);
		mGfx.FillRectangle(mClientRect, mGfx.kMagenta);

		Point origin(mClientRect.w / 2, mClientRect.h / 2);
		Point imageLoc(origin.x - gCarTopView->width / 2, 0);

		mGfx.DrawBMP(imageLoc, gCarTopView, false);

		Point loc(48, 32);
		char text[256];
		sprintf(text, "%d", mPressureFrontLeft);
		mGfx.DrawText(gFontErasDemi18, text, loc, mTextColor, false, eAlignCenter);
		loc.x = 140;
		sprintf(text, "%d", mPressureFrontRight);
		mGfx.DrawText(gFontErasDemi18, text, loc, mTextColor, false, eAlignCenter);
		loc.y = 80;
		sprintf(text, "%d", mPressureRearRight);
		mGfx.DrawText(gFontErasDemi18, text, loc, mTextColor, false, eAlignCenter);
		loc.x = 48;
		sprintf(text, "%d", mPressureRearLeft);
		mGfx.DrawText(gFontErasDemi18, text, loc, mTextColor, false, eAlignCenter);

		loc.x = 96;
		loc.y = 0;
		sprintf(text, "Tire Pressure");
		mGfx.DrawText(gFontErasDemi18, text, loc, mTextColor, false, eAlignCenter);

		mStateChanged = false;
	}

	if (!mForegroundDirtyRegion.GetDirtyRects().empty())
	{
		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects(), true);
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	return ret;
}	

void
InfoCenter::SetTirePressures(uint8_t fl, uint8_t fr, uint8_t rl, uint8_t rr)
{
	if (mPressureFrontLeft  != fl ||
		mPressureFrontRight != fr ||
		mPressureRearLeft   != rl ||
		mPressureRearRight  != rr)
	{
		mStateChanged = true;
		Invalidate(mGfx.ClientToScreen(mClientRect));
	}
	mPressureFrontLeft		= fl;
	mPressureFrontRight		= fr;
	mPressureRearLeft		= rl;
	mPressureRearRight		= rr;
}
