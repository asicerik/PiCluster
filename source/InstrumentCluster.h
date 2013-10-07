#pragma once

#include <vector>

class InstrumentCluster
{
public:
	InstrumentCluster();
	~InstrumentCluster();
	bool Init(const Rect& box);
	bool Update();
protected:
	ClusterElement mBackground;
	ClusterElement mTest;

	std::vector<ClusterElement>		mElements;		// All the elements are also stored here for easy traversal

	Rect							mExtents;		// The bounding box for our instrument cluster
};


