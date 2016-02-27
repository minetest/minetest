/*
This file is a part of the JThread package, which contains some object-
oriented thread wrappers for different thread implementations.

Copyright (c) 2000-2006  Jori Liesenborgs (jori.liesenborgs@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

// Windows std::mutex is much slower than the critical section API
#if __cplusplus < 201103L || defined(_WIN32)

#include "threading/mutex.h"

#ifndef _WIN32
	#include <cassert>
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

Mutex::Mutex()
{
	init_mutex(false);
}


Mutex::Mutex(bool recursive)
{
	init_mutex(recursive);
}

void Mutex::init_mutex(bool recursive)
{
#ifdef _WIN32
	// Windows critical sections are recursive by default
	UNUSED(recursive);

	InitializeCriticalSection(&mutex);
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

	if (recursive)
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	int ret = pthread_mutex_init(&mutex, &attr);
	assert(!ret);
	UNUSED(ret);

	pthread_mutexattr_destroy(&attr);
#endif
}

Mutex::~Mutex()
{
#ifdef _WIN32
	DeleteCriticalSection(&mutex);
#else
	int ret = pthread_mutex_destroy(&mutex);
	assert(!ret);
	UNUSED(ret);
#endif
}

void Mutex::lock()
{
#ifdef _WIN32
	EnterCriticalSection(&mutex);
#else
	int ret = pthread_mutex_lock(&mutex);
	assert(!ret);
	UNUSED(ret);
#endif
}

void Mutex::unlock()
{
#ifdef _WIN32
	LeaveCriticalSection(&mutex);
#else
	int ret = pthread_mutex_unlock(&mutex);
	assert(!ret);
	UNUSED(ret);
#endif
}

RecursiveMutex::RecursiveMutex()
	: Mutex(true)
{}

#endif

