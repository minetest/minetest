/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef GETTIME_HEADER
#define GETTIME_HEADER

#include "irrlichttypes.h"
#include <ctime>
#include <string>

enum TimePrecision {
	PRECISION_SECONDS = 0,
	PRECISION_MILLI,
	PRECISION_MICRO,
	PRECISION_NANO
};

/*
	Resolution is 10-20ms.
	Remember to check for overflows.
	Overflow can occur at any value higher than 10000000.
*/
#ifdef _WIN32 // Windows
	// This is to prevent accidential including of winsock2.h after windows.h
	#include "winsock2.h"
	#include "windows.h"

	inline u32 getTimeS()
	{
		return GetTickCount() / 1000;
	}

	inline u32 getTimeMs()
	{
		return GetTickCount();
	}

	inline u32 getTimeUs()
	{
		LARGE_INTEGER freq, t;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&t);
		return (double)(t.QuadPart) / ((double)(freq.QuadPart) / 1000000.0);
	}

	inline u32 getTimeNs()
	{
		LARGE_INTEGER freq, t;
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&t);
		return (double)(t.QuadPart) / ((double)(freq.QuadPart) / 1000000000.0);
	}

#else // Posix
	#include <sys/time.h>
	#include <time.h>
	#if defined(__MACH__) && defined(__APPLE__)
		#include <mach/clock.h>
		#include <mach/mach.h>
	#endif

	inline void _os_get_clock(struct timespec *ts)
	{
#if defined(__MACH__) && defined(__APPLE__)
	// from http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
	// OS X does not have clock_gettime, use clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		ts->tv_sec = mts.tv_sec;
		ts->tv_nsec = mts.tv_nsec;
#elif defined(CLOCK_MONOTONIC_RAW)
		clock_gettime(CLOCK_MONOTONIC_RAW, ts);
#elif defined(_POSIX_MONOTONIC_CLOCK)
		clock_gettime(CLOCK_MONOTONIC, ts);
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		TIMEVAL_TO_TIMESPEC(&tv, ts);
#endif // defined(__MACH__) && defined(__APPLE__)
	}

	// Note: these clock functions do not return wall time, but
	// generally a clock that starts at 0 when the process starts.
	inline u32 getTimeS()
	{
		struct timespec ts;
		_os_get_clock(&ts);
		return ts.tv_sec;
	}

	inline u32 getTimeMs()
	{
		struct timespec ts;
		_os_get_clock(&ts);
		return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	}

	inline u32 getTimeUs()
	{
		struct timespec ts;
		_os_get_clock(&ts);
		return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
	}

	inline u32 getTimeNs()
	{
		struct timespec ts;
		_os_get_clock(&ts);
		return ts.tv_sec * 1000000000 + ts.tv_nsec;
	}

	/*#include <sys/timeb.h>
	inline u32 getTimeMs()
	{
		struct timeb tb;
		ftime(&tb);
		return tb.time * 1000 + tb.millitm;
	}*/
#endif


inline u32 getTime(TimePrecision prec)
{
	switch (prec) {
		case PRECISION_SECONDS:
			return getTimeS();
		case PRECISION_MILLI:
			return getTimeMs();
		case PRECISION_MICRO:
			return getTimeUs();
		case PRECISION_NANO:
			return getTimeNs();
	}
	return 0;
}

/**
 * Delta calculation function taking two 32bit arguments.
 * @param old_time_ms old time for delta calculation (order is relevant!)
 * @param new_time_ms new time for delta calculation (order is relevant!)
 * @return positive 32bit delta value
 */
inline u32 getDeltaMs(u32 old_time_ms, u32 new_time_ms)
{
	if (new_time_ms >= old_time_ms) {
		return (new_time_ms - old_time_ms);
	} else {
		return (old_time_ms - new_time_ms);
	}
}

/*
	Timestamp stuff
*/
inline std::string getTimestamp()
{
	time_t t = time(NULL);
	// This is not really thread-safe but it won't break anything
	// except its own output, so just go with it.
	struct tm *tm = localtime(&t);
	char cs[20]; //YYYY-MM-DD HH:MM:SS + '\0'
	strftime(cs, 20, "%Y-%m-%d %H:%M:%S", tm);
	return cs;
}

#endif
