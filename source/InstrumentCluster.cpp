#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"
#include "Trig.h"
#include "InstrumentCluster.h"

//GraphicsContext			InstrumentCluster::mFontEras18;

extern BMPImage*		gWaterTempImage;
extern BMPImage*		gFuelImage;
extern BMPImage*		gLeftArrowImage;
extern BMPImage*		gRightArrowImage;

/*
	Graphics general concepts:
	Each display element shown is based on a ClusterElement.
	We maintain a vector of elements to be drawn. The list is kept in z-order.
	The lowest order element is the background element.

	ClusterElements can draw to their own framebuffers, or directly to the screen (which
	may be double buffered itself.) Wherever possible, it is best to pre-render elements
	to framebuffers that can quickly be copied to the screen using DMA or BitBlits.
	The mBackgrond element is an example of this. It draws a gradient at boot time, and then
	never has to recreate the gradient again. It can simply copy the rendered image from its
	framebuffer to the screen.

	ClusterElements keep track of their foreground and background regions that need repainting
	using the Region class. The drawing cycle consists of two phases:

	Phase 1 Update() : 
		During the update phase, ClusterElements determine what state changes have occurred since
		the last Update/Draw cycle. If their internal elements have moved, they may have exposed 
		an area from the element below it. This is indicated by returning a Region object from
		the Update() call. The Update() phase is called in reverse z-order. So, the top elements
		are updated first. Since each object returns a region of what it exposed, the next element
		called will be passed the Region that was just invalidated. If this next element called
		can completely draw the exposed area, it will n
*/
InstrumentCluster::InstrumentCluster()
{
	mNextFlasherChange	= GetTimeMs();
}

InstrumentCluster::~InstrumentCluster()
{
}

bool
InstrumentCluster::Init(const Rect& boxIn)
{
	bool res = false;
	do
	{
		Trig::BuildTrigTabs();

		Rect box(boxIn);
		mExtents = box;

		// The primary surface is where we draw images to the screen
		mPrimarySurface.Init(box, true, true);

		// We draw to the back surface to get double buffering
		mPrimarySurface.GetGraphicsContext().SetSurfaceSelection(ePrimaryBack);

		// Background - we use a cluster element so that we get all the graphics functions
		mBackground.Init(box);
		mBackground.SetGradientAngle(0);
		mBackground.AddGradientStop(0.0, eOpaque, 0, 0, 64);
		mBackground.AddGradientStop(1.0, eOpaque, 0, 0, 0);

		mBackground.GetGraphicsContext().SetSurfaceSelection(eFront);
		mElements.push_back(&mBackground);

		Color32 textColor(200,200,200,eOpaque);

		// Speedometer
		box.x = box.y = 0;
		box.w = box.h = 360;
		Point point;
		point.x = 200;
		point.y = 50;
		mSpeedo.Init(box);
		mSpeedo.SetMinMax(0, 160, 5, 10, -120, 120);
		mSpeedo.SetLocation(point);
		mSpeedo.SetTextColor(textColor);
		mSpeedo.SetLabelText("miles/hour");
		mSpeedo.SetLabelCenter(Point(180, 75));
		mSpeedo.SetFullCircle(true);
		mElements.push_back(&mSpeedo);
		
		// Tachometer
		point.x = 1280 - 200 - box.w;
		point.y = 50;
		mTach.Init(box);
		mTach.SetValue(0);
		mTach.SetMinMax(0, 8000, 250, 1000, -120, 120);
		mTach.AddColorRange(Color32(200, 0, 0, eOpaque), 7000, 8000);
		mTach.AddColorRange(Color32(0xff, 0x33, 0, eOpaque), 6500, 7000);
		mTach.SetLocation(point);
		mTach.SetTextColor(textColor);
		mTach.SetLabelText("revs/min");
		mTach.SetLabelCenter(Point(180, 75));
		mTach.SetFullCircle(true);
		mElements.push_back(&mTach);

		// Water temp guage
		point.x = 100;
		point.y = 300;
		box.w = box.h = 150;
		mWaterTemp.Init(box);
		mWaterTemp.SetMinMax('C', 'H', 0, 100, 25, 100, -150, -30);
		mWaterTemp.SetValue(50);
		mWaterTemp.AddColorRange(Color32(200, 200, 200, eOpaque), 0, 80);
		mWaterTemp.AddColorRange(Color32(200, 0, 0, eOpaque), 85, 100);
		mWaterTemp.SetLocation(point);
		mWaterTemp.SetLabelImage(gWaterTempImage);
//		mWaterTemp.SetLabelImageCenter(Point(35, 75));
		mWaterTemp.SetLabelImageCenter(Point(70, 90));
		mWaterTemp.SetFullCircle(false);
		mElements.push_back(&mWaterTemp);

		// Fuel guage
		point.x = 1280 - 100 - box.w;
		point.y = 300;
		box.w = box.h = 150;
		mFuel.Init(box);
		mFuel.SetValue(50);
		mFuel.SetMinMax('F', 'E', 100, 0, 25, 100, 30, 150);
		mFuel.AddColorRange(Color32(200, 200, 200, eOpaque), 100, 0);
		mFuel.AddColorRange(Color32(200, 0, 0, eOpaque), 10, 0);
		mFuel.SetLocation(point);
		mFuel.SetLabelImage(gFuelImage);
//		mFuel.SetLabelImageCenter(Point(90, 75));
		mFuel.SetLabelImageCenter(Point(50, 90));
		mFuel.SetFullCircle(false);
		mElements.push_back(&mFuel);

		// Caps for dials
		int16_t capRadius = 30;
		box.x = 0;
		box.y = 0;
		box.w = capRadius * 2;
		box.h = capRadius * 2;

		// Speedo cap
		point.x = 200 + 180 - capRadius;
		point.y = 50  + 180 - capRadius;
		mSpeedoCap.Init(box);
		mSpeedoCap.SetLocation(point);
		mSpeedoCap.GetGraphicsContext().SetSurfaceSelection(eFront);
		//mElements.push_back(&mSpeedoCap);

		// Tach cap
		point.x = 1280 - 200 - 180 - capRadius;
		point.y = 50  + 180 - capRadius;
		mTachCap.Init(box);
		mTachCap.SetLocation(point);
		mTachCap.GetGraphicsContext().SetSurfaceSelection(eFront);
		//mElements.push_back(&mTachCap);
		
		// Water temp cap
		capRadius = 25;
		box.w = capRadius * 2;
		box.h = capRadius * 2;
		point.x = 175 - capRadius;
		point.y = 300 + 75 - capRadius;
		mWaterTempCap.Init(box);
		mWaterTempCap.SetLocation(point);
		mWaterTempCap.GetGraphicsContext().SetSurfaceSelection(eFront);
		//mElements.push_back(&mWaterTempCap);

		// Fuel cap
		point.x = 1280 - 175 - capRadius;
		point.y = 300 + 75 - capRadius;
		mFuelCap.Init(box);
		mFuelCap.SetLocation(point);
		mFuelCap.GetGraphicsContext().SetSurfaceSelection(eFront);
		//mElements.push_back(&mFuelCap);

		// InfoCenter - top view of our car + information display
		box.w = 192;
		box.h = 192;
		mInfoCenter.Init(box);
		point.x = 544;
		point.y = 100;
		mInfoCenter.SetLocation(point);
		mInfoCenter.SetMode(eInfoModeTirePressure);
		mInfoCenter.SetTextColor(textColor);
		mElements.push_back(&mInfoCenter);

		// Left turn signal
		box.w = 48;
		box.h = 48;
		mLeftArrow.Init(box);
		point.x = 500;
		point.y = 30;
		mLeftArrow.SetLocation(point);
		mLeftArrow.SetImage(gLeftArrowImage);
		mElements.push_back(&mLeftArrow);

		// Right turn signal
		mRightArrow.Init(box);
		point.x = 732;
		point.y = 30;
		mRightArrow.SetLocation(point);
		mRightArrow.SetImage(gRightArrowImage);
		mElements.push_back(&mRightArrow);

		res = true;
	} while (false);
	return res;
}

bool
InstrumentCluster::Update()
{
	PROFILE_START(GraphicsContextBase::mProfileData.mUpdate)

	bool res = false;
	static int16_t rpm = 0;
	static int16_t speed = 0;
	static int16_t fuel = 0;
	static int16_t temp = 0;
	do
	{
		mTach.SetValue(rpm);
		mSpeedo.SetValue(speed);
		mFuel.SetValue(fuel);
		mWaterTemp.SetValue(temp);
		mInfoCenter.SetTirePressures(rpm / 4000, rpm / 4000 + 1, rpm / 4000 + 2, rpm / 4000 + 3);
		if (GetTimeMs() >= mNextFlasherChange)
		{
			mNextFlasherChange = GetTimeMs() + 500;
			if (mLeftArrow.GetVisible())
			{
				mLeftArrow.SetVisible(false);
				mRightArrow.SetVisible(false);
			}
			else 
			{
				mLeftArrow.SetVisible(true);
				mRightArrow.SetVisible(true);
			}
		}
		rpm += 100;
		if (rpm >= 8000)
			rpm = 0;
		speed += 1;
		if (speed >= 180)
			speed = 0;
		fuel -= 1;
		if (fuel <= 0)
			fuel = 100;
		temp += 1;
		if (temp >= 100)
			temp = 0;

		// Go through all the elements in top-down Z order
		// Elements will return any background regions that were exposed
		// We tell the lower element about the exposed region so it can redraw it
		Region backgroundUpdate;
		std::vector<ClusterElement*>::reverse_iterator iter = mElements.rbegin();
		for (; iter != mElements.rend(); iter++)
		{
			// Invalidate the region exposed from the element above (if any)
			(*iter)->Invalidate(backgroundUpdate);

			// Update the element. 
			// Note - someday, it may consume any prior background updates
			// if it completely contains the invalidated region.
			// For now, we 'or' them all together
			Region updateRegion =  (*iter)->Update();
			backgroundUpdate = Region::CombineRegion(backgroundUpdate, updateRegion, Region::eOr);

//			UartPrintf("InstrumentCluster::Update() : backgroundUpdate = %d,%d - %d,%d. iter=%p\n",
//					backgroundUpdate.GetDirtyRect().x,
//					backgroundUpdate.GetDirtyRect().y,
//					backgroundUpdate.GetDirtyRect().w,
//					backgroundUpdate.GetDirtyRect().h,
//					*iter
//					);
		}

		res = true;
	} while (false);

	PROFILE_STOP(GraphicsContextBase::mProfileData.mUpdate)

	return res;
}

void
InstrumentCluster::Draw()
{
	PROFILE_START(GraphicsContextBase::mProfileData.mDraw)

//	UartPrintf("InstrumentCluster::Draw()\n");

	// Go through all the elements in bottoms-up Z order
	// Elements will return a region indicating the area it updated
	// so that we know what areas we need to copy to the screen (if any)

	// Start by clearing the dirty regions since it assumed
	// the previous updates copied all the data needed
	mDirty.Clear();

	std::vector<ClusterElement*>::iterator iter = mElements.begin();
	for (; iter != mElements.end(); iter++)
	{
		// Combine the updated regions from each draw call
		Region drawResult = (*iter)->Draw();
		mDirty = Region::CombineRegion(mDirty, drawResult, Region::eOr);
	}

	PROFILE_STOP(GraphicsContextBase::mProfileData.mDraw)
}

