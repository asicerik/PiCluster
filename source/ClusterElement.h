#pragma once

#include "Region.h"

class ClusterElement
{
public:
	ClusterElement();
	virtual ~ClusterElement();
	virtual bool Init(const Rect& box, bool isPrimary=false, bool doubleBuffer=false);
	virtual void SetVisible(bool visible);
	virtual void Invalidate(const Rect& box);
	virtual void Invalidate(const Region& region);
	virtual void SetLocation(const Point& loc);
	virtual Region Update();
	virtual Region Draw();
	void AddGradientStop(float position, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
	void SetGradientAngle(int16_t angle);
	GraphicsContext& GetGraphicsContext()	{ return mGfx; };
protected:
	bool				mVisible;
	Rect				mBoundingBox;
	GraphicsContext		mGfx;
	Region				mForegroundDirtyRegion;		//!< The portion of our foreground that needs to be redrawn
	Region				mBackgroundDirtyRegion;		//!< The portion of our background that needs to be redrawn
	Region				mLowerZOrderDirtyRegion;	//!< The portion of the object below us that needs to be redrawn
	int16_t				mGradientAngle;
	std::vector<GradientStop>
						mGradientStops;
	bool				mStateChanged;
};


