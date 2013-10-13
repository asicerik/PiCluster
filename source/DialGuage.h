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
protected:
};


