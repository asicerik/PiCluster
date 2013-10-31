#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#ifdef WIN32
#include "windows.h"
#endif
#include "GraphicsShim.h"
#include "Region.h"

Region::Region()
{
}

Region::Region(Rect rect)
{
	mDirtyRects.push_back(rect);
}

Region::~Region()
{
}

void
Region::Clear()
{
	mDirtyRects.clear();
}

bool
Region::Empty()
{
	return mDirtyRects.empty();
}

void 
Region::SetMaxRects(size_t maxRects)
{
	mMaxRects = maxRects;
	if (mDirtyRects.size() > mMaxRects)
		CoalesceRects();
}

void 
Region::AddRect(const Rect& rect)
{
	mDirtyRects.push_back(rect);
}

void 
Region::SubtractRect(const Rect& rect)
{
	// TODO
}

Region 
Region::CombineRegion(Region& a, Region& b, RegionOperation op)
{
	PROFILE_START(GraphicsContextBase::mProfileData.mRegion)

	Region c ALIGN;
	switch (op)
	{
		case eOr:
		{
			// 'or' is simple, just add all the rects together
			std::vector<Rect>& rects = a.GetDirtyRects();
			std::vector<Rect>::const_iterator iter = rects.begin();
			for (; iter != rects.end(); iter++)
			{
				c.AddRect(*iter);
			}
			rects = b.GetDirtyRects();
			iter = rects.begin();
			for (; iter != rects.end(); iter++)
			{
				c.AddRect(*iter);
			}
		}
		break;
		case eAnd:
		{
			// For the 'and' case, go through each rect in 'a',
			// and intersect it with each rect in 'b'.
			// We 'or' the intermediate results since we need
			// to check all the rectangles.
			std::vector<Rect>& rectsA = a.GetDirtyRects();
			std::vector<Rect>::const_iterator iterA = rectsA.begin();
			for (; iterA != rectsA.end(); iterA++)
			{
				std::vector<Rect>& rectsB = b.GetDirtyRects();
				std::vector<Rect>::const_iterator iterB = rectsB.begin();
				for (; iterB != rectsB.end(); iterB++)
				{
					Rect rect = IntersectRects(*iterA, *iterB);
					// Only add it if there is a positive area left after the intersection
					if ((rect.w > 0) && (rect.h > 0))
						c.AddRect(rect);
				}
			}
		}
		break;
	}
	
	PROFILE_STOP(GraphicsContextBase::mProfileData.mRegion)

	return c;
}

Rect
Region::IntersectRects(const Rect& a, const Rect& b)
{
	PROFILE_START(GraphicsContextBase::mProfileData.mRegion)

	Rect c ALIGN;
	int16_t right, bottom;

	// Find the left edge
	if (a.x < b.x)
	{
		c.x = b.x;
	}
	else
	{
		c.x = a.x;
	}

	// Now the top
	if (a.y < b.y)
	{
		c.y = b.y;
	}
	else
	{
		c.y = a.y;
	}

	// Now the right edge
	if ((a.x + a.w) < (b.x + b.w))
	{
		right = a.x + a.w;
	}
	else
	{
		right = b.x + b.w;
	}

	// Finally the bottom edge
	if ((a.y + a.h) < (b.y + b.h))
	{
		bottom = a.y + a.h;
	}
	else
	{
		bottom = b.y + b.h;
	}

	c.w = right - c.x;
	c.h = bottom - c.y;

	PROFILE_STOP(GraphicsContextBase::mProfileData.mRegion)

	return c;
}

Rect 
Region::GetDirtyRect()
{
	PROFILE_START(GraphicsContextBase::mProfileData.mRegion)

	Rect ret  ALIGN (0, 0, 0, 0);
	if (!mDirtyRects.empty())
	{
		Point topLeft( INT16_MAX, INT16_MAX );
		Point botRight( INT16_MIN, INT16_MIN );
		std::vector<Rect>::iterator iter = mDirtyRects.begin();
		for (; iter != mDirtyRects.end(); iter++)
		{
			if (iter->x < topLeft.x)
				topLeft.x = iter->x;
			if (iter->y < topLeft.y)
				topLeft.y = iter->y;
			if ((iter->x + iter->w) > botRight.x)
				botRight.x = (iter->x + iter->w);
			if ((iter->y + iter->h) > botRight.y)
				botRight.y = (iter->y + iter->h);
		}
		ret.x = topLeft.x;
		ret.y = topLeft.y;
		ret.w = botRight.x - topLeft.x;
		ret.h = botRight.y - topLeft.y;
	}
	PROFILE_STOP(GraphicsContextBase::mProfileData.mRegion)

	return ret;
}

void 
Region::CoalesceRects()
{
	PROFILE_START(GraphicsContextBase::mProfileData.mRegion)
	PROFILE_STOP(GraphicsContextBase::mProfileData.mRegion)
	return;

	while (mDirtyRects.size() > mMaxRects)
	{
		// Coalesce the smallest rects instead of making the biggest ones even bigger
	}
}

void 
Region::OffsetRegion(Point offset)
{
	OffsetRegion(offset.x, offset.y);
}

void 
Region::OffsetRegion(int16_t x, int16_t y)
{
	PROFILE_START(GraphicsContextBase::mProfileData.mRegion)

	std::vector<Rect>::iterator iter = mDirtyRects.begin();
	for (; iter != mDirtyRects.end(); iter++)
	{
		iter->x += x;
		iter->y += y;
	}

	PROFILE_STOP(GraphicsContextBase::mProfileData.mRegion)
}

