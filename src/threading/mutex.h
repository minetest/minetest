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

#ifndef THREADING_MUTEX_H
#define THREADING_MUTEX_H

#include "threads.h"

#if USE_CPP11_MUTEX
	#include <mutex>
	using Mutex = std::mutex;
	using RecursiveMutex = std::recursive_mutex;
#else

#if USE_WIN_MUTEX
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <pthread.h>
#endif

#include "util/basic_macros.h"

class Mutex
{
public:
	Mutex();
	~Mutex();
	void lock();
	void unlock();

	bool try_lock();

protected:
	Mutex(bool recursive);
	void init_mutex(bool recursive);
private:
#if USE_WIN_MUTEX
	CRITICAL_SECTION mutex;
#else
	pthread_mutex_t mutex;
#endif

	DISABLE_CLASS_COPY(Mutex);
};

class RecursiveMutex : public Mutex
{
public:
	RecursiveMutex();

	DISABLE_CLASS_COPY(RecursiveMutex);
};

#endif // C++11

#endif
