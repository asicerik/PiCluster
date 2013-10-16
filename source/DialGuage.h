#pragma once

#include "Region.h"

struct DialProperties
{
	int16_t		min;
	int16_t		max;
	int16_t		startAngle;
	int16_t		endAngle;
};

class DialGuage : public ClusterElement
{
public:
	DialGuage();
	virtual ~DialGuage();
	virtual Region Update();
	virtual Region Draw();
	void SetMinMax(uint16_t min, uint16_t max, uint16_t minorStep, uint16_t majorStep);
	void SetMinMax(char min, char max);
protected:
	void DrawEmboss(Color32 colorMax, Point origin, int16_t innerRadius, int16_t outerRadius, 
					int32_t startAngleWide, int32_t endAngleWide, int32_t peakAngleWide);
	void DrawTicks(Color32 color, Point origin, int16_t innerRadius, int16_t outerRadius, 
				   int32_t startAngleWide, int32_t endAngleWide, int32_t minorTickWide, int32_t majorTickWide);
	char		mMinChar;		//!< Minimum value in char mode (0 means integer mode)
	char		mMaxChar;		//!< Maximum value in char mode (0 means integer mode)
	int16_t		mMinInt;		//!< Minimum value in integer mode
	int16_t		mMaxInt;		//!< Minimum value in integer mode
};


