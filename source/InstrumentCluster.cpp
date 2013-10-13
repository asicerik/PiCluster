#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "uart0.h"
#include "Trig.h"
#include "InstrumentCluster.h"

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
		mPrimarySurface.Init(box, true);

		// Background - we use a cluster element so that we get all the graphics functions
		mBackground.Init(box);
		mBackground.SetGradientAngle(0);
		mBackground.AddGradientStop(0.0, eOpaque, 0, 0, 64);
		mBackground.AddGradientStop(1.0, eOpaque, 0, 0, 0);

		// Draw directly to the screen
		mBackground.GetGraphicsContext().SelectSurface(eFront);
		mElements.push_back(&mBackground);

		// A test element
		Rect box;
		box.x = box.y = 0;
		box.w = box.h = 100;

		mTest.Init(box);
		mTest.SetGradientAngle(0);
		mTest.AddGradientStop(0.0, eOpaque, 128, 128, 128 );	// med grey
		mTest.AddGradientStop(1.0, eOpaque, 192, 192, 192 );	// light black

		// Draw directly to the screen
		mTest.GetGraphicsContext().SelectSurface(ePrimaryFront);
		//mElements.push_back(&mTest);

		// Speedometer
		box.x = box.y = 0;
		box.w = box.h = 360;
		Point point;
		point.x = 0;
		point.y = 0;

		mSpeedo.Init(box);
		mSpeedo.SetLocation(point);

		//// Draw directly to the screen
		mSpeedo.GetGraphicsContext().SelectSurface(ePrimaryFront);
		mElements.push_back(&mSpeedo);

		res = true;
	} while (false);
	return res;
}

bool
InstrumentCluster::Update()
{
	bool res = false;
	static Point loc = { 0, 0 };
	do
	{
		mTest.SetLocation(loc);
		loc.x++;
		loc.y++;
		if (loc.y > 480)
		{
			loc.x = 0;
			loc.y = 0;
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
		}

		res = true;
	} while (false);
	return res;
}

void
InstrumentCluster::Draw()
{

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
}

