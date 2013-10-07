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
	void Clear();
	void AddRect(const Rect& rect);
	void SubtractRect(const Rect& rect);
	static Region CombineRegion(Region& a, Region& b, RegionOperation op);
	static Rect IntersectRects(const Rect& a, const Rect& b);
	std::vector<Rect>& GetDirtyRects()
	{
		return mDirtyRects;
	};
protected:
	std::vector<Rect>	mDirtyRects;
};


