#pragma once

#include <vector>

class ALIGN Region
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
	Region& operator=(const Region &rhs)
	{
	    // Check for self-assignment!
	    if (this == &rhs)      // Same object?
	      return *this;        // Yes, so skip assignment, and just return *this.

	    mDirtyRects.clear();
	    this->mMaxRects = rhs.mMaxRects;
	    for (size_t i=0; i<rhs.mDirtyRects.size(); i++)
	    	mDirtyRects.push_back(rhs.mDirtyRects[i]);

	    return *this;
	}
	void SetMaxRects(size_t maxRects);
	void Clear();
	bool Empty();
	void AddRect(const Rect& rect);
	void SubtractRect(const Rect& rect);
	static Region CombineRegion(Region& a, Region& b, RegionOperation op);
	static Rect IntersectRects(const Rect& a, const Rect& b);
	void OffsetRegion(Point offset);
	void OffsetRegion(int16_t x, int16_t y);
	std::vector<Rect>& GetDirtyRects()
	{
		return mDirtyRects;
	};
	Rect GetDirtyRect();
protected:
	void CoalesceRects();

	int32_t				mAlign		ALIGN;			//!< Make sure everything below is 32b aligned
	std::vector<Rect>	mDirtyRects ALIGN;
	size_t				mMaxRects;		//!< The max number of rects we will allow. We will coelesce them after that
};


