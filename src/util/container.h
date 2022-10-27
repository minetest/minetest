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

#pragma once

#include "irrlichttypes.h"
#include "exceptions.h"
#include "threading/mutex_auto_lock.h"
#include "threading/semaphore.h"
#include <list>
#include <vector>
#include <map>
#include <set>
#include <queue>

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
	bool push_back(const Value& value)
	{
		if (m_set.insert(value).second)
		{
			m_queue.push(value);
			return true;
		}
		return false;
	}

	void pop_front()
	{
		m_set.erase(m_queue.front());
		m_queue.pop();
	}

	const Value& front() const
	{
		return m_queue.front();
	}

	u32 size() const
	{
		return m_queue.size();
	}

private:
	std::set<Value> m_set;
	std::queue<Value> m_queue;
};

template<typename Key, typename Value>
class MutexedMap
{
public:
	MutexedMap() = default;

	void set(const Key &name, const Value &value)
	{
		MutexAutoLock lock(m_mutex);
		m_values[name] = value;
	}

	bool get(const Key &name, Value *result) const
	{
		MutexAutoLock lock(m_mutex);
		auto n = m_values.find(name);
		if (n == m_values.end())
			return false;
		if (result)
			*result = n->second;
		return true;
	}

	std::vector<Value> getValues() const
	{
		MutexAutoLock lock(m_mutex);
		std::vector<Value> result;
		result.reserve(m_values.size());
		for (auto it = m_values.begin(); it != m_values.end(); ++it)
			result.push_back(it->second);
		return result;
	}

	void clear() { m_values.clear(); }

private:
	std::map<Key, Value> m_values;
	mutable std::mutex m_mutex;
};


// Thread-safe Double-ended queue

template<typename T>
class MutexedQueue
{
public:
	template<typename Key, typename U, typename Caller, typename CallerData>
	friend class RequestQueue;

	MutexedQueue() = default;

	bool empty() const
	{
		MutexAutoLock lock(m_mutex);
		return m_queue.empty();
	}

	void push_back(const T &t)
	{
		MutexAutoLock lock(m_mutex);
		m_queue.push_back(t);
		m_signal.post();
	}

	void push_back(T &&t)
	{
		MutexAutoLock lock(m_mutex);
		m_queue.push_back(std::move(t));
		m_signal.post();
	}

	/* this version of pop_front returns an empty element of T on timeout.
	* Make sure default constructor of T creates a recognizable "empty" element
	*/
	T pop_frontNoEx(u32 wait_time_max_ms)
	{
		if (m_signal.wait(wait_time_max_ms)) {
			MutexAutoLock lock(m_mutex);

			T t = std::move(m_queue.front());
			m_queue.pop_front();
			return t;
		}

		return T();
	}

	T pop_front(u32 wait_time_max_ms)
	{
		if (m_signal.wait(wait_time_max_ms)) {
			MutexAutoLock lock(m_mutex);

			T t = std::move(m_queue.front());
			m_queue.pop_front();
			return t;
		}

		throw ItemNotFoundException("MutexedQueue: queue is empty");
	}

	T pop_frontNoEx()
	{
		m_signal.wait();

		MutexAutoLock lock(m_mutex);

		T t = std::move(m_queue.front());
		m_queue.pop_front();
		return t;
	}

	T pop_back(u32 wait_time_max_ms=0)
	{
		if (m_signal.wait(wait_time_max_ms)) {
			MutexAutoLock lock(m_mutex);

			T t = std::move(m_queue.back());
			m_queue.pop_back();
			return t;
		}

		throw ItemNotFoundException("MutexedQueue: queue is empty");
	}

	/* this version of pop_back returns an empty element of T on timeout.
	* Make sure default constructor of T creates a recognizable "empty" element
	*/
	T pop_backNoEx(u32 wait_time_max_ms)
	{
		if (m_signal.wait(wait_time_max_ms)) {
			MutexAutoLock lock(m_mutex);

			T t = std::move(m_queue.back());
			m_queue.pop_back();
			return t;
		}

		return T();
	}

	T pop_backNoEx()
	{
		m_signal.wait();

		MutexAutoLock lock(m_mutex);

		T t = std::move(m_queue.back());
		m_queue.pop_back();
		return t;
	}

protected:
	std::mutex &getMutex() { return m_mutex; }

	std::deque<T> &getQueue() { return m_queue; }

	std::deque<T> m_queue;
	mutable std::mutex m_mutex;
	Semaphore m_signal;
};

template<typename K, typename V>
class LRUCache
{
public:
	LRUCache(size_t limit, void (*cache_miss)(void *data, const K &key, V *dest),
			void *data)
	{
		m_limit = limit;
		m_cache_miss = cache_miss;
		m_cache_miss_data = data;
	}

	void setLimit(size_t limit)
	{
		m_limit = limit;
		invalidate();
	}

	void invalidate()
	{
		m_map.clear();
		m_queue.clear();
	}

	const V *lookupCache(K key)
	{
		typename cache_type::iterator it = m_map.find(key);
		V *ret;
		if (it != m_map.end()) {
			// found!

			cache_entry_t &entry = it->second;

			ret = &entry.second;

			// update the usage information
			m_queue.erase(entry.first);
			m_queue.push_front(key);
			entry.first = m_queue.begin();
		} else {
			// cache miss -- enter into cache
			cache_entry_t &entry =
				m_map[key];
			ret = &entry.second;
			m_cache_miss(m_cache_miss_data, key, &entry.second);

			// delete old entries
			if (m_queue.size() == m_limit) {
				const K &id = m_queue.back();
				m_map.erase(id);
				m_queue.pop_back();
			}

			m_queue.push_front(key);
			entry.first = m_queue.begin();
		}
		return ret;
	}
private:
	void (*m_cache_miss)(void *data, const K &key, V *dest);
	void *m_cache_miss_data;
	size_t m_limit;
	typedef typename std::template pair<typename std::template list<K>::iterator, V> cache_entry_t;
	typedef std::template map<K, cache_entry_t> cache_type;
	cache_type m_map;
	// we can't use std::deque here, because its iterators get invalidated
	std::list<K> m_queue;
};
