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

#include "threading/mutex_auto_lock.h"
#include "util/timetaker.h"
#include "util/numeric.h"      // paging()

class Profiler;
// Global profiler. Its data is available for user.
extern Profiler *g_profiler;

/*
	Time profiler

	Collects data (time intervals and other) in two independent cycles.

	- Cycle for profiler graph lasts for one frame and managed in 
	`Game::updateProfilerGraphs()`, result is displayed by `ProfilerGraph`.

	- Cycle for all other data lasts for `profiler_print_interval` seconds (user
	setting) and managed in `Game::updateProfilers()`, result is given in text
	form by `print` function. User can view it via in-game profiler menu or
	"/profiler" command.

	All data is cleared at cycle end.
*/
class Profiler
{
public:
	Profiler();

	/*
		Adds value to given profiler entry (`name`). Does not change count
		in the entry.

		As a resut, profiler will give *sum* of record values as value and "1" as avgCount.
		(If only this method is used for given entry between profiler updates)
	*/
	void add(const std::string &name, float value);

	/*
		Adds value to given profiler entry (`name`) and increases its record count
		by 1.
	
		As a resut, profiler will give *average* of record vaues as value and count of
		records as avgCount.
		(If only this method is used for given entry between profiler updates)
	*/
	void avg(const std::string &name, float value);
	/*
		Sets value to given profiler entry (`name`) if it is larger then existing one and
		increases record coutn by 1.

		As a resut, profiler will give *max* record value as value and count of records as avgCount.
		(If only this method is used for given entry between profiler updates)
	*/
	void max(const std::string &name, float value);
	void clear();

	float getValue(const std::string &name) const;
	int getAvgCount(const std::string &name) const;
	u64 getElapsedMs() const;

	typedef std::map<std::string, float> GraphValues;

	// Returns the line count
	int print(std::ostream &o, u32 page = 1, u32 pagecount = 1);
	// Writes values on page into `o`.
	void getPage(GraphValues &o, u32 page, u32 pagecount);


	void graphSet(const std::string &id, float value)
	{
		MutexAutoLock lock(m_mutex);
		m_graphvalues[id] = value;
	}
	void graphAdd(const std::string &id, float value)
	{
		MutexAutoLock lock(m_mutex);
		auto it = m_graphvalues.find(id);
		if (it == m_graphvalues.end())
			m_graphvalues.emplace(id, value);
		else
			it->second += value;
	}
	void graphPop(GraphValues &result)
	{
		MutexAutoLock lock(m_mutex);
		assert(result.empty());
		std::swap(result, m_graphvalues);
	}

	void remove(const std::string& name)
	{
		MutexAutoLock lock(m_mutex);
		m_data.erase(name);
	}

private:
	struct DataPair {
		// Recorded value. Recording method depends on used functions (add/avg/max)
		// of profiler
		float value = 0;
		// Count of records.
		int avgcount = 0;

		inline void reset() {
			value = 0;
			// negative values are used for type checking, so leave them alone
			if (avgcount >= 1)
				avgcount = 0;
		}
		inline float getValue() const {
			return avgcount >= 1 ? (value / avgcount) : value;
		}
	};

	std::mutex m_mutex;
	// All the profiler entries, stored untill `clear()` call.
	std::map<std::string, DataPair> m_data;
	// Values for profiler graph collected untill next frame draw. Value history
	// are stored in `ProfilerGraph`.
	std::map<std::string, float> m_graphvalues;
	u64 m_start_time;
};


enum ScopeProfilerType : u8
{
	// Accumulate measurements untill profiler update.
	// Profiler will print sum as value and "1" as count.
	SPT_ADD = 1,
	// Accumulate measurements and record count untill profiler update.
	// Profiler will print average value as value and record count as count.
	SPT_AVG,
	// Accumulate measurements until frame update and then add sum to profiler graph.
	SPT_GRAPH_ADD,
	// Save only the largest recorded value. Profiler will print them as value and "1" as count.
	SPT_MAX
};

/*
	A class for time measurements. Each created object records time from its
	construction till scope end.

	Note: this class should be kept lightweight.
*/
class ScopeProfiler
{
public:
	/* 
		Begins record untill scope end. Result will be added to `name` profiling
		entry in `profiler` (profiling entries are created/deleted automatically).
	*/
	ScopeProfiler(Profiler *profiler, const std::string &name,
			ScopeProfilerType type = SPT_ADD,
			TimePrecision precision = PRECISION_MILLI);

	// Ends record and logs result in profiler.
	~ScopeProfiler();

private:
	Profiler *m_profiler = nullptr;
	std::string m_name;
	u64 m_time1;  // Record start time
	ScopeProfilerType m_type;
	TimePrecision m_precision;
};
