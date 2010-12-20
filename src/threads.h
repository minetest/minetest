/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef THREADS_HEADER
#define THREADS_HEADER

#include <jmutex.h>

#if (defined(WIN32) || defined(_WIN32_WCE))
typedef DWORD threadid_t;
#define __NORETURN __declspec(noreturn)
#define __FUNCTION_NAME __FUNCTION__
#else
typedef pthread_t threadid_t;
#define __NORETURN __attribute__ ((__noreturn__))
#define __FUNCTION_NAME __PRETTY_FUNCTION__
#endif

inline threadid_t get_current_thread_id()
{
#if (defined(WIN32) || defined(_WIN32_WCE))
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

#endif

