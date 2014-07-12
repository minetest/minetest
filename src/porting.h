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

#ifdef _WIN32
	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
	#endif
	#define _WIN32_WINNT 0x0501 // We need to do this before any other headers
		// because those might include sdkddkver.h which defines _WIN32_WINNT if not already set
#endif

#include <string>
#include "irrlicht.h"
#include "irrlichttypes.h" // u32
#include "irrlichttypes_extrabloated.h"
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

#if defined(__APPLE__)
	#include <mach-o/dyld.h>
	#include <CoreFoundation/CoreFoundation.h>
#endif

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

void initIrrlicht(irr::IrrlichtDevice * );

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
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

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
		// from http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
		clock_serv_t cclock;
		mach_timespec_t mts;
		host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
		clock_get_time(cclock, &mts);
		mach_port_deallocate(mach_task_self(), cclock);
		ts.tv_sec = mts.tv_sec;
		ts.tv_nsec = mts.tv_nsec;
#else
		clock_gettime(CLOCK_REALTIME, &ts);
#endif
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

#if defined(linux) || defined(__linux)
	#include <sys/prctl.h>

	inline void setThreadName(const char *name) {
		/* It would be cleaner to do this with pthread_setname_np,
		 * which was added to glibc in version 2.12, but some major
		 * distributions are still runing 2.11 and previous versions.
		 */
		prctl(PR_SET_NAME, name);
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	#include <pthread.h>
	#include <pthread_np.h>

	inline void setThreadName(const char *name) {
		pthread_set_name_np(pthread_self(), name);
	}
#elif defined(__NetBSD__)
	#include <pthread.h>

	inline void setThreadName(const char *name) {
		pthread_setname_np(pthread_self(), name);
	}
#elif defined(_MSC_VER)
	typedef struct tagTHREADNAME_INFO {
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

	inline void setThreadName(const char *name) {
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = name;
		info.dwThreadID = -1;
		info.dwFlags = 0;
		__try {
			RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR *) &info);
		} __except (EXCEPTION_CONTINUE_EXECUTION) {}
	}
#elif defined(__APPLE__)
	#include <pthread.h>

	inline void setThreadName(const char *name) {
		pthread_setname_np(name);
	}
#elif defined(_WIN32)
	inline void setThreadName(const char* name) {}
#else
	#warning "Unrecognized platform, thread names will not be available."
	inline void setThreadName(const char* name) {}
#endif

#ifndef SERVER
float getDisplayDensity();

v2u32 getDisplaySize();
v2u32 getWindowSize();
#endif

} // namespace porting

#ifdef __ANDROID__
#include "porting_android.h"
#endif

#endif // PORTING_HEADER

