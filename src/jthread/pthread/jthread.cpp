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

#include "jthread/jthread.h"
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#define UNUSED(expr) do { (void)(expr); } while (0)

JThread::JThread()
{
	retval = NULL;
	requeststop = false;
	running = false;
	started = false;
}

JThread::~JThread()
{
	Kill();
}

void JThread::Wait() {
	void* status;
	if (started) {
		int pthread_join_retval = pthread_join(threadid,&status);
		assert(pthread_join_retval == 0);
		UNUSED(pthread_join_retval);
		started = false;
	}
}

int JThread::Start()
{
	int status;

	if (running)
	{
		return ERR_JTHREAD_ALREADYRUNNING;
	}
	requeststop = false;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

	continuemutex.Lock();
	status = pthread_create(&threadid,&attr,TheThread,this);
	pthread_attr_destroy(&attr);
	if (status != 0)
	{
		continuemutex.Unlock();
		return ERR_JTHREAD_CANTSTARTTHREAD;
	}

	/* Wait until 'running' is set */

	while (!running)
	{
		struct timespec req,rem;

		req.tv_sec = 0;
		req.tv_nsec = 1000000;
		nanosleep(&req,&rem);
	}
	started = true;

	continuemutex.Unlock();

	continuemutex2.Lock();
	continuemutex2.Unlock();
	return 0;
}

int JThread::Kill()
{
	void* status;
	if (!running)
	{
		if (started) {
			int pthread_join_retval = pthread_join(threadid,&status);
			assert(pthread_join_retval == 0);
			UNUSED(pthread_join_retval);
			started = false;
		}
		return ERR_JTHREAD_NOTRUNNING;
	}
#ifdef __ANDROID__
	pthread_kill(threadid, SIGKILL);
#else
	pthread_cancel(threadid);
#endif
	if (started) {
		int pthread_join_retval = pthread_join(threadid,&status);
		assert(pthread_join_retval == 0);
		UNUSED(pthread_join_retval);
		started = false;
	}
	running = false;
	return 0;
}

void *JThread::GetReturnValue()
{
	void *val;

	if (running) {
		val = NULL;
	} else {
		val = retval;
	}

	return val;
}

bool JThread::IsSameThread()
{
	return pthread_equal(pthread_self(), threadid);
}

void *JThread::TheThread(void *param)
{
	JThread *jthread;
	void *ret;

	jthread = (JThread *)param;

	jthread->continuemutex2.Lock();
	jthread->running = true;

	jthread->continuemutex.Lock();
	jthread->continuemutex.Unlock();

	ret = jthread->Thread();

	jthread->running = false;

	return NULL;
}

void JThread::ThreadStarted()
{
	continuemutex2.Unlock();
}

