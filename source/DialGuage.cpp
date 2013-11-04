#include "stddef.h"
#include "stdarg.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
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
	mCurrentAngleWide 	= 0;
	mMinAngleWide		= 0;
	mMaxAngleWide		= 0;
	mMinorTickWide		= 0;
	mMajorTickWide		= 0;
	mMinInt				= 0;
	mMaxInt				= 0;
	mValToAngle			= 0;
	mCurrentAngleWide 	= 0;
	mMinChar			= 0;
	mMaxChar			= 0;
	mFullCircle			= false;
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

		// The needle will draw straight to the primary surface so that
		// we don't have to do an alpha blend later
//		mNeedleGfx.SetSurfaceSelection(ePrimaryBack);
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
	int16_t minorSteps	= abs(valueDelta) / minorStep;
	int16_t majorSteps	= abs(valueDelta) / majorStep;

	int16_t angleDelta	= maxAngle - minAngle;
	mMinorTickWide		= abs(4 * angleDelta / minorSteps);
	mMajorTickWide		= abs(4 * angleDelta / majorSteps);
	
	// Pre-compute the factor to convert value to drawing angle
	mValToAngle			= 4.0f * angleDelta / valueDelta;

	mCurrentAngleWide = mMinAngleWide;		// for now
}

void 
DialGuage::AddColorRange(Color32 color, int16_t minValue, int16_t maxValue)
{
	ColorRange range;
	range.mColor	= color;
	range.mMinValue	= minValue;
	range.mMaxValue	= maxValue;
	mColorRanges.push_back(range);
}

void
DialGuage::SetValue(uint16_t val)
{
	if (mMinInt > mMaxInt)
		mCurrentAngleWide = mMaxAngleWide + (val * mValToAngle);
	else
		mCurrentAngleWide = (val * mValToAngle) + mMinAngleWide;
	mStateChanged	= true;

	if (mScreenDirtyRegion.Empty())
		mForegroundDirtyRegion.AddRect(mClientRect);
	else
	{
		Region temp = mScreenDirtyRegion;
		temp.OffsetRegion(-mGfx.GetScreenOffset().x, -mGfx.GetScreenOffset().y);
		mForegroundDirtyRegion = Region::CombineRegion(mForegroundDirtyRegion, temp, Region::eOr);
	}
//	UartPrintf("DialGuage::SetValue() : value=%d, mForegroundDirtyRegion = %d,%d - %d,%d\n",
//			val,
//			mForegroundDirtyRegion.GetDirtyRect().x,
//			mForegroundDirtyRegion.GetDirtyRect().y,
//			mForegroundDirtyRegion.GetDirtyRect().w,
//			mForegroundDirtyRegion.GetDirtyRect().h
//			);
}

Region 
DialGuage::Update()
{
	mTempRegion = mForegroundDirtyRegion;
	mTempRegion.OffsetRegion(mGfx.GetScreenOffset().x, mGfx.GetScreenOffset().y);
//	UartPrintf("DialGuage::Update() : mScreenOffset = %d,%d. mTempRegion = %d,%d - %d,%d\n",
//			mGfx.GetScreenOffset().x, mGfx.GetScreenOffset().y,
//			mTempRegion.GetDirtyRect().x,
//			mTempRegion.GetDirtyRect().y,
//			mTempRegion.GetDirtyRect().w,
//			mTempRegion.GetDirtyRect().h
//			);
	return mTempRegion;

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
//	UartPrintf("DialGuage::Draw() : mForegroundDirtyRegion = %d,%d - %d,%d, this=%p\n",
//			mForegroundDirtyRegion.GetDirtyRect().x,
//			mForegroundDirtyRegion.GetDirtyRect().y,
//			mForegroundDirtyRegion.GetDirtyRect().w,
//			mForegroundDirtyRegion.GetDirtyRect().h,
//			this
//			);

	Region ret = mForegroundDirtyRegion;
	if (mStateChanged)// || !mForegroundDirtyRegion.Empty())
	{
		DrawBackground();
		DrawForeground();
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	ret.OffsetRegion(mGfx.GetScreenOffset().x, mGfx.GetScreenOffset().y);
	return ret;
}	

void
DialGuage::DrawBackground()
{
//	UartPrintf("DialGuage::DrawBackground() : mBackgroundDirtyRegion = %d,%d - %d,%d, this=%p\n",
//			mBackgroundDirtyRegion.GetDirtyRect().x,
//			mBackgroundDirtyRegion.GetDirtyRect().y,
//			mBackgroundDirtyRegion.GetDirtyRect().w,
//			mBackgroundDirtyRegion.GetDirtyRect().h,
//			this
//			);

	if (!mBackgroundDirtyRegion.Empty())
	{
		Point origin;
		origin.x = mClientRect.x + mClientRect.w / 2;
		origin.y = mClientRect.y + mClientRect.h / 2;
		int32_t	embossMin;
		int32_t embossMax;
		if (mFullCircle)
		{
			embossMin = 0;
			embossMax = 1440;
		}
		else
		{
			embossMin = mMinAngleWide;
			embossMax = mMaxAngleWide;
		}

		Color32 color(32, 32, 64, eOpaque);

		mGfx.SetAntiAlias(true);

		mGfx.DrawEmboss(color, origin, mClientRect.w / 2 - 30, mClientRect.w / 2 - 10, embossMin, embossMax, 600);

		std::vector<ColorRange>::iterator iter = mColorRanges.begin();
		for (; iter != mColorRanges.end(); iter++)
		{
			int16_t minAngle;
			int16_t maxAngle;
			int16_t stepAngleWide = 20;
			int16_t innerRadius   = mClientRect.w / 2 - 20;
			int16_t peakColorAngleWide = 600;
			if (mClientRect.w < 200)
			{
				stepAngleWide = 20;
				innerRadius   = mClientRect.w / 2 - 15;
				peakColorAngleWide = mMaxAngleWide;
			}
			if (mMinInt > mMaxInt)
			{
				minAngle = (int16_t)(iter->mMinValue * mValToAngle) + mMaxAngleWide;
				maxAngle = (int16_t)(iter->mMaxValue * mValToAngle) + mMaxAngleWide;
				mGfx.DrawEmboss(iter->mColor, origin, innerRadius, mClientRect.w / 2 - 10, minAngle, maxAngle, peakColorAngleWide, stepAngleWide);
			}
			else
			{
				minAngle = (int16_t)(iter->mMinValue * mValToAngle) + mMinAngleWide;
				maxAngle = (int16_t)(iter->mMaxValue * mValToAngle) + mMinAngleWide;
				mGfx.DrawEmboss(iter->mColor, origin, innerRadius, mClientRect.w / 2 - 10, minAngle, maxAngle, peakColorAngleWide, stepAngleWide);
			}
		}

		if (mFullCircle)
		{
			color.r = color.g = color.b = 200;
			mGfx.DrawEmboss(color, origin, mClientRect.w / 2 - 5, mClientRect.w / 2 - 0, 0, 1440, 1320);
		}

		color.r = color.g = color.b = 200;
		DrawTicks(color, origin, mClientRect.w / 2 - 25, mClientRect.w / 2 - 10, 
			//Trig::ClipWideDegree(mMinAngleWide), 
			//Trig::ClipWideDegree(mMaxAngleWide), 
			mMinAngleWide, 
			mMaxAngleWide, 
			mMinorTickWide, mMajorTickWide);

		DrawLabel();
		DrawLabelImage();

		// Once we have drawn our background, we should never need to draw it again.
		// Only copy it from our buffer
		mBackgroundDirtyRegion.Clear();
	}

//	mForegroundDirtyRegion.Clear();
//	mForegroundDirtyRegion.AddRect(mClientRect);

//	UartPrintf("DialGuage::DrawBackground() : mForegroundDirtyRegion = %d,%d - %d,%d, this=%p\n",
//			mForegroundDirtyRegion.GetDirtyRect().x,
//			mForegroundDirtyRegion.GetDirtyRect().y,
//			mForegroundDirtyRegion.GetDirtyRect().w,
//			mForegroundDirtyRegion.GetDirtyRect().h,
//			this
//			);
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

		// Trim the foreground region to our client rect
		Region clientRegion(mClientRect);
		mForegroundDirtyRegion = mForegroundDirtyRegion.CombineRegion(mForegroundDirtyRegion, clientRegion, Region::eAnd);

//		Color32 color;
//		color.r = color.g = color.b = 0;
//		color.g = 255;
//		Rect rect = mForegroundDirtyRegion.GetDirtyRect();
//		mGfx.DrawLine(color, rect.x, rect.y, rect.x + rect.w, rect.y);
//		mGfx.DrawLine(color, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
//		mGfx.DrawLine(color, rect.x, rect.y + rect.h-1, rect.x + rect.w, rect.y + rect.h-1);
//		mGfx.DrawLine(color, rect.x, rect.y, rect.x, rect.y + rect.h);

		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mForegroundDirtyRegion.GetDirtyRects(), true);
	}
}

void
DialGuage::DrawForeground()
{
//	UartPrintf("DialGuage::DrawForeground() : mForegroundDirtyRegion = %d,%d - %d,%d, this=%p\n",
//			mForegroundDirtyRegion.GetDirtyRect().x,
//			mForegroundDirtyRegion.GetDirtyRect().y,
//			mForegroundDirtyRegion.GetDirtyRect().w,
//			mForegroundDirtyRegion.GetDirtyRect().h,
//			this
//			);

	if (!mForegroundDirtyRegion.Empty())
	{
		mScreenDirtyRegion.Clear();

		Point origin;
		origin.x = mClientRect.x + mClientRect.w / 2;
		origin.y = mClientRect.y + mClientRect.h / 2;
		if (mNeedleGfx.GetSurfaceSelection() == ePrimaryBack)
		{
			// If we are drawing to the primary, we need to account for our screen location
			origin.x += mGfx.GetScreenOffset().x;
			origin.y += mGfx.GetScreenOffset().y;
		}

		Color32 color(0, 0, 64, eOpaque);
		mNeedleGfx.SetAntiAlias(true);
		mScreenDirtyRegion.Clear();
		mNeedleGfx.EnableDirtyRects(&mScreenDirtyRegion);
		color.r = color.g = color.b = 230;

		// Draw the needle
		if (mClientRect.w < 200)
			mNeedleGfx.DrawTrapezoid(color, origin, mCurrentAngleWide, 10, mClientRect.w / 2 - 20, 90, 8, true, eAntiAliasS0S1S2S3);
		else
			mNeedleGfx.DrawTrapezoid(color, origin, mCurrentAngleWide, 30, mClientRect.w / 2 - 20, 36, 4, true, eAntiAliasS0S1S2S3);

		mNeedleGfx.DisableDirtyRects();
		//color.r = color.g = color.b = 0;
		//color.g = 255;
		//Rect rect = mScreenDirtyRegion.GetDirtyRect();
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x + rect.w, rect.y);
		//mGfx.DrawLine(color, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
		//mGfx.DrawLine(color, rect.x, rect.y + rect.h-1, rect.x + rect.w, rect.y + rect.h-1);
		//mGfx.DrawLine(color, rect.x, rect.y, rect.x, rect.y + rect.h);

		if (mNeedleGfx.GetSurfaceSelection() != ePrimaryBack)
		{
			// Move the screen dirty region to screen coordinates
			// only if we are not drawing directly to the primary already
			mScreenDirtyRegion.OffsetRegion(mGfx.GetScreenOffset());
		}
	}
	if (!mScreenDirtyRegion.Empty())
	{
		Color32 color(0, 0, 0, eTransparent);

		// Move the screen dirty region to client coordinates
		mScreenDirtyRegion.OffsetRegion(-mGfx.GetScreenOffset().x, -mGfx.GetScreenOffset().y);

		if (mNeedleGfx.GetSurfaceSelection() != ePrimaryBack)
		{
			// Copy the affected region to the primary surface
			mNeedleGfx.CopyToPrimary(mScreenDirtyRegion.GetDirtyRects(), true);
		}

		color.r = color.g = color.b = 0;
		color.r = 255;
		color.a = 0;
		std::vector<Rect>::iterator iter = mForegroundDirtyRegion.GetDirtyRects().begin();
		for (; iter != mForegroundDirtyRegion.GetDirtyRects().end(); iter++)
		{
			Rect& rect = *iter;
			//mNeedleGfx.DrawLine(color, rect.x, rect.y, rect.x + rect.w, rect.y);
			//mNeedleGfx.DrawLine(color, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h);
			//mNeedleGfx.DrawLine(color, rect.x, rect.y + rect.h-1, rect.x + rect.w, rect.y + rect.h-1);
			//mNeedleGfx.DrawLine(color, rect.x, rect.y, rect.x, rect.y + rect.h);
			//mNeedleGfx.FillRectangle(rect, color);
		}
		if (mNeedleGfx.GetSurfaceSelection() != ePrimaryBack)
		{
	//		mNeedleGfx.FillRectangle(mClientRect, color);
			mNeedleGfx.FillRectangle(mScreenDirtyRegion.GetDirtyRect(), color);
		}

		// Move the screen dirty region back to screen coordinates
		mScreenDirtyRegion.OffsetRegion(mGfx.GetScreenOffset());
	}
}

void 
DialGuage::DrawTicks(Color32 color, Point origin, int16_t innerRadius, int16_t outerRadius, 
					 int32_t startAngleWide, int32_t endAngleWide, int32_t minorTickWide, int32_t majorTickWide)
{
	char textChar[256];
	int16_t textRadius = innerRadius - 25;
	int16_t majorTicks = abs((int16_t)(majorTickWide ? ((endAngleWide - startAngleWide) / majorTickWide) : 0));
	int16_t labelDelta = (mMaxInt - mMinInt) / majorTicks;
	int16_t currLabelVal = mMinInt;
	int16_t tickArc = 8;
	int32_t angleInc = startAngleWide < endAngleWide ? 1 : -1;
	if (outerRadius < 100)
		tickArc = 18;
	for (int32_t angleWide = startAngleWide; (angleInc == 1 ? (angleWide <= endAngleWide) : (angleWide >= endAngleWide)); angleWide+=angleInc)
	{
		if (((angleWide - startAngleWide) % majorTickWide) == 0)
		{
			mGfx.DrawTrapezoid(color, origin, angleWide, innerRadius, outerRadius, 
				tickArc, tickArc,
				true,
				eAntiAliasS0S1S2S3	// Anti-alias all four sides
			);
			if (mMinChar != 0 && mMaxChar != 0)
			{
				textRadius = innerRadius - 15;
				if (currLabelVal == mMinInt)
					textChar[0] = mMinChar;
				else
					textChar[0] = mMaxChar;
				textChar[1] = 0;
			}
			else
			{
				sprintf(textChar, "%d", currLabelVal);
			}
			currLabelVal += labelDelta;
			std::string text(textChar);
			int16_t length = mGfx.GetTextDrawnLength(gFontErasDemi18, text);
			Point point;
			int32_t clipped = Trig::ClipWideDegree(angleWide);
			point.x = (int16_t)(origin.x - (length / 2) + ((textRadius * Trig::mSinWideInt[clipped]) >> Trig::kTrigShift));
			point.y = (int16_t)(origin.y - (length / 2) + ((textRadius * -Trig::mCosWideInt[clipped]) >> Trig::kTrigShift));
			if (outerRadius < 100)
				point.y -= (gFontErasDemi18->fontAscent + gFontErasDemi18->fontDescent) / 2;
			mGfx.DrawText(gFontErasDemi18, text, point, color, false, eAlignLeft);
		}
		else if (((angleWide - startAngleWide) % minorTickWide) == 0)
			mGfx.DrawTrapezoid(color, origin, angleWide, innerRadius + (outerRadius-innerRadius)/2, outerRadius, 
				tickArc / 2, tickArc / 2,
				true,
				eAntiAliasS0S1S2S3	// Anti-alias all four sides
			);
	}
}

Region 
DialCap::Draw()
{
	Region ret = mForegroundDirtyRegion;
	if (mStateChanged)
	{
		mStateChanged = false;

		mGfx.SetAntiAlias(true);

		Point origin;
		origin.x = mClientRect.x + mClientRect.w / 2;
		origin.y = mClientRect.y + mClientRect.h / 2;
		Color32 color;

		color.r = color.g = color.b = 32;
		if (mClientRect.w < 60)
		{
			mGfx.DrawCircle(color, origin, 16, true);
			color.r = color.g = color.b = 100;
			mGfx.DrawEmboss(color, origin, 16, 20, 0, 1440, 1320, 60);
		}
		else
		{
			mGfx.DrawCircle(color, origin, 25, true);
			color.r = color.g = color.b = 100;
			mGfx.DrawEmboss(color, origin, 25, 30, 0, 1440, 1320, 20);
		}
		Rect rect;
		rect.x = origin.x - 30;
		rect.y = origin.y - 30;
		rect.w = 60;
		rect.h = 60;
		mScreenDirtyRegion.AddRect(rect);
	}
	mForegroundDirtyRegion.Clear();
	mForegroundDirtyRegion.AddRect(mClientRect);

	if (!mForegroundDirtyRegion.GetDirtyRects().empty())
	{
		Region clientRegion(mClientRect);
		mTempRegion = Region::CombineRegion(mForegroundDirtyRegion, clientRegion, Region::eAnd);

		// Copy the affected region to the primary surface
		mGfx.CopyToPrimary(mTempRegion.GetDirtyRects(), true);
	}
	// Clear our dirty regions since it is assumed we have drawn everything we are going to draw
	mForegroundDirtyRegion.Clear();
	mBackgroundDirtyRegion.Clear();
	ret.OffsetRegion(mGfx.GetScreenOffset().x, mGfx.GetScreenOffset().y);
	return ret;
}	












