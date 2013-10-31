#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"
#include "Trig.h"
#include "InstrumentCluster.h"

GraphicsContext			InstrumentCluster::mFontEras18;

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
}

InstrumentCluster::~InstrumentCluster()
{
}

bool
InstrumentCluster::Init(const Rect& box)
{
	bool res = false;
	do
	{
		Trig::BuildTrigTabs();

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

		// A test element
		Rect box;

		// Speedometer
		box.x = box.y = 0;
		box.w = box.h = 360;
		Point point;
		point.x = 200;
		point.y = 50;
		mSpeedo.Init(box);
		mSpeedo.SetMinMax(0, 160, 5, 10, -120, 120);

		mSpeedo.SetLocation(point);
		mElements.push_back(&mSpeedo);
		
		point.x = 1280 - 200 - box.w;
		point.y = 50;
		mTach.Init(box);
		mTach.SetValue(800);
		mTach.SetMinMax(0, 8000, 250, 1000, -120, 120);
		mTach.AddColorRange(Color32(200, 0, 0, eOpaque), 7000, 8000);
		mTach.AddColorRange(Color32(0xff, 0x33, 0, eOpaque), 6500, 7000);

		mTach.SetLocation(point);
		mElements.push_back(&mTach);

		// Caps for dials
		int16_t capRadius = 30;
		box.x = 0;
		box.y = 0;
		box.w = capRadius * 2;
		box.h = capRadius * 2;
		point.x = 200 + 180 - capRadius;
		point.y = 50  + 180 - capRadius;
		mSpeedoCap.Init(box);
		mSpeedoCap.SetLocation(point);
		mSpeedoCap.GetGraphicsContext().SetSurfaceSelection(eFront);
		mElements.push_back(&mSpeedoCap);

		point.x = 1280 - 200 - 180 - capRadius;
		point.y = 50  + 180 - capRadius;
		mTachCap.Init(box);
		mTachCap.SetLocation(point);
		mTachCap.GetGraphicsContext().SetSurfaceSelection(eFront);
		mElements.push_back(&mTachCap);

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
	do
	{
		mTach.SetValue(rpm);
		mSpeedo.SetValue(rpm / 100);
		rpm += 50;
		if (rpm > 8000)
		{
			rpm = 0;
		}

		// Go through all the elements in top-down Z order
		// Elements will return any background regions that were exposed
		// We tell the lower element about the exposed region so it can redraw it
		Region backgroundUpdate;
		std::vector<ClusterElement*>::reverse_iterator iter = mElements.rbegin();
		for (; iter != mElements.rend(); iter++)
		{
			// Invalidate the region exposed from the element above (if any)
			(*iter)->Invalidate(backgroundUpdate);

			// Update the element. Note - it may consume any prior background updates
			// if it completely contains the invalidated region.
			// It may also make the invalid region bigger
			backgroundUpdate = (*iter)->Update();

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

