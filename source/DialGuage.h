#pragma once

#include "Region.h"

struct PACK DialProperties
{
	int16_t		min;
	int16_t		max;
	int16_t		startAngle;
	int16_t		endAngle;
};

class ALIGN DialGuage : public ClusterElement
{
public:
	DialGuage();
	virtual ~DialGuage();
	virtual bool Init(const Rect& box, bool isPrimary=false, bool doubleBuffer=false);
	virtual void SetLocation(const Point& loc);
	virtual Region Update();
	virtual Region Draw();
	void SetValue(uint16_t val);
	void SetMinMax(uint16_t min, uint16_t max, uint16_t minorTick, uint16_t majorTick, int16_t minAngle, int16_t maxAngle);
	void SetMinMax(char minLabel, char maxLabel, uint16_t min, uint16_t max, uint16_t minorStep, uint16_t majorStep, int16_t minAngle, int16_t maxAngle);
protected:
	void DrawBackground();
	void DrawForeground();
	void DrawEmboss(GraphicsContext& gfx, Color32 colorMax, Point origin, int16_t innerRadius, int16_t outerRadius, 
					int32_t startAngleWide, int32_t endAngleWide, int32_t peakAngleWide);
	void DrawTicks(Color32 color, Point origin, int16_t innerRadius, int16_t outerRadius, 
				   int32_t startAngleWide, int32_t endAngleWide, int32_t minorTickWide, int32_t majorTickWide);
	
	GraphicsContext		mNeedleGfx;	//!< Graphics context for drawing the needle

	int16_t		mMinAngleWide;	//!< Angle of "min" point. Note - "wide" angle from 0-1439 instead of 0-359
	int16_t		mMaxAngleWide;	//!< Angle of "max" point. Note - "wide" angle from 0-1439 instead of 0-359
	uint16_t	mMinorTickWide;	//!< Angle between small ticks on dial graph (wide angle)
	uint16_t	mMajorTickWide;	//!< Angle between large ticks on dial graph (wide angle)
	char		mMinChar;		//!< Minimum value in char mode (0 means integer mode)
	char		mMaxChar;		//!< Maximum value in char mode (0 means integer mode)
	uint16_t	mMinInt;		//!< Minimum value in integer mode
	uint16_t	mMaxInt;		//!< Minimum value in integer mode

	int16_t		mValToAngle;	//!< Value to convert current value to the drawing angle (scale=256)
	int16_t		mCurrentAngleWide;	//!< Current dial value in wide degrees
};


