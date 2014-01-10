/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "../jthread/jthread.h"
#include "../jthread/jmutex.h"
#include "../jthread/jmutexautolock.h"
#include "porting.h"

template<typename T>
class MutexedVariable
{
public:
	MutexedVariable(T value):
		m_value(value)
	{
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
	A single worker thread - multiple client threads queue framework.
*/
template<typename Key, typename T, typename Caller, typename CallerData>
class GetResult
{
public:
	Key key;
	T item;
	std::pair<Caller, CallerData> caller;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class ResultQueue: public MutexedQueue< GetResult<Key, T, Caller, CallerData> >
{
};

template<typename Caller, typename Data, typename Key, typename T>
class CallerInfo
{
public:
	Caller caller;
	Data data;
	ResultQueue< Key, T, Caller, Data>* dest;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class GetRequest
{
public:
	GetRequest()
	{
	}
	GetRequest(Key a_key)
	{
		key = a_key;
	}
	~GetRequest()
	{
	}
	
	Key key;
	std::list<CallerInfo<Caller, CallerData, Key, T> > callers;
};

/**
 * Notes for RequestQueue usage
 * @param Key unique key to identify a request for a specific resource
 * @param T ?
 * @param Caller unique id of calling thread
 * @param CallerData data passed back to caller
 */
template<typename Key, typename T, typename Caller, typename CallerData>
class RequestQueue
{
public:
	bool empty()
	{
		return m_queue.empty();
	}

	void add(Key key, Caller caller, CallerData callerdata,
			ResultQueue<Key, T, Caller, CallerData> *dest)
	{
		{
			JMutexAutoLock lock(m_queue.getMutex());

			/*
				If the caller is already on the list, only update CallerData
			*/
			for(typename std::list< GetRequest<Key, T, Caller, CallerData> >::iterator
					i = m_queue.getList().begin();
					i != m_queue.getList().end(); ++i)
			{
				GetRequest<Key, T, Caller, CallerData> &request = *i;

				if(request.key == key)
				{
					for(typename std::list< CallerInfo<Caller, CallerData, Key, T> >::iterator
							i = request.callers.begin();
							i != request.callers.end(); ++i)
					{
						CallerInfo<Caller, CallerData, Key, T> &ca = *i;
						if(ca.caller == caller)
						{
							ca.data = callerdata;
							return;
						}
					}
					CallerInfo<Caller, CallerData, Key, T> ca;
					ca.caller = caller;
					ca.data = callerdata;
					ca.dest = dest;
					request.callers.push_back(ca);
					return;
				}
			}
		}

		/*
			Else add a new request to the queue
		*/

		GetRequest<Key, T, Caller, CallerData> request;
		request.key = key;
		CallerInfo<Caller, CallerData, Key, T> ca;
		ca.caller = caller;
		ca.data = callerdata;
		ca.dest = dest;
		request.callers.push_back(ca);
		
		m_queue.push_back(request);
	}

	GetRequest<Key, T, Caller, CallerData> pop(unsigned int timeout_ms)
	{
		return m_queue.pop_front(timeout_ms);
	}

	GetRequest<Key, T, Caller, CallerData> pop()
	{
		return m_queue.pop_frontNoEx();
	}

	void pushResult(GetRequest<Key, T, Caller, CallerData> req,
					T res) {

		for(typename std::list< CallerInfo<Caller, CallerData, Key, T> >::iterator
				i = req.callers.begin();
				i != req.callers.end(); ++i)
		{
			CallerInfo<Caller, CallerData, Key, T> &ca = *i;

			GetResult<Key,T,Caller,CallerData> result;

			result.key = req.key;
			result.item = res;
			result.caller.first = ca.caller;
			result.caller.second = ca.data;

			ca.dest->push_back(result);
		}
	}

private:
	MutexedQueue< GetRequest<Key, T, Caller, CallerData> > m_queue;
};

#endif

