#pragma once

#if defined(ENABLE_PROFILING)
#define PROFILE_START(x)	(x).StartTimer();
#define PROFILE_STOP(x)		(x).StopTimer();
#define PROFILE_RESET(x)	(x).Reset();
#else
#define PROFILE_START(x)
#define PROFILE_STOP(x)
#define PROFILE_RESET(x)
#endif

uint64_t GetTimeUs();
uint64_t GetTimeMs();
uint64_t GetTimeUTC();
#if defined _WIN32
uint64_t FileTimeToUTCTime( const FILETIME &ft );
#endif // defined _WIN32

class Timer
{
public:
	Timer();
	void Reset();
	uint64_t StartTimer();
	uint64_t StopTimer();
	uint64_t GetElapsed();
	uint64_t GetElapsedTotal();
	uint64_t Snapshot(bool clear);
	uint64_t GetSnapshot();
protected:
	uint64_t mStartTime;
	uint64_t mStopTime;
	uint64_t mElapsed;
	uint64_t mElapsedTotal;
	uint64_t mSnapshot;
	bool	 mRunning;
};
