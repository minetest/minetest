/*
Minetest
Copyright (C) 2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "profiler.h"
#include "porting.h"

StatsCollector g_collector;
std::atomic<bool> Profiler::m_profiling_enabled{false};
thread_local Profiler g_profiler;
thread_local std::vector<ScopeProfiler*> ScopeProfiler::m_stack;

void StatsCollector::clear()
{
	m_start_time = porting::getTimeMs();
	MutexAutoLock lock(m_mutex);
	for (auto profiler : m_threads) {
		profiler->clear();
	}
}

std::vector<std::string> StatsCollector::listThreadNames()
{
	MutexAutoLock lock(m_mutex);
	std::vector<std::string> threads;
	for (const auto & profiler : m_threads) {
		threads.push_back(profiler->getThreadName());
	}
	return threads;
}

int StatsCollector::print(std::ostream &o, u32 page, u32 pagecount)
{
	ProfileDataMap values;
	getPage(values, page, pagecount);
	return print(o, values);
}

int StatsCollector::print(std::ostream &o, const ProfileDataMap &values)
{
	for (const auto &i : values) {
		o << "  " << i.first << " ";
		if (i.second.total == 0) {
			o << std::endl;
			continue;
		}

		s32 space = 44 - i.first.size();
		for (s32 j = 0; j < space; j++) {
			if ((j & 1) && j < space - 1)
				o << ".";
			else
				o << " ";
		}
		float total = i.second.total;
		u64 count = i.second.count;
		if (i.second.kind == PD_STAT && count > 0) {
			total /= count;
			count = 1;
		}
		char num_buf[50];
		porting::mt_snprintf(num_buf, sizeof(num_buf), "% 4ix % 3g",
			(unsigned int)count, (double)total);
		o << num_buf << std::endl;
	}
	return values.size();
}

void StatsCollector::getPage(ProfileDataMap &o, u32 page, u32 pagecount)
{
	MutexAutoLock lock(m_mutex);
	aggregateThreads();

	u32 minindex, maxindex;
	paging(m_combined_data.size(), page, pagecount, minindex, maxindex);
	sanity_check(minindex <= maxindex);
	sanity_check(maxindex <= m_combined_data.size());

	// Copy slice of the map corresponding to the page
	auto begin = std::next(m_combined_data.begin(), minindex);
	auto end = std::next(m_combined_data.begin(), maxindex);
	o.insert(begin, end);
}

// Must hold m_mutex when calling this
void StatsCollector::aggregateThreads()
{
	for (auto profiler : m_threads) {
		auto data = profiler->takeData();
		auto& thread_specific_data = m_thread_data[profiler->getThreadName()];
		for (const auto &kv : data) {
			m_combined_data[kv.first].merge(kv.second);
			// Thread-specific data is just scope timers
			if (kv.second.kind == PD_TIME) {
				thread_specific_data[kv.first].merge(kv.second);
			}
		}
		auto graphvalues = profiler->takeGraphValues();
		for (const auto &kv : graphvalues) {
			m_combined_graphvalues[kv.first] += kv.second;
		}
	}
}
