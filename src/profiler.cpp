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

static Profiler main_profiler;
Profiler *g_profiler = &main_profiler;
ScopeProfiler::ScopeProfiler(
		Profiler *profiler, const std::string &name, ScopeProfilerType type) :
		m_profiler(profiler),
		m_name(name), m_type(type)
{
	m_name.append(" [ms]");
	if (m_profiler)
		m_timer = new TimeTaker(m_name, nullptr, PRECISION_MILLI);
}

ScopeProfiler::~ScopeProfiler()
{
	if (!m_timer)
		return;

	float duration_ms = m_timer->stop(true);
	float duration = duration_ms / 1000.0;
	if (m_profiler) {
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
		}
	}
	delete m_timer;
}

float Profiler::getValue(const std::string &name) const
{
	auto numerator = m_data.find(name);
	if (numerator == m_data.end())
		return 0.f;

	auto denominator = m_avgcounts.find(name);
	if (denominator != m_avgcounts.end()){
		if (denominator->second >= 1)
			return numerator->second / denominator->second;
	}

	return numerator->second;
}

int Profiler::getAvgCount(const std::string &name) const
{
	auto n = m_avgcounts.find(name);

	if (n != m_avgcounts.end() && n->second >= 1)
		return n->second;

	return 1;
}

int Profiler::print(std::ostream &o, u32 page, u32 pagecount)
{
	GraphValues values;
	getPage(values, page, pagecount);

	for (const auto &i : values) {
		o << "  " << i.first << " ";
		s32 space = 44 - i.first.size();
		for (s32 j = 0; j < space; j++) {
			if ((j & 1) && j < space - 1)
				o << ".";
			else
				o << " ";
		}
		o << getAvgCount(i.first) << "x ";
		o << i.second << std::endl;
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

		o[i.first] = i.second / getAvgCount(i.first);
	}
}
