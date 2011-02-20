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

#ifndef JMUTEX_H

#define JMUTEX_H

#if (defined(WIN32) || defined(_WIN32_WCE))
	#ifndef _WIN32_WCE
		#include <process.h>
	#endif // _WIN32_WCE
	#include <winsock2.h>
	#include <windows.h>
	// CriticalSection is way faster than the alternative
	#define JMUTEX_CRITICALSECTION
#else // using pthread
	#include <pthread.h>
#endif // WIN32

#define ERR_JMUTEX_ALREADYINIT						-1
#define ERR_JMUTEX_NOTINIT						-2
#define ERR_JMUTEX_CANTCREATEMUTEX					-3

class JMutex
{
public:
	JMutex();
	~JMutex();
	int Init();
	int Lock();
	int Unlock();
	bool IsInitialized() 						{ return initialized; }
private:
#if (defined(WIN32) || defined(_WIN32_WCE))
#ifdef JMUTEX_CRITICALSECTION
	CRITICAL_SECTION mutex;
#else // Use standard mutex
	HANDLE mutex;
#endif // JMUTEX_CRITICALSECTION
#else // pthread mutex
	pthread_mutex_t mutex;
#endif // WIN32
	bool initialized;
};

#endif // JMUTEX_H
