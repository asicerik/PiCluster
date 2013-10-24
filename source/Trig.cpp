#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#include "Trig.h"
#include <math.h>

float		Trig::kPi		= 3.14156f;
int32_t*	Trig::mCosInt		= NULL;
int32_t*	Trig::mSinInt		= NULL;
int32_t*	Trig::mCosWideInt	= NULL;
int32_t*	Trig::mSinWideInt	= NULL;

void 
Trig::BuildTrigTabs()
{
	mCosInt = new int32_t[360];
	mSinInt = new int32_t[360];
	mCosWideInt = new int32_t[1440];
	mSinWideInt = new int32_t[1440];
	for (int i=0;i<360;i++)
	{
		float rad = 2 * kPi * i / 360.0f;
		mCosInt[i] = (int32_t)(cos(rad) * kTrigScale);
		mSinInt[i] = (int32_t)(sin(rad) * kTrigScale);
	}
	for (int i=0;i<1440;i++)
	{
		float rad = 2 * kPi * i / 1440.0f;
		mCosWideInt[i] = (int32_t)(cos(rad) * kTrigScale);
		mSinWideInt[i] = (int32_t)(sin(rad) * kTrigScale);
	}
}

int32_t 
Trig::ClipDegree(int32_t angle)
{
	if (angle < 0)
		return 360 + angle % 360;
	else
		return angle % 360;
}

int32_t 
Trig::ClipWideDegree(int32_t angle)
{
	if (angle < 0)
		return 1440 + angle % 1440;
	else
		return angle % 1440;
}

