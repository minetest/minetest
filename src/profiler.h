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


/**	\defgroup Profiling Profiling tools
	\brief Embedded tools to profile CPU time or anything else that is recorded
	by certain functions.

	An example with global profiler (#g_profiler), that is available in any file
	with profiler.h included:
	\code{.c++}
	void functionToProfie(int count) {
		// Record (add) time till function (scope) end
		ScopeProfiler scope_profiler(g_profiler, "functionToProfie()"); // SPT_ADD by default
		// Record (add) `count`
		g_profiler->add("functionToProfie() count", count)
	}
	\endcode
	In this example "add" method is used, so if function is called several times during
	g_profiler cycle, sum of all recorded values will be shown at cycle end. See
	#g_profiler for details about cycle and info showing.

	TimeTaker can be used instead of ScopeProfiler.
	\{
*/


/**	\brief Collects values (time intervals, counts and other) in named entries.

	Can be used to get sum, average or maximum of recorded values for each entry. Gives
	result in text form via print().

	Can collect values for #ProfilerGraph and give them via graphPop().

	For usage, see \ref Profiling.
*/
class Profiler
{
public:
	Profiler();

	/**	\brief Records the \p value in entry with given \p name to get sum of recorded values.
		\param[in] name: entry name; can be same for several function calls
		\param[in] value: value to record
	*/
	void add(const std::string &name, float value);

	/**	\brief Records the \p value in entry with given \p name to get average of recorded values.
		\param[in] name: entry name; can be same for several function calls
		\param[in] value: value to record
	*/
	void avg(const std::string &name, float value);

	/**	\brief Records the \p value in entry with given \p name to get maximum of recorded values.
		\param[in] name: entry name; can be same for several function calls
		\param[in] value: value to record
	*/
	void max(const std::string &name, float value);

	/// Clears all profiling entries (#m_data). Does not affect #m_graphvalues.
	void clear();

	float getValue(const std::string &name) const;
	int getAvgCount(const std::string &name) const;
	u64 getElapsedMs() const;

	typedef std::map<std::string, float> GraphValues;

	/**	\brief Prints collected data formatted as table.

		Prints each entry as one line which contains name, avgCount and value.
		Meaning of printed numbers depends on used recording method (add/avg/max).
		avgCount makes sense only for "avg" method and shows number of records;
		always shows 1 for other methods.
		
		Breaks all entries into \p pagecount pages and print only given \p page.

		\param[out] o: stream to print into
		\param[in] page: number of page to print
		\param[in] pagecount: count of pages to break data into
		\returns printed line count
	*/
	int print(std::ostream &o, u32 page = 1, u32 pagecount = 1);

	/**	\brief Breaks #m_data (entries) into \p pagecount pages (GraphValues) and
		writes values on given \p page into \p o.
		\param[out] o: variable to write page into
		\param[in] page: page to get
		\param[in] pagecount: number of page to break data into
	*/
	void getPage(GraphValues &o, u32 page, u32 pagecount);

	/**	\brief Records the \p value in graph entry with given \p id (name) to get
		exactly this value.
		\param[in] id: entry name
		\param[in] value: value to record
	*/
	void graphSet(const std::string &id, float value)
	{
		MutexAutoLock lock(m_mutex);
		m_graphvalues[id] = value;
	}

	/**	\brief Records the \p value in graph entry with given \p id (name) to get
		sum of recorded values.
		\param[in] id: entry name
		\param[in] value: value to record
	*/
	void graphAdd(const std::string &id, float value)
	{
		MutexAutoLock lock(m_mutex);
		auto it = m_graphvalues.find(id);
		if (it == m_graphvalues.end())
			m_graphvalues.emplace(id, value);
		else
			it->second += value;
	}

	///	\brief Gives m_graphvalues into \p result.
	///	\param[out] result: Variable to record values into
	void graphPop(GraphValues &result)
	{
		MutexAutoLock lock(m_mutex);
		assert(result.empty());
		std::swap(result, m_graphvalues);
	}

	///	\brief Removes entry with given \p name.
	///	\param[in] name: Name of entry to remove
	void remove(const std::string& name)
	{
		MutexAutoLock lock(m_mutex);
		m_data.erase(name);
	}

private:
	struct DataPair {
		/// Recorded value. Recording method depends on used functions (add/avg/max)
		/// of profiler.
		float value = 0;
		/// Count of records if "avg" method is used.
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
	/// All the profiler entries, stored until `clear()` call.
	std::map<std::string, DataPair> m_data;
	/// Values for profiler graph collected until next frame draw. Value history
	/// is stored in `ProfilerGraph`.
	std::map<std::string, float> m_graphvalues;
	u64 m_start_time;
};


/**	\brief Global profiler. User can view its data via in-game profiler menu,
	graph or "/profiler" command.

	Profiling cycles are managed in Game class:

	- cycle for profiler menu (GameUI::updateProfiler()) is managed in
	Game::updateProfilers() and lasts for \c profiler_print_interval seconds (user
	setting) or for 3 seconds if setting is set to 0;

	- cycle for ProfilerGraph is managed in Game::updateProfilerGraphs() and lasts
	for one frame;
*/
extern Profiler* g_profiler;


enum ScopeProfilerType : u8
{
	/// Record time with Profiler::add() (to get sum of recorded values) at scope end.
	SPT_ADD = 1,
	/// Record time with Profiler::avg() (to get average of recorded values) at scope end.
	SPT_AVG,
	/// Record time with Profiler::graphAdd() (to get sum of recorded values on
	/// ProfilerGraph) at scope end.
	SPT_GRAPH_ADD,
	/// Record time with Profiler::max() (to get maximum of recorded values) at scope end.
	SPT_MAX
};


/**	\brief A class for time measurements. Each created object records time from its
	construction till scope end.

	Note: this class should be kept lightweight.
*/
class ScopeProfiler
{
public:
	/**	\brief Begins measurement until scope end.Result will be recorded to
		\p name
		profiling entry in \p profiler (profiling entries are created/deleted automatically).
	*/
	ScopeProfiler(Profiler *profiler, const std::string &name,
			ScopeProfilerType type = SPT_ADD,
			TimePrecision precision = PRECISION_MILLI);

	/// Ends measurement and record result in profiler.
	~ScopeProfiler();

private:
	Profiler *m_profiler = nullptr;	/// Profiler to record in
	std::string m_name;				/// Name of profiler entry to record in
	u64 m_time1;					/// Record start time
	ScopeProfilerType m_type;
	TimePrecision m_precision;
};

/** \} */
