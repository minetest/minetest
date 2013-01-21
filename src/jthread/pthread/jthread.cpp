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

#include "jthread.h"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

static int JThreadPrioToPThreadPrio(JThreadPriority toconvert) {
	switch (toconvert) {
		case PRIO_00:
			return 0;
		case PRIO_01:
			return 10;
		case PRIO_02:
			return 20;
		case PRIO_03:
			return 30;
		case PRIO_04:
			return 40;
		case PRIO_05:
			return 50;
		case PRIO_06:
			return 60;
		case PRIO_07:
			return 70;
		case PRIO_08:
			return 80;
		case PRIO_09:
			return 90;
		default:
			return 0;
	}
}

JThread::JThread()
{
	retval = NULL;
	mutexinit = false;
	running = false;
	m_SchedPolicy = SCHED_DEFAULT;
	m_ThreadPriority = PRIO_DEFAULT;
}

JThread::~JThread()
{
	Kill();
}

int JThread::Start()
{
	int status;

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
		}
		mutexinit = true;
	}
	
	runningmutex.Lock();
	if (running)
	{
		runningmutex.Unlock();
		return ERR_JTHREAD_ALREADYRUNNING;
	}
	runningmutex.Unlock();
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	struct sched_param toset;
	
	if (m_SchedPolicy == ROUND_ROBIN) {
		pthread_attr_setschedpolicy(&attr,SCHED_RR);
	}
	else if (m_SchedPolicy == FIFO) {
		pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
	}

	if (m_ThreadPriority != PRIO_DEFAULT) {
		toset.sched_priority = JThreadPrioToPThreadPrio(m_ThreadPriority);
		pthread_attr_setschedparam(&attr,&toset);
	}

	continuemutex.Lock();
	status = pthread_create(&threadid,&attr,TheThread,this);
	pthread_attr_destroy(&attr);
	if (status != 0)
	{
		continuemutex.Unlock();
		return ERR_JTHREAD_CANTSTARTTHREAD;
	}
	
	/* Wait until 'running' is set */
	
	runningmutex.Lock();
	while (!running)
	{
		runningmutex.Unlock();
		
		struct timespec req,rem;

		req.tv_sec = 0;
		req.tv_nsec = 1000000;
		nanosleep(&req,&rem);

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
	pthread_cancel(threadid);
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
	void *val;
	
	runningmutex.Lock();
	if (running)
		val = NULL;
	else
		val = retval;
	runningmutex.Unlock();
	return val;
}

void *JThread::TheThread(void *param)
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
	jthread->runningmutex.Unlock();

	return NULL;
}

void JThread::ThreadStarted()
{
	continuemutex2.Unlock();
}

SchedulingPolicy JThread::GetSchedulingPolicy() {
	return m_SchedPolicy;
}

bool JThread::SetSchedulingPolicy(SchedulingPolicy policy) {
	if (IsRunning()) {
		int cur_policy;
		struct sched_param sched_params;

		pthread_getschedparam(threadid,&cur_policy,&sched_params);

		if (policy == ROUND_ROBIN)
			cur_policy = SCHED_RR;
		else
			cur_policy = SCHED_FIFO;
		pthread_getschedparam(threadid,&cur_policy,&sched_params);
	}
	m_SchedPolicy = policy;

	return true;
}

JThreadPriority JThread::GetThreadPriority() {
	return m_ThreadPriority;
}

bool JThread::SetThreadPriority(JThreadPriority prio) {
	if (IsRunning()) {
		int policy;
		struct sched_param sched_params;

		pthread_getschedparam(threadid,&policy,&sched_params);
		sched_params.sched_priority = JThreadPrioToPThreadPrio(prio);
		pthread_getschedparam(threadid,&policy,&sched_params);
		}
	m_ThreadPriority = prio;

	return true;
}

JThread::JThread(SchedulingPolicy policy,JThreadPriority prio) {
	retval = NULL;
	mutexinit = false;
	running = false;
	m_SchedPolicy = policy;
	m_ThreadPriority = prio;
}

