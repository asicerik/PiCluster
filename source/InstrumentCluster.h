#pragma once

#include <vector>

#include "GraphicsShim.h"
#include "ClusterElement.h"
#include "DialGuage.h"
#include "InfoCenter.h"

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

	// Images and fonts
	//static GraphicsContext			mFontEras18;	//!< 18 point Eras Demi ITC font
	//static BMPImage					mWaterTempImage;//!< Image for water temp guage

protected:
	ClusterElement	mPrimarySurface;				//!< This is the surface that is copied to the screen
	ClusterElement	mBackground;					//!< Background image
	DialGuage		mSpeedo;						//!< Speedometer
	DialGuage		mTach;							//!< Tachometer
	DialGuage		mWaterTemp;						//!< Water temp gauge
	DialGuage		mFuel;							//!< Fuel gauge
	DialCap			mSpeedoCap;						//!< Speedometer cap
	DialCap			mTachCap;						//!< Tachometer cap
	DialCap			mWaterTempCap;					//!< Water temp guage cap
	DialCap			mFuelCap;						//!< Fuel guage cap
	InfoCenter		mInfoCenter;					//!< Top view of our car
	ClusterElement	mLeftArrow;						//!< Left turn signal
	ClusterElement	mRightArrow;					//!< Right turn signal

	// Timers
	uint64_t		mNextFlasherChange;				//!< Time (in ms) to cycle the turn signal flasher

	// Fonts
	int32_t					mALign	ALIGN;			//!< Make sure everything below is 32b aligned
	std::vector<ClusterElement*>	mElements;		//!< All the elements are also stored here for easy traversal
	Rect							mExtents;		//!< The bounding box for our instrument cluster
};


