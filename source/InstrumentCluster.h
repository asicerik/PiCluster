#pragma once

#include <vector>

#include "GraphicsShim.h"
#include "ClusterElement.h"
#include "DialGuage.h"

class ALIGN InstrumentCluster
{
public:
	InstrumentCluster();
	~InstrumentCluster();
	bool Init(const Rect& box);
	bool Update();
	void Draw();
	ClusterElement& GetPrimarySurface()	{ return mPrimarySurface; };
	Region							mDirty;			//!< Region that need to be drawn to screen
	static GraphicsContext			mFontEras18;	//!< 18 point Eras Demi ITC font

protected:
	ClusterElement	mPrimarySurface;				//!< This is the surface that is copied to the screen
	ClusterElement	mBackground;					//!< Background image
	ClusterElement	mTest;
	DialGuage		mSpeedo;						//!< Speedometer
	DialGuage		mTach;							//!< Tachometer
	DialCap			mSpeedoCap;						//!< Speedometer cap
	DialCap			mTachCap;						//!< Tachometer cap

	// Fonts
	int32_t					mALign	ALIGN;			//!< Make sure everything below is 32b aligned
	std::vector<ClusterElement*>	mElements;		//!< All the elements are also stored here for easy traversal
	Rect							mExtents;		//!< The bounding box for our instrument cluster
};


