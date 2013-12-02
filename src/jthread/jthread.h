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

#ifndef JTHREAD_H

#define JTHREAD_H

#include "jthread/jmutex.h"

#define ERR_JTHREAD_CANTINITMUTEX						-1
#define ERR_JTHREAD_CANTSTARTTHREAD						-2
#define ERR_JTHREAD_THREADFUNCNOTSET						-3
#define ERR_JTHREAD_NOTRUNNING							-4
#define ERR_JTHREAD_ALREADYRUNNING						-5

class JThread
{
public:
	JThread();
	virtual ~JThread();
	int Start();
	void Stop();
	int Kill();
	virtual void *Thread() = 0;
	bool IsRunning();
	bool StopRequested();
	void *GetReturnValue();
	bool IsSameThread();

	/*
	 * Wait for thread to finish
	 * Note: this does not stop a thread you have to do this on your own
	 * WARNING: never ever call this on a thread not started or already killed!
	 */
	void Wait();
protected:
	void ThreadStarted();
private:

#if (defined(WIN32) || defined(_WIN32_WCE))
#ifdef _WIN32_WCE
	DWORD threadid;
	static DWORD WINAPI TheThread(void *param);
#else
	static UINT __stdcall TheThread(void *param);
	UINT threadid;
#endif // _WIN32_WCE
	HANDLE threadhandle;
#else // pthread type threads
	static void *TheThread(void *param);

	pthread_t threadid;

	bool started;
#endif // WIN32
	void *retval;
	bool running;
	bool requeststop;

	JMutex runningmutex;
	JMutex continuemutex,continuemutex2;
};

#endif // JTHREAD_H

