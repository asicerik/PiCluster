#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"
#include "ClusterElement.h"
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
		mExtents = box;

		// The primary surface is where we draw images to the screen
		mPrimarySurface.Init(box, true);

		// Background - we use a cluster element so that we get all the graphics functions
		mBackground.Init(box);
		mBackground.SetGradientAngle(0);
		mBackground.AddGradientStop(0.0, eOpaque, 64, 64, 64 );	// dark grey
		mBackground.AddGradientStop(1.0, eOpaque, 0, 0, 0 );	// black

		// Draw directly to the screen
		mBackground.GetGraphicsContext().SelectPrimaryFrontBuffer();
		mElements.push_back(mBackground);

		// A test element
		Rect box = { 0, 0, 100, 100 };
		mTest.Init(box);
		mTest.SetGradientAngle(0);
		mTest.AddGradientStop(0.0, eOpaque, 128, 128, 128 );	// med grey
		mTest.AddGradientStop(1.0, eOpaque, 192, 192, 192 );	// light black

		// Draw directly to the screen
		mTest.GetGraphicsContext().SelectPrimaryFrontBuffer();
		mElements.push_back(mTest);

		res = true;
	} while (false);
	return res;
}

bool
InstrumentCluster::Update()
{
	bool res = false;
	do
	{
		// Update each element in turn.
		// The update calls will return the background area that was invalidated
		// by the update.
		// For now, elements cannot invalidate each other, except for the background
		Region backgroundUpdate;
		std::vector<ClusterElement>::iterator iter = mElements.begin();
		for (; iter != mElements.end(); iter++)
		{
			Region invalidated = iter->Update();
			backgroundUpdate = Region::CombineRegion(backgroundUpdate, invalidated, Region::eOr);
		}
		mBackground.Invalidate(backgroundUpdate);

		res = true;
	} while (false);
	return res;
}

