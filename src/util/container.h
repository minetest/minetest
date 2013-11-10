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

#ifndef UTIL_CONTAINER_HEADER
#define UTIL_CONTAINER_HEADER

#include "../irrlichttypes.h"
#include "../exceptions.h"
#include "../jthread/jmutex.h"
#include "../jthread/jmutexautolock.h"
#include "../porting.h" // For sleep_ms
#include <list>
#include <vector>
#include <map>

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
		if(m_map.find(value) != m_map.end())
			return false;

		// Add
		m_map[value] = 0;
		m_list.push_back(value);
		
		return true;
	}

	Value pop_front()
	{
		typename std::list<Value>::iterator i = m_list.begin();
		Value value = *i;
		m_map.erase(value);
		m_list.erase(i);
		return value;
	}

	u32 size()
	{
		return m_map.size();
	}

private:
	std::map<Value, u8> m_map;
	std::list<Value> m_list;
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

		typename std::map<Key, Value>::iterator n;
		n = m_values.find(name);

		if(n == m_values.end())
			return false;
		
		if(result != NULL)
			*result = n->second;
			
		return true;
	}

	std::list<Value> getValues()
	{
		std::list<Value> result;
		for(typename std::map<Key, Value>::iterator
				i = m_values.begin();
				i != m_values.end(); ++i){
			result.push_back(i->second);
		}
		return result;
	}
	
	void clear ()
	{
		m_values.clear();
	}

private:
	std::map<Key, Value> m_values;
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
		typename std::map<T, u32>::iterator n;
		n = m_value_to_id.find(value);
		if(n != m_value_to_id.end())
			return n->second;
		m_id_to_value.push_back(value);
		u32 new_id = m_id_to_value.size();
		m_value_to_id.insert(value, new_id);
		return new_id;
	}

private:
	JMutex m_mutex;
	// Values are stored here at id-1 position (id 1 = [0])
	std::vector<T> m_id_to_value;
	std::map<T, u32> m_value_to_id;
};

/*
	FIFO queue (well, actually a FILO also)
*/
template<typename T>
class Queue
{
public:
	Queue():
		m_list_size(0)
	{}

	void push_back(T t)
	{
		m_list.push_back(t);
		++m_list_size;
	}
	
	T pop_front()
	{
		if(m_list.empty())
			throw ItemNotFoundException("Queue: queue is empty");

		typename std::list<T>::iterator begin = m_list.begin();
		T t = *begin;
		m_list.erase(begin);
		--m_list_size;
		return t;
	}
	T pop_back()
	{
		if(m_list.empty())
			throw ItemNotFoundException("Queue: queue is empty");

		typename std::list<T>::iterator last = m_list.back();
		T t = *last;
		m_list.erase(last);
		--m_list_size;
		return t;
	}

	u32 size()
	{
		return m_list_size;
	}

	bool empty()
	{
		return m_list.empty();
	}

protected:
	std::list<T> m_list;
	u32 m_list_size;
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
	bool empty()
	{
		JMutexAutoLock lock(m_mutex);
		return m_list.empty();
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

				if(!m_list.empty())
				{
					typename std::list<T>::iterator begin = m_list.begin();
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

				if(!m_list.empty())
				{
					typename std::list<T>::iterator last = m_list.back();
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

	std::list<T> & getList()
	{
		return m_list;
	}

protected:
	JMutex m_mutex;
	std::list<T> m_list;
};

#endif

