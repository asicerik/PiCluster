#pragma once

#include "Region.h"

enum InfoCenterMode
{
	eInfoModeOff,			// Don't show anything
	eInfoModeTirePressure,	// Show the tire pressures
};

class InfoCenter : public ClusterElement
{
public:
	InfoCenter();
	virtual ~InfoCenter();

	virtual bool Init(const Rect& box, bool isPrimary=false, bool doubleBuffer=false);
	virtual Region Update();
	virtual Region Draw();
	
	void SetMode(InfoCenterMode mode);
	void SetTirePressures(uint8_t fl, uint8_t fr, uint8_t rl, uint8_t rr); 

protected:
	InfoCenterMode		mMode;		// What mode are we displaying

	// Tire pressures
	uint8_t				mPressureFrontLeft;
	uint8_t				mPressureFrontRight;
	uint8_t				mPressureRearLeft;
	uint8_t				mPressureRearRight;
};


