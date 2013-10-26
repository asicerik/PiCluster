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
	mCurrentAngleWide = 0;
}

DialGuage::~DialGuage()
{
}

bool
DialGuage::Init(const Rect& box, bool isPrimary, bool doubleBuffer)
{
	bool res = false;
	do
	{
		if (isPrimary || doubleBuffer)
		{
			// DialGuage does not support these options
			break;
		}
		FramebufferProperties properties;
		properties.mBitsPerPixel = 32;
		properties.mDoubleBuffer = false;
		properties.mGeometry = box;
		mNeedleGfx.AllocateFramebuffer(properties);

		// Call the base class method to do all the normal housekeeping
		res = ClusterElement::Init(box);

		mGfx.SetSurfaceSelection(eFront);
		mNeedleGfx.SetSurfaceSelection(eFront);

		// Fill the framebuffers with magenta
		// This is needed because our fills will fail if they encounter the same
		// background color. So, let's pick a color that hopefully will not
		// show up anywhere
		mGfx.FillRectangle(box, GraphicsContextBase::kMagenta);
		mNeedleGfx.FillRectangle(box, GraphicsContextBase::kMagenta);

	} while (false);
	return res;
}

void
DialGuage::SetLocation(const Point& loc)
{
	mNeedleGfx.SetScreenOffset(loc);

	// Call the base class method to do all the normal housekeeping
	ClusterElement::SetLocation(loc);
}

void
DialGuage::SetMinMax(uint16_t min, uint16_t max, uint16_t minorStep, uint16_t majorStep, int16_t minAngle, int16_t maxAngle)
{
	SetMinMax(0, 0, min, max, minorStep, majorStep, minAngle, maxAngle);
}

void
DialGuage::SetMinMax(char minLabel, char maxLabel, uint16_t min, uint16_t max, uint16_t minorStep, uint16_t majorStep, int16_t minAngle, int16_t maxAngle)
{
	mMinChar	= minLabel;
	mMaxChar	= maxLabel;
	mMinInt		= min;
	mMaxInt		= max;

	mMinAngleWide	= minAngle * 4;
	mMaxAngleWide	= maxAngle * 4;

	// Calculate the delta between the dial min and max
	int16_t valueDelta	= max - min;
	int16_t minorSteps	= valueDelta / minorStep;
	int16_t majorSteps	= valueDelta / majorStep;

	int16_t angleDelta	= maxAngle - minAngle;
	mMinorTickWide		= 4 * angleDelta / minorSteps;
	mMajorTickWide		= 4 * angleDelta / majorSteps;
	
	// Pre-compute the factor to convert value to drawing angle
	mValToAngle			= 4 * 256 * angleDelta / valueDelta;

	mCurrentAngleWide = mMinAngleWide;		// for now
}

void
DialGuage::SetValue(uint16_t val)
{
	mCurrentAngleWide = (val * mValToAngle) / 256 + mMinAngleWide;
	mStateChanged	= true;
	if (mScreenDirtyRegion.Empty())
		mForegroundDirtyRegion.AddRect(mClientRect);
	else
	{
		Region temp = mScreenDirtyRegion;
		temp.OffsetRegion(-mGfx.GetScreenOffset().x, -mGfx.GetScreenOffset().y);
		mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, temp, Region::eOr);
	}
}

Region 
DialGuage::Update()
{
	if (mStateChanged)
	{
		return mScreenDirtyRegion;
	}
	Region dummy;
	return dummy;
}	

Region 
DialGuage::Draw()
{
	Region ret = mForegroundDirtyRegion;
	if (mStateChanged || !mForegroundDirtyRegion.Empty())
	{
		DrawBackground();
		DrawForeground();
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	return ret;
}	

void
DialGuage::DrawBackground()
{
	if (!mBackgroundDirtyRegion.Empty())
	{
		Point origin;
		origin.x = mClientRect.x + mClientRect.w / 2;
		origin.y = mClientRect.y + mClientRect.h / 2;

		Color32 color(32, 32, 64, eOpaque);

		mGfx.SetAntiAlias(true);

		DrawEmboss(mGfx, color, origin, mClientRect.w / 2 - 30, mClientRect.w / 2 - 10, 0, 1440, 600);

		color.r = color.g = color.b = 200;
		DrawEmboss(mGfx, color, origin, mClientRect.w / 2 - 5, mClientRect.w / 2 - 0, 0, 1440, 1320);

		color.r = color.g = color.b = 200;
		DrawTicks(color, origin, mClientRect.w / 2 - 25, mClientRect.w / 2 - 15, mMinAngleWide, mMaxAngleWide, mMinorTickWide, mMajorTickWide);

		// Once we have drawn our background, we should never need to draw it again.
		// Only copy it from our buffer
		mBackgroundDirtyRegion.Clear();
	}

//	mForegroundDirtyRegion.Clear();
//	mForegroundDirtyRegion.AddRect(mClientRect);

	// If the foreground dirty region is not empty, we need to copy some (or all) of our background
	// image to the primary buffer before the foreground is drawn
	if (!mForegroundDirtyRegion.Empty())
	{
//		UartPrintf("DrawBackground: ClientRect=%d,%d : %d,%d\n",
//				mClientRect.x,
//				mClientRect.y,
//				mClientRect.w,
//				mClientRect.h);
//		UartPrintf("DrawBackground: mForegroundDirtyRegion=%d,%d : %d,%d\n",
//				mForegroundDirtyRegion.GetDirtyRect().x,
//				mForegroundDirtyRegion.GetDirtyRect().y,
//				mForegroundDirtyRegion.GetDirtyRect().w,
//				mForegroundDirtyRegion.GetDirtyRect().h);

		//Color32 color;
		//color.r = color.g = color.b = 0;
		//color.r = 255;
		//Rect rect = mForegroundDirtyRegion.GetDirtyRect();
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x + rect.w, rect.y);
		//mGfx.DrawLine(color, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
		//mGfx.DrawLine(color, rect.x, rect.y + rect.h-1, rect.x + rect.w, rect.y + rect.h-1);
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x, rect.y + rect.h);

		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects(), true);
	}
}

void
DialGuage::DrawForeground()
{
	if (!mForegroundDirtyRegion.Empty())
	{
		mScreenDirtyRegion.Clear();

		Point origin;
		origin.x = mClientRect.x + mClientRect.w / 2;
		origin.y = mClientRect.y + mClientRect.h / 2;

		Color32 color(0, 0, 64, eOpaque);
		mNeedleGfx.SetAntiAlias(true);
		mScreenDirtyRegion.Clear();
		mNeedleGfx.EnableDirtyRects(&mScreenDirtyRegion);
		color.r = color.g = color.b = 230;

		// Draw the needle
		mNeedleGfx.DrawTrapezoid(color, origin, mCurrentAngleWide, 30, mClientRect.w / 2 - 15, 36, 4, true, eAntiAliasS0S1S2S3);

		mNeedleGfx.DisableDirtyRects();
		//color.r = color.g = color.b = 0;
		//color.g = 255;
		//Rect rect = mScreenDirtyRegion.GetDirtyRect();
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x + rect.w, rect.y);
		//mGfx.DrawLine(color, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
		//mGfx.DrawLine(color, rect.x, rect.y + rect.h-1, rect.x + rect.w, rect.y + rect.h-1);
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x, rect.y + rect.h);

		color.r = color.g = color.b = 200;
		DrawEmboss(mNeedleGfx, color, origin, 25, 30, 0, 1440, 1320);
		Rect rect;
		rect.x = origin.x - 30;
		rect.y = origin.y - 30;
		rect.w = 60;
		rect.h = 60;
		mScreenDirtyRegion.AddRect(rect);

		// Move the screen dirty region to screen coordinates
		mScreenDirtyRegion.OffsetRegion(mGfx.GetScreenOffset());
	}
	if (!mScreenDirtyRegion.Empty())
	{
		Color32 color(0, 0, 0, eTransparent);

		// Move the screen dirty region to client coordinates
		mScreenDirtyRegion.OffsetRegion(-mGfx.GetScreenOffset().x, -mGfx.GetScreenOffset().y);

		//color.r = color.g = color.b = 0;
		//color.r = 255;
		//Rect rect = mForegroundDirtyRegion.GetDirtyRect();
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x + rect.w, rect.y);
		//mGfx.DrawLine(color, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
		//mGfx.DrawLine(color, rect.x, rect.y + rect.h-1, rect.x + rect.w, rect.y + rect.h-1);
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x, rect.y + rect.h);

		// Copy the affected region to the primary surface
		mNeedleGfx.CopyToPrimary(mScreenDirtyRegion.GetDirtyRects(), true);
		mNeedleGfx.FillRectangle(mScreenDirtyRegion.GetDirtyRect(), color);

		// Move the screen dirty region back to screen coordinates
		mScreenDirtyRegion.OffsetRegion(mGfx.GetScreenOffset());
	}
}

void 
DialGuage::DrawEmboss(GraphicsContext& gfx, Color32 colorMax, Point origin, int16_t innerRadius, int16_t outerRadius, 
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

		gfx.DrawTrapezoid(color, origin, angleWide + stepAngleWide/2, innerRadius, outerRadius, 
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
		{
			mGfx.DrawTrapezoid(color, origin, angleWide, innerRadius, outerRadius, 
				4, 4,
//				18, 18,
				true,
				eAntiAliasS0S1S2S3	// Anti-alias all four sides
			);
			//std::string text = "100";
			//mGfx.DrawText(gFontErasDemi18, text, origin, color);
		}
		else if (((angleWide - startAngleWide) % minorTickWide) == 0)
			mGfx.DrawTrapezoid(color, origin, angleWide, innerRadius + (outerRadius-innerRadius)/2, outerRadius, 
				2, 2,
//				14, 14,
				true,
				eAntiAliasS0S1S2S3	// Anti-alias all four sides
			);
	}
}









