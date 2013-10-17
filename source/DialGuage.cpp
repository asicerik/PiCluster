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
#include "Trig.h"

extern FontDatabaseFile*	gFontErasDemi18;		// 18 point Eras Demi ITC

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
	Rect box;
	box.x = 200;
	box.y = 50;
	box.w = 360;
	box.h = 360;
	if (true || mStateChanged)
		mBackgroundDirtyRegion.AddRect(mBoundingBox);
		//mBackgroundDirtyRegion.AddRect(box);
	return ClusterElement::Update();
}	

Region 
DialGuage::Draw()
{
	int max=480;
	static int x=-max;
	static bool ping=false;
	Point origin;
	origin.x = mBoundingBox.x + mBoundingBox.w / 2;
	origin.y = mBoundingBox.y + mBoundingBox.h / 2;
	Region ret = mForegroundDirtyRegion;
	if (true || mStateChanged)
	{
		// Our state has changed, so redraw everything
		Color32 color;
		color.a = eOpaque;
		color.r = 0;
		color.g = 0;
		color.b = 255;
		mGfx.SetAntiAlias(true);
		//if (mStateChanged)
		{
			DrawEmboss(color, origin, mBoundingBox.w / 2 - 15, mBoundingBox.w / 2, 0, 1440, 600);

			color.r = color.g = color.b = 200;
			DrawTicks(color, origin, mBoundingBox.w / 2 - 30, mBoundingBox.w / 2 - 15, -480, 480, 30, 120);
		}
		color.r = color.g = color.b = 230;
		mGfx.DrawTrapezoid(color, origin, x, 30, mBoundingBox.w / 2 - 35, 36, 4, true, eAntiAliasS0S1S2S3);

		color.r = color.g = color.b = 255;
		DrawEmboss(color, origin, 25, 30, 0, 1440, 1320);

		std::string text = "Veronica";
		Point loc = { 120, 12 };
		color.r = 255;
		color.g = color.b = 0;
		mGfx.DrawText(gFontErasDemi18, text, loc, color);

		mStateChanged = false;
		Invalidate(mBoundingBox);
		x += 4;
		if (x >= max)
		{
			x = -max;
			ping = !ping;
		}
			mStateChanged = true;
		//Sleep(50);
	}
	mForegroundDirtyRegion.AddRect(mBoundingBox);
	if (!mForegroundDirtyRegion.GetDirtyRects().empty())
	{
		Color32 color;
		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects(), true);
		color.r = color.b = color.g = color.a = 0;
		mGfx.FillRectangle(mBoundingBox, color);
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	return ret;
}	

void 
DialGuage::DrawEmboss(Color32 colorMax, Point origin, int16_t innerRadius, int16_t outerRadius, 
					  int32_t startAngleWide, int32_t endAngleWide, int32_t peakAngleWide)
{
	const int32_t stepAngleWide = 20;		// 20 wide = 5 degrees
	Color32 color;
	color.a = eOpaque;
	for (int32_t angleWide = startAngleWide; angleWide <= endAngleWide; angleWide += stepAngleWide)
	{
		int32_t offset = Trig::ClipWideDegree(angleWide - peakAngleWide);
		int32_t shift  = offset < 720 ? (720 - offset) : (offset - 720);
		color.r = (uint8_t)(colorMax.r * shift / 720);
		color.g = (uint8_t)(colorMax.g * shift / 720);
		color.b = (uint8_t)(colorMax.b * shift / 720);

		mGfx.DrawTrapezoid(color, origin, angleWide + stepAngleWide/2, innerRadius, outerRadius, 
			stepAngleWide, stepAngleWide, true, 
			eAntiAliasS1S3	// Only anti-alias side 1 and 3 (top/bottom). If we do it on the sides, we will see a boundary
		);
	}
}

void 
DialGuage::DrawTicks(Color32 color, Point origin, int16_t innerRadius, int16_t outerRadius, 
					 int32_t startAngleWide, int32_t endAngleWide, int32_t minorTickWide, int32_t majorTickWide)
{
	for (int32_t angleWide = startAngleWide; angleWide <= endAngleWide; angleWide ++)
	{
		if (((angleWide - startAngleWide) % majorTickWide) == 0)
			mGfx.DrawTrapezoid(color, origin, angleWide, innerRadius, outerRadius, 
				4, 4,
//				18, 18,
				true,
				eAntiAliasS0S1S2S3	// Anti-alias all four sides
			);
		else if (((angleWide - startAngleWide) % minorTickWide) == 0)
			mGfx.DrawTrapezoid(color, origin, angleWide, innerRadius + (outerRadius-innerRadius)/2, outerRadius, 
				2, 2,
//				14, 14,
				true,
				eAntiAliasS0S1S2S3	// Anti-alias all four sides
			);
	}
}









