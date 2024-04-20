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

static Profiler main_profiler;
Profiler *g_profiler = &main_profiler;

ScopeProfiler::ScopeProfiler(Profiler *profiler, const std::string &name,
		ScopeProfilerType type, TimePrecision prec) :
	m_profiler(profiler),
	m_name(name), m_type(type), m_precision(prec)
{
	m_name.append(" [").append(TimePrecision_units[prec]).append("]");
	m_time1 = porting::getTime(prec);
}

ScopeProfiler::~ScopeProfiler()
{
	if (!m_profiler)
		return;

	float duration = porting::getTime(m_precision) - m_time1;

	switch (m_type) {
	case SPT_ADD:
		m_profiler->add(m_name, duration);
		break;
	case SPT_AVG:
		m_profiler->avg(m_name, duration);
		break;
	case SPT_GRAPH_ADD:
		m_profiler->graphAdd(m_name, duration);
		break;
	case SPT_MAX:
		m_profiler->max(m_name, duration);
		break;
	}
}

Profiler::Profiler()
{
	m_start_time = porting::getTimeMs();
}

void Profiler::add(const std::string &name, float value)
{
	MutexAutoLock lock(m_mutex);

	auto it = m_data.find(name);
	if (it == m_data.end()) {
		// mark with special value for checking
		m_data.emplace(name, DataPair{value, -SPT_ADD});
	} else {
		assert(it->second.avgcount == -SPT_ADD);
		it->second.value += value;
	}
}

void Profiler::max(const std::string &name, float value)
{
	MutexAutoLock lock(m_mutex);

	auto it = m_data.find(name);
	if (it == m_data.end()) {
		// mark with special value for checking
		m_data.emplace(name, DataPair{value, -SPT_MAX});
	} else {
		assert(it->second.avgcount == -SPT_MAX);
		it->second.value = std::max(value, it->second.value);
	}
}

void Profiler::avg(const std::string &name, float value)
{
	MutexAutoLock lock(m_mutex);

	auto it = m_data.find(name);
	if (it == m_data.end()) {
		m_data.emplace(name, DataPair{value, 1});
	} else {
		assert(it->second.avgcount >= 0);
		it->second.value += value;
		it->second.avgcount++;
	}
}

void Profiler::clear()
{
	MutexAutoLock lock(m_mutex);
	for (auto &it : m_data)
		it.second.reset();
	m_start_time = porting::getTimeMs();
}

float Profiler::getValue(const std::string &name) const
{
	auto it = m_data.find(name);
	if (it == m_data.end())
		return 0;
	return it->second.getValue();
}

int Profiler::getAvgCount(const std::string &name) const
{
	auto it = m_data.find(name);
	if (it == m_data.end())
		return 1;
	int denominator = it->second.avgcount;
	return denominator >= 1 ? denominator : 1;
}

u64 Profiler::getElapsedMs() const
{
	return porting::getTimeMs() - m_start_time;
}

int Profiler::print(std::ostream &o, u32 page, u32 pagecount)
{
	GraphValues values;
	getPage(values, page, pagecount);
	char buffer[50];

	for (const auto &i : values) {
		o << "  " << i.first << " ";
		if (i.second == 0) {
			o << std::endl;
			continue;
		}

		{
			// Padding
			s32 space = std::max(0, 44 - (s32)i.first.size());
			memset(buffer, '_', space);
			buffer[space] = '\0';
			o << buffer;
		}

		porting::mt_snprintf(buffer, sizeof(buffer), "% 5ix % 7g",
				getAvgCount(i.first), floor(i.second * 1000.0) / 1000.0);
		o << buffer << std::endl;
	}
	return values.size();
}

void Profiler::getPage(GraphValues &o, u32 page, u32 pagecount)
{
	MutexAutoLock lock(m_mutex);

	u32 minindex, maxindex;
	paging(m_data.size(), page, pagecount, minindex, maxindex);

	for (const auto &i : m_data) {
		if (maxindex == 0)
			break;
		maxindex--;

		if (minindex != 0) {
			minindex--;
			continue;
		}

		o[i.first] = i.second.getValue();
	}
}
