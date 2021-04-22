// Timer.cpp: implementation of the Timer class.
//
//////////////////////////////////////////////////////////////////////
#include "PrecompileHeaders.h"
#include "Timer.h"

#ifdef _WIN32
float Timer::rfreq = 1.0f;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Timer::Timer()
#ifdef _WIN32
: startTime(0)
#endif
{
}

//
void Timer::Init()
{
#ifdef _WIN32
	if (rfreq < 1.0f) {
		return;	
	}
	LogMsg("Timer init...\n");
	//
	SetLastError(0);
	//
	LARGE_INTEGER Frequency;
	//
	if (!QueryPerformanceFrequency(&Frequency)) {
		//
		SetLastError(0);
	}
	//
	rfreq = 1.0f / Frequency.QuadPart;
	LogMsg(" Timer init completed.\n");
#endif
}

//
float Timer::AbsoluteDiffTime()
{
#ifdef _WIN32
	LARGE_INTEGER temp;
	// Get counter
	QueryPerformanceCounter(&temp);
	float dt = (temp.QuadPart - absoluteStartTime) * rfreq;
	return dt;
#else
	return 0;
#endif
}
