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


int Profiler::print(std::ostream &o, u32 page, u32 pagecount)
{
	GraphValues values;
	getPage(values, page, pagecount);

	for (const auto &i : values) {
		o << "  " << i.first << ": ";
		s32 clampsize = 50;
		s32 space = clampsize - i.first.size();
		for(s32 j = 0; j < space; j++) {
			if ((j & 1) && j < space - 1)
				o << "-";
			else
				o << " ";
		}
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

		int avgcount = 1;
		auto n = m_avgcounts.find(i.first);
		if (n != m_avgcounts.end()) {
			if(n->second >= 1)
				avgcount = n->second;
		}
		o[i.first] = i.second / avgcount;
	}
}
