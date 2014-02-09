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

/*
	Random portability stuff
*/

#ifndef PORTING_HEADER
#define PORTING_HEADER

#include <string>
#include "irrlichttypes.h" // u32
#include "debug.h"
#include "constants.h"
#include "gettime.h"
#include "threads.h"

#ifdef _MSC_VER
	#define SWPRINTF_CHARSTRING L"%S"
#else
	#define SWPRINTF_CHARSTRING L"%s"
#endif

//currently not needed
//template<typename T> struct alignment_trick { char c; T member; };
//#define ALIGNOF(type) offsetof (alignment_trick<type>, member)

#ifdef _WIN32
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#include <windows.h>
	
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#include <stdint.h> //for uintptr_t
	
	#if (defined(linux) || defined(__linux)) && !defined(_GNU_SOURCE)
		#define _GNU_SOURCE
	#endif

	#include <sched.h>

	#ifdef __FreeBSD__
		#include <pthread_np.h>
		typedef cpuset_t cpu_set_t;
	#elif defined(__sun) || defined(sun)
		#include <sys/types.h>
		#include <sys/processor.h>
	#elif defined(_AIX)
		#include <sys/processor.h>
	#elif __APPLE__
		#include <mach/mach_init.h>
		#include <mach/thread_policy.h>
	#endif

	#define sleep_ms(x) usleep(x*1000)
	
	#define THREAD_PRIORITY_LOWEST       0
	#define THREAD_PRIORITY_BELOW_NORMAL 1
	#define THREAD_PRIORITY_NORMAL       2
	#define THREAD_PRIORITY_ABOVE_NORMAL 3
	#define THREAD_PRIORITY_HIGHEST      4
#endif

#ifdef _MSC_VER
	#define ALIGNOF(x) __alignof(x)
	#define strtok_r(x, y, z) strtok_s(x, y, z)
	#define strtof(x, y) (float)strtod(x, y)
	#define strtoll(x, y, z) _strtoi64(x, y, z)
	#define strtoull(x, y, z) _strtoui64(x, y, z)
	#define strcasecmp(x, y) stricmp(x, y)
	#define strncasecmp(x, y, n) strnicmp(x, y, n)
#else
	#define ALIGNOF(x) __alignof__(x)
#endif

#ifdef __MINGW32__
	#define strtok_r(x, y, z) mystrtok_r(x, y, z)
#endif

// strlcpy is missing from glibc.  thanks a lot, drepper.
// strlcpy is also missing from AIX and HP-UX because they aim to be weird.
// We can't simply alias strlcpy to MSVC's strcpy_s, since strcpy_s by
// default raises an assertion error and aborts the program if the buffer is
// too small.
#if defined(__FreeBSD__) || defined(__NetBSD__)    || \
	defined(__OpenBSD__) || defined(__DragonFly__) || \
	defined(__APPLE__)   ||                           \
	defined(__sun)       || defined(sun)           || \
	defined(__QNX__)     || defined(__QNXNTO__)
	#define HAVE_STRLCPY
#endif

// So we need to define our own.
#ifndef HAVE_STRLCPY
	#define strlcpy(d, s, n) mystrlcpy(d, s, n)
#endif

#define PADDING(x, y) ((ALIGNOF(y) - ((uintptr_t)(x) & (ALIGNOF(y) - 1))) & (ALIGNOF(y) - 1))

namespace porting
{

/*
	Signal handler (grabs Ctrl-C on POSIX systems)
*/

void signal_handler_init(void);
// Returns a pointer to a bool.
// When the bool is true, program should quit.
bool * signal_handler_killstatus(void);

/*
	Path of static data directory.
*/
extern std::string path_share;

/*
	Directory for storing user data. Examples:
	Windows: "C:\Documents and Settings\user\Application Data\<PROJECT_NAME>"
	Linux: "~/.<PROJECT_NAME>"
	Mac: "~/Library/Application Support/<PROJECT_NAME>"
*/
extern std::string path_user;

/*
	Get full path of stuff in data directory.
	Example: "stone.png" -> "../data/stone.png"
*/
std::string getDataPath(const char *subpath);

/*
	Initialize path_share and path_user.
*/
void initializePaths();

/*
	Get number of online processors in the system.
*/
int getNumberOfProcessors();

/*
	Set a thread's affinity to a particular processor.
*/
bool threadBindToProcessor(threadid_t tid, int pnumber);

/*
	Set a thread's priority.
*/
bool threadSetPriority(threadid_t tid, int prio);

/*
	Return system information
	e.g. "Linux/3.12.7 x86_64"
*/
std::string get_sysinfo();

/*
	Resolution is 10-20ms.
	Remember to check for overflows.
	Overflow can occur at any value higher than 10000000.
*/
#ifdef _WIN32 // Windows
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif
	#include <windows.h>
	
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
	
	inline u32 getTimeS()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec;
	}
	
	inline u32 getTimeMs()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}
	
	inline u32 getTimeUs()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000000 + tv.tv_usec;
	}
	
	inline u32 getTimeNs()
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
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


} // namespace porting

#endif // PORTING_HEADER

