// Timer.h: interface for the Timer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __TIMER_H__
#define __TIMER_H__

#include "Platform.h"

#ifndef _WIN32
#include <sys/time.h>
#elif defined(__ANDROID__) || defined(__linux__) || defined(BSD)
#define USE_CLOCK_GETTIME
#endif

// Timer implememtation
class Timer  {
private:
#ifdef _WIN32
	// 
	static float rfreq;
	// current time
	uint64 startTime;
#else
#ifdef USE_CLOCK_GETTIME
	typedef timespec timeType_t;
#else
	typedef timeval timeType_t;
#endif
	timeType_t startTime;
#endif
	// absolute start Time
	uint64 absoluteStartTime;
#ifdef _WIN32
	// calculate delta time
	INLINE float CalcDeltaTime(uint64& start)
	{
		LARGE_INTEGER temp;
		// Get counter
		QueryPerformanceCounter(&temp);
		float dt = (temp.QuadPart - start) * rfreq;
		start = temp.QuadPart;
#else
		// calculate delta time
		INLINE float CalcDeltaTime(timeType_t& start)
		{
			timeType_t tm;
#ifdef USE_CLOCK_GETTIME
			clock_gettime(CLOCK_REALTIME, &tm);
			float dt = tm.tv_sec - start.tv_sec  + (tm.tv_nsec - start.tv_nsec) * 0.000000001f;
#else
			gettimeofday(&tm, NULL);
			float dt = tm.tv_sec - start.tv_sec  + (tm.tv_usec - start.tv_usec) * 0.000001f;
#endif
			start = tm;
#endif
			return dt;
		}
		uint64 CalcDeltaTimeInt(uint64& start)
		{
#ifdef _WIN32
			LARGE_INTEGER temp;
			// Get counter
			QueryPerformanceCounter(&temp);
			uint64 dt = (temp.QuadPart - start);
			start = temp.QuadPart;
			return dt;
#else
			return 0;
#endif
		}
#ifdef _WIN32
		INLINE void RestartTime(uint64& start)
		{
			LARGE_INTEGER temp;
			// 
			QueryPerformanceCounter(&temp);
			// 
			start = temp.QuadPart;
#else
		INLINE void RestartTime(timeType_t& start)
		{
#ifdef USE_CLOCK_GETTIME
			clock_gettime(CLOCK_REALTIME, &start);
#else
			gettimeofday(&start, NULL);
#endif
#endif
		}
		public:
			// timer init
			static void Init();
			//
			void RestartTimer()
			{
				RestartTime(startTime);
			}
			// Restart timer for absolute time
			void RestartAbsoluteTimer()
			{
#ifdef _WIN32
				RestartTime(absoluteStartTime);
#endif
			}
			// Get delta time
			float DiffTime()
			{
				return CalcDeltaTime(startTime);
			}
			//
			uint64 DiffTimeInt()
			{
#ifdef _WIN32
				return CalcDeltaTimeInt(startTime);
#else
				return 0;
#endif
			}
			// Get Absolute Start Time
			float AbsoluteDiffTime();
			Timer();
		};
		
#endif // __TIMER_H__
