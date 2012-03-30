/*

    This file is a part of the JThread package, which contains some object-
    oriented thread wrappers for different thread implementations.

    Copyright (c) 2000-2011  Jori Liesenborgs (jori.liesenborgs@gmail.com)

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

#include "jthread.h"
#include "jmutexautolock.h"

#ifndef _WIN32_WCE
	#include <process.h>
#endif // _WIN32_WCE

namespace jthread
{

JThread::JThread()
{
	retval = NULL;
	mutexinit = false;
	running = false;
}

JThread::~JThread()
{
	Kill();
}

int JThread::Start()
{
	if (!mutexinit)
	{
		if (!runningmutex.IsInitialized())
		{
			if (runningmutex.Init() < 0)
				return ERR_JTHREAD_CANTINITMUTEX;
		}
		if (!continuemutex.IsInitialized())
		{
			if (continuemutex.Init() < 0)
				return ERR_JTHREAD_CANTINITMUTEX;
		}
		if (!continuemutex2.IsInitialized())
		{
			if (continuemutex2.Init() < 0)
				return ERR_JTHREAD_CANTINITMUTEX;
		}		mutexinit = true;
	}
	
	runningmutex.Lock();
	if (running)
	{
		runningmutex.Unlock();
		return ERR_JTHREAD_ALREADYRUNNING;
	}
	runningmutex.Unlock();
	
	continuemutex.Lock();
#ifndef _WIN32_WCE
	threadhandle = (HANDLE)_beginthreadex(NULL,0,TheThread,this,0,&threadid);
#else
	threadhandle = CreateThread(NULL,0,TheThread,this,0,&threadid);
#endif // _WIN32_WCE
	if (threadhandle == NULL)
	{
		continuemutex.Unlock();
		return ERR_JTHREAD_CANTSTARTTHREAD;
	}
	
	/* Wait until 'running' is set */

	runningmutex.Lock();			
	while (!running)
	{
		runningmutex.Unlock();
		Sleep(1);
		runningmutex.Lock();
	}
	runningmutex.Unlock();
	
	continuemutex.Unlock();
	
	continuemutex2.Lock();
	continuemutex2.Unlock();
		
	return 0;
}

int JThread::Kill()
{
	runningmutex.Lock();			
	if (!running)
	{
		runningmutex.Unlock();
		return ERR_JTHREAD_NOTRUNNING;
	}
	TerminateThread(threadhandle,0);
	CloseHandle(threadhandle);
	running = false;
	runningmutex.Unlock();
	return 0;
}

bool JThread::IsRunning()
{
	bool r;
	
	runningmutex.Lock();			
	r = running;
	runningmutex.Unlock();
	return r;
}

void *JThread::GetReturnValue()
{
	JMutexAutoLock autolock(runningmutex);
	void *val;
	
	if (running)
		val = NULL;
	else
		val = retval;
	return val;
}

#ifndef _WIN32_WCE
UINT __stdcall JThread::TheThread(void *param)
#else
DWORD WINAPI JThread::TheThread(void *param)
#endif // _WIN32_WCE
{
	JThread *jthread;
	void *ret;

	jthread = (JThread *)param;
	
	jthread->continuemutex2.Lock();
	jthread->runningmutex.Lock();
	jthread->running = true;
	jthread->runningmutex.Unlock();
	
	jthread->continuemutex.Lock();
	jthread->continuemutex.Unlock();
	
	ret = jthread->Thread();
	
	jthread->runningmutex.Lock();
	jthread->running = false;
	jthread->retval = ret;
	CloseHandle(jthread->threadhandle);
	jthread->runningmutex.Unlock();
	return 0;		
}

void JThread::ThreadStarted()
{
	continuemutex2.Unlock();
}

} // end namespace

