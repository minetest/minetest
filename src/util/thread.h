/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef UTIL_THREAD_HEADER
#define UTIL_THREAD_HEADER

#include "../irrlichttypes.h"
#include <jthread.h>
#include <jmutex.h>
#include <jmutexautolock.h>

template<typename T>
class MutexedVariable
{
public:
	MutexedVariable(T value):
		m_value(value)
	{
		m_mutex.Init();
	}

	T get()
	{
		JMutexAutoLock lock(m_mutex);
		return m_value;
	}

	void set(T value)
	{
		JMutexAutoLock lock(m_mutex);
		m_value = value;
	}
	
	// You'll want to grab this in a SharedPtr
	JMutexAutoLock * getLock()
	{
		return new JMutexAutoLock(m_mutex);
	}
	
	// You pretty surely want to grab the lock when accessing this
	T m_value;

private:
	JMutex m_mutex;
};

/*
	A base class for simple background thread implementation
*/

class SimpleThread : public JThread
{
	bool run;
	JMutex run_mutex;

public:

	SimpleThread():
		JThread(),
		run(true)
	{
		run_mutex.Init();
	}

	virtual ~SimpleThread()
	{}

	virtual void * Thread() = 0;

	bool getRun()
	{
		JMutexAutoLock lock(run_mutex);
		return run;
	}
	void setRun(bool a_run)
	{
		JMutexAutoLock lock(run_mutex);
		run = a_run;
	}

	void stop()
	{
		setRun(false);
		while(IsRunning())
			sleep_ms(100);
	}
};

/*
	A single worker thread - multiple client threads queue framework.
*/

template<typename Caller, typename Data>
class CallerInfo
{
public:
	Caller caller;
	Data data;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class GetResult
{
public:
	Key key;
	T item;
	core::list<CallerInfo<Caller, CallerData> > callers;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class ResultQueue: public MutexedQueue< GetResult<Key, T, Caller, CallerData> >
{
};

template<typename Key, typename T, typename Caller, typename CallerData>
class GetRequest
{
public:
	GetRequest()
	{
		dest = NULL;
	}
	GetRequest(ResultQueue<Key,T, Caller, CallerData> *a_dest)
	{
		dest = a_dest;
	}
	GetRequest(ResultQueue<Key,T, Caller, CallerData> *a_dest,
			Key a_key)
	{
		dest = a_dest;
		key = a_key;
	}
	~GetRequest()
	{
	}
	
	Key key;
	ResultQueue<Key, T, Caller, CallerData> *dest;
	core::list<CallerInfo<Caller, CallerData> > callers;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class RequestQueue
{
public:
	u32 size()
	{
		return m_queue.size();
	}

	void add(Key key, Caller caller, CallerData callerdata,
			ResultQueue<Key, T, Caller, CallerData> *dest)
	{
		JMutexAutoLock lock(m_queue.getMutex());
		
		/*
			If the caller is already on the list, only update CallerData
		*/
		for(typename core::list< GetRequest<Key, T, Caller, CallerData> >::Iterator
				i = m_queue.getList().begin();
				i != m_queue.getList().end(); i++)
		{
			GetRequest<Key, T, Caller, CallerData> &request = *i;

			if(request.key == key)
			{
				for(typename core::list< CallerInfo<Caller, CallerData> >::Iterator
						i = request.callers.begin();
						i != request.callers.end(); i++)
				{
					CallerInfo<Caller, CallerData> &ca = *i;
					if(ca.caller == caller)
					{
						ca.data = callerdata;
						return;
					}
				}
				CallerInfo<Caller, CallerData> ca;
				ca.caller = caller;
				ca.data = callerdata;
				request.callers.push_back(ca);
				return;
			}
		}

		/*
			Else add a new request to the queue
		*/

		GetRequest<Key, T, Caller, CallerData> request;
		request.key = key;
		CallerInfo<Caller, CallerData> ca;
		ca.caller = caller;
		ca.data = callerdata;
		request.callers.push_back(ca);
		request.dest = dest;
		
		m_queue.getList().push_back(request);
	}

	GetRequest<Key, T, Caller, CallerData> pop(bool wait_if_empty=false)
	{
		return m_queue.pop_front(wait_if_empty);
	}

private:
	MutexedQueue< GetRequest<Key, T, Caller, CallerData> > m_queue;
};

#endif

