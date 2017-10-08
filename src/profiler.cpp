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
	if (m_profiler)
		m_timer = new TimeTaker(m_name);
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
