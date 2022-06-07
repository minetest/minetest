/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <cassert>
#include <string>
#include <map>
#include <ostream>
#include <set>

#include "threading/mutex_auto_lock.h"
#include "config.h"
#include "debug.h"
#include "porting.h"
#include "util/timetaker.h"
#include "util/numeric.h"      // paging()

//
// g_collector
//
//   Global stats collector. Use this for retrieving profiler data.
//   Must be declared before g_profiler so it is constructed first.
class StatsCollector;
extern StatsCollector g_collector;

//
// g_profiler
//
//   Thread-specific global profiler. Use this for recording data.
class Profiler;
extern thread_local Profiler g_profiler;

enum ProfileDataKind {
	PD_NONE,
	PD_STAT,
	PD_TIME,
};

struct ProfileData {
	ProfileDataKind kind;
	float total;
	float self;
	u64 count;

	ProfileData() :
		kind(PD_NONE),
		total(0),
		self(0),
		count(0)
	{ }

	void merge(const ProfileData &other)
	{
		if (kind == PD_NONE)
			kind = other.kind;
		sanity_check(kind == other.kind);
		total += other.total;
		self += other.self;
		count += other.count;
	}
};

class ScopeProfiler;

class StatsCollector {
public:
	// id -> {total time, self time, count}
	using ProfileDataMap = std::map<std::string, ProfileData>;
	// id -> total
	using GraphValuesMap = std::map<std::string, float>;
	// thread name -> data
	using ThreadDataMap = std::map<std::string, ProfileDataMap>;

	void clear();

	u64 getElapsedMs() const
	{
		return porting::getTimeMs() - m_start_time;
	}

	// Return and clear graph values.
	GraphValuesMap graphGet()
	{
		MutexAutoLock lock(m_mutex);
		aggregateThreads();
		GraphValuesMap result = std::move(m_combined_graphvalues);
		m_combined_graphvalues.clear();
		return result;
	}

	// Returns the line count
	int print(std::ostream &o, u32 page = 1, u32 pagecount = 1);
	int print(std::ostream &o, const ProfileDataMap &values);
	void getPage(ProfileDataMap &o, u32 page, u32 pagecount);

	// Return and clear per-thread data
	ThreadDataMap getThreadMap()
	{
		MutexAutoLock lock(m_mutex);
		aggregateThreads();
		ThreadDataMap result = std::move(m_thread_data);
		m_thread_data.clear();
		return result;
	}

	std::vector<std::string> listThreadNames();

#if BUILD_UNITTESTS
	// Used by unit tests to inspect the stats
	float getValue(const std::string &name)
	{
		MutexAutoLock lock(m_mutex);
		aggregateThreads();
		auto& entry = m_combined_data[name];
		return entry.total / entry.count;
	}
#endif
protected:
	friend class Profiler;

	void registerThread(Profiler *p)
	{
		MutexAutoLock lock(m_mutex);
		m_threads.insert(p);
	}

	void deregisterThread(Profiler *p)
	{
		MutexAutoLock lock(m_mutex);
		m_threads.erase(p);
	}
private:
	std::mutex m_mutex;
	std::set<Profiler*> m_threads;
	u64 m_start_time{0};
	ProfileDataMap m_combined_data;
	GraphValuesMap m_combined_graphvalues;
	// Thread Name -> aggregate data for that thread
	ThreadDataMap m_thread_data;
	// Must hold m_mutex when calling this.
	void aggregateThreads();
};

/*
	Time profiler
*/

class Profiler
{
public:

	Profiler()
	{
		g_collector.registerThread(this);
	}

	~Profiler()
	{
		g_collector.deregisterThread(this);
	}

	// Enable profiling
	static void enable()
	{
		m_profiling_enabled.store(true, std::memory_order_relaxed);
	}

	// Disable profiling
	static void disable()
	{
		m_profiling_enabled.store(false, std::memory_order_relaxed);
	}

	// Check if profiling is enabled
	static bool isEnabled()
	{
		return m_profiling_enabled.load(std::memory_order_relaxed);
	}

	void add(const std::string &name, float total_time, float self_time)
	{
		if (!isEnabled())
			return;
		MutexAutoLock lock(m_local_mutex);
		auto& entry = m_data[name];
		if (entry.kind == PD_NONE)
			entry.kind = PD_TIME;
		sanity_check(entry.kind == PD_TIME);

		entry.total += total_time;
		entry.self += self_time;
		entry.count += 1;
	}

	// 'avg' is used as a general stats collection endpoint (not profiling data)
	// This complicates the internal representation a little bit.
	// Normally a profiler feeds into a stats collector, not vice versa.
	void avg(const std::string &name, float value)
	{
		if (!isEnabled())
			return;
		MutexAutoLock lock(m_local_mutex);
		auto& entry = m_data[name];

		if (entry.kind == PD_NONE)
			entry.kind = PD_STAT;
		sanity_check(entry.kind == PD_STAT);

		entry.total += value;
		entry.count += 1;
	}

	// For generating real time event graphs.
	void graphAdd(const std::string &id, float value)
	{
		if (!isEnabled())
			return;
		MutexAutoLock lock(m_local_mutex);
		m_graphvalues[id] += value;
	}

	void remove(const std::string& name)
	{
		if (!isEnabled())
			return;
		MutexAutoLock lock(m_local_mutex);
		m_data.erase(name);
	}

	void setThreadName(const std::string &name)
	{
		MutexAutoLock lock(m_local_mutex);
		m_thread_name = name;
	}

protected:
	friend class StatsCollector;

	void clear()
	{
		MutexAutoLock lock(m_local_mutex);
		m_data.clear();
		m_graphvalues.clear();
	}

	std::string getThreadName() const
	{
		MutexAutoLock lock(m_local_mutex);
		return m_thread_name;
	}

	StatsCollector::ProfileDataMap takeData()
	{
		MutexAutoLock lock(m_local_mutex);
		StatsCollector::ProfileDataMap tmp = std::move(m_data);
		m_data.clear();
		return tmp;
	}

	StatsCollector::GraphValuesMap takeGraphValues()
	{
		MutexAutoLock lock(m_local_mutex);
		StatsCollector::GraphValuesMap tmp = std::move(m_graphvalues);
		m_graphvalues.clear();
		return tmp;
	}

private:
	static std::atomic<bool> m_profiling_enabled;

	std::string m_thread_name;

	mutable std::mutex m_local_mutex;

	// Local mutex protects these
	StatsCollector::ProfileDataMap m_data;
	StatsCollector::GraphValuesMap m_graphvalues;
};

class ScopeProfiler
{
public:
	ScopeProfiler(const std::string &name) :
		ScopeProfiler(name.c_str())
	{ }

	ScopeProfiler(const char *name) :
		m_enabled(Profiler::isEnabled())
	{
		if (!m_enabled)
			return;
		m_name = std::string(name) + " [ms]";
		m_start_time = porting::getTimeMs();
		m_exclude_time = 0;
		m_stack.push_back(this);
	}

	~ScopeProfiler()
	{
		if (!m_enabled)
			return;
		sanity_check(!m_stack.empty() && m_stack.back() == this);
		m_stack.pop_back();

		u64 duration = porting::getTimeMs() - m_start_time;
		if (!m_stack.empty()) {
			// Exclude from the 'self' time of the scope above it
			m_stack.back()->m_exclude_time += duration;
		}
		g_profiler.add(m_name, duration, duration - m_exclude_time);
	}
private:
	bool m_enabled;
	static thread_local std::vector<ScopeProfiler*> m_stack;
protected:
	friend class Profiler;
	std::string m_name;
	u64 m_start_time;
	u64 m_exclude_time;
};
