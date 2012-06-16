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

#ifndef UTIL_CONTAINER_HEADER
#define UTIL_CONTAINER_HEADER

#include "../irrlichttypes.h"
#include <jmutex.h>
#include <jmutexautolock.h>
#include "../porting.h" // For sleep_ms

/*
	Queue with unique values with fast checking of value existence
*/

template<typename Value>
class UniqueQueue
{
public:
	
	/*
		Does nothing if value is already queued.
		Return value:
			true: value added
			false: value already exists
	*/
	bool push_back(Value value)
	{
		// Check if already exists
		if(m_map.find(value) != NULL)
			return false;

		// Add
		m_map.insert(value, 0);
		m_list.push_back(value);
		
		return true;
	}

	Value pop_front()
	{
		typename core::list<Value>::Iterator i = m_list.begin();
		Value value = *i;
		m_map.remove(value);
		m_list.erase(i);
		return value;
	}

	u32 size()
	{
		assert(m_list.size() == m_map.size());
		return m_list.size();
	}

private:
	core::map<Value, u8> m_map;
	core::list<Value> m_list;
};

#if 1
template<typename Key, typename Value>
class MutexedMap
{
public:
	MutexedMap()
	{
		m_mutex.Init();
		assert(m_mutex.IsInitialized());
	}
	
	void set(const Key &name, const Value &value)
	{
		JMutexAutoLock lock(m_mutex);

		m_values[name] = value;
	}
	
	bool get(const Key &name, Value *result)
	{
		JMutexAutoLock lock(m_mutex);

		typename core::map<Key, Value>::Node *n;
		n = m_values.find(name);

		if(n == NULL)
			return false;
		
		if(result != NULL)
			*result = n->getValue();
			
		return true;
	}

private:
	core::map<Key, Value> m_values;
	JMutex m_mutex;
};
#endif

/*
	Generates ids for comparable values.
	Id=0 is reserved for "no value".

	Is fast at:
	- Returning value by id (very fast)
	- Returning id by value
	- Generating a new id for a value

	Is not able to:
	- Remove an id/value pair (is possible to implement but slow)
*/
template<typename T>
class MutexedIdGenerator
{
public:
	MutexedIdGenerator()
	{
		m_mutex.Init();
		assert(m_mutex.IsInitialized());
	}
	
	// Returns true if found
	bool getValue(u32 id, T &value)
	{
		if(id == 0)
			return false;
		JMutexAutoLock lock(m_mutex);
		if(m_id_to_value.size() < id)
			return false;
		value = m_id_to_value[id-1];
		return true;
	}
	
	// If id exists for value, returns the id.
	// Otherwise generates an id for the value.
	u32 getId(const T &value)
	{
		JMutexAutoLock lock(m_mutex);
		typename core::map<T, u32>::Node *n;
		n = m_value_to_id.find(value);
		if(n != NULL)
			return n->getValue();
		m_id_to_value.push_back(value);
		u32 new_id = m_id_to_value.size();
		m_value_to_id.insert(value, new_id);
		return new_id;
	}

private:
	JMutex m_mutex;
	// Values are stored here at id-1 position (id 1 = [0])
	core::array<T> m_id_to_value;
	core::map<T, u32> m_value_to_id;
};

/*
	FIFO queue (well, actually a FILO also)
*/
template<typename T>
class Queue
{
public:
	void push_back(T t)
	{
		m_list.push_back(t);
	}
	
	T pop_front()
	{
		if(m_list.size() == 0)
			throw ItemNotFoundException("Queue: queue is empty");

		typename core::list<T>::Iterator begin = m_list.begin();
		T t = *begin;
		m_list.erase(begin);
		return t;
	}
	T pop_back()
	{
		if(m_list.size() == 0)
			throw ItemNotFoundException("Queue: queue is empty");

		typename core::list<T>::Iterator last = m_list.getLast();
		T t = *last;
		m_list.erase(last);
		return t;
	}

	u32 size()
	{
		return m_list.size();
	}

protected:
	core::list<T> m_list;
};

/*
	Thread-safe FIFO queue (well, actually a FILO also)
*/

template<typename T>
class MutexedQueue
{
public:
	MutexedQueue()
	{
		m_mutex.Init();
	}
	u32 size()
	{
		JMutexAutoLock lock(m_mutex);
		return m_list.size();
	}
	void push_back(T t)
	{
		JMutexAutoLock lock(m_mutex);
		m_list.push_back(t);
	}
	T pop_front(u32 wait_time_max_ms=0)
	{
		u32 wait_time_ms = 0;

		for(;;)
		{
			{
				JMutexAutoLock lock(m_mutex);

				if(m_list.size() > 0)
				{
					typename core::list<T>::Iterator begin = m_list.begin();
					T t = *begin;
					m_list.erase(begin);
					return t;
				}

				if(wait_time_ms >= wait_time_max_ms)
					throw ItemNotFoundException("MutexedQueue: queue is empty");
			}

			// Wait a while before trying again
			sleep_ms(10);
			wait_time_ms += 10;
		}
	}
	T pop_back(u32 wait_time_max_ms=0)
	{
		u32 wait_time_ms = 0;

		for(;;)
		{
			{
				JMutexAutoLock lock(m_mutex);

				if(m_list.size() > 0)
				{
					typename core::list<T>::Iterator last = m_list.getLast();
					T t = *last;
					m_list.erase(last);
					return t;
				}

				if(wait_time_ms >= wait_time_max_ms)
					throw ItemNotFoundException("MutexedQueue: queue is empty");
			}

			// Wait a while before trying again
			sleep_ms(10);
			wait_time_ms += 10;
		}
	}

	JMutex & getMutex()
	{
		return m_mutex;
	}

	core::list<T> & getList()
	{
		return m_list;
	}

protected:
	JMutex m_mutex;
	core::list<T> m_list;
};

#endif

