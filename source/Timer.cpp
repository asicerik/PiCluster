#include "stdint.h"
#if defined _WIN32
#include "windows.h"
#else
#include <sys/time.h>
#include "bcm2835.h"
#endif
#include "Timer.h"

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif

uint64_t GetTimeUs()
{
	uint64_t t = GetTimeUTC( );
	return t;
}

uint64_t GetTimeMs()
{
	uint64_t t = GetTimeUs( );
	t /= 1000;
	return t;
}

// return utc time in microseconds
uint64_t GetTimeUTC( ) 
{
	uint64_t t;
#if defined _WIN32
	FILETIME        ft;
    SYSTEMTIME      st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,&ft);

	t  = FileTimeToUTCTime(ft);
#else
	t = bcm2835_st_read();
#endif // defined _WIN32
    return t; 
}

#if defined _WIN32
uint64_t FileTimeToUTCTime( const FILETIME &ft ) 
{
	uint64_t t;
	LARGE_INTEGER   li;
	li.LowPart  = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    t  = li.QuadPart;				/* In 100-nanosecond intervals */
    t -= EPOCHFILETIME;             /* Offset to the Epoch time */
    t /= 10;                        // convert to microseconds
	return t;
}
#endif // defined _WIN32

Timer::Timer()
{
	Reset();
}

void
Timer::Reset()
{
	mStartTime		= 0L;
	mStopTime		= 0L;
	mElapsed		= 0L;
	mElapsedTotal	= 0L;
	mSnapshot		= 0L;
	mRunning		= false;
}

uint64_t 
Timer::StartTimer()
{
	mStartTime	= GetTimeUs();
	mRunning	= true;
	return mStartTime;
}

uint64_t 
Timer::StopTimer()
{
	mStopTime		= GetTimeUs();
	mRunning		= false;
	mElapsedTotal	+= (mStopTime - mStartTime);
	return mStopTime;
}

uint64_t 
Timer::GetElapsed()
{
	if (mRunning)
	{
		return GetTimeUs() - mStartTime;
	}
	else
	{
		return mStopTime - mStartTime;
	}
}

uint64_t 
Timer::GetElapsedTotal()
{
	if (mRunning)
	{
		return (GetTimeUs() - mStartTime) + mElapsedTotal;
	}
	else
	{
		return mElapsedTotal;
	}
}

uint64_t 
Timer::Snapshot(bool clear)
{
	mSnapshot = GetElapsedTotal();
	if (clear)
		mElapsedTotal = 0L;
	return mSnapshot;
}

uint64_t 
Timer::GetSnapshot()
{
	return mSnapshot;
}
