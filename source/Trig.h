#pragma once

/*	NOTE : there are two types of trig functions
	The *Int functions are indexed with degrees, 0-359.
	The *WideInt functions are indexed with wide degrees, 0-1339 to allow 1/4 degree resolution
*/

class Trig
{
public:
	static float kPi;						//!< Pi. DUH!
	static const int32_t kTrigScale = 1024;	//!< Factor used to scale [0..1] to [0..1024]
	static const int32_t kTrigShift = 10;	//!< 2^10 = 1024
	static int32_t*	mCosInt;				//!< Cos table indexed by degrees, scaled by kTrigScale
	static int32_t*	mSinInt;				//!< Sin table indexed by degrees, scaled by kTrigScale
	static int32_t*	mCosWideInt;			//!< Cos table indexed by wide degrees, scaled by kTrigScale
	static int32_t*	mSinWideInt;			//!< Sin table indexed by wide degrees, scaled by kTrigScale
	static void BuildTrigTabs();			//!< Pre-compute the trig tables
	static int32_t ClipDegree(int32_t angle);//!< Make sure angle is between 0 and 359
	static int32_t ClipWideDegree(int32_t angle);//!< Make sure angle is between 0 and 1339
};
