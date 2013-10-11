#pragma once

#include <vector>

class InstrumentCluster
{
public:
	InstrumentCluster();
	~InstrumentCluster();
	bool Init(const Rect& box);
	bool Update();
	void Draw();
	ClusterElement& GetPrimarySurface()	{ return mPrimarySurface; };
	Region							mDirty;			//!< Region that need to be drawn to screen

protected:
	ClusterElement mPrimarySurface;					//!< This is the surface that is copied to the screen
	ClusterElement mBackground;						//!< Background image
	ClusterElement mTest;

	std::vector<ClusterElement*>	mElements;		//!< All the elements are also stored here for easy traversal
	Rect							mExtents;		//!< The bounding box for our instrument cluster
};


