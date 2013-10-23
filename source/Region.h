#pragma once

#include <vector>

class Region
{
public:
	enum RegionOperation
	{
		eOr,
		eAnd
	};

	Region();
	Region(Rect rect);
	~Region();
	void SetMaxRects(size_t maxRects);
	void Clear();
	bool Empty();
	void AddRect(const Rect& rect);
	void SubtractRect(const Rect& rect);
	static Region CombineRegion(Region& a, Region& b, RegionOperation op);
	static Rect IntersectRects(const Rect& a, const Rect& b);
	std::vector<Rect>& GetDirtyRects()
	{
		return mDirtyRects;
	};
	Rect GetDirtyRect();
protected:
	void CoalesceRects();

	std::vector<Rect>	mDirtyRects;
	size_t				mMaxRects;		//!< The max number of rects we will allow. We will coelesce them after that
};


