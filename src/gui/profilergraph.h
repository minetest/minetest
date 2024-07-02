/*
Minetest
Copyright (C) 2010-2018 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <SColor.h>
#include <deque>
#include <utility>
#include <IGUIFont.h>
#include <IVideoDriver.h>
#include "profiler.h"


/** \brief Draws graph every frame using data given by Profiler.

	It displays only entries that are recorded for profiler graph (via Profiler::graphAdd,
	ScopeProfilerType::SPT_GRAPH_ADD and other) and Profiler itself doesn't show them.

	Each profiling entry described with upper bound value, entry name and lower bound
	value on its right. Graph is displayed as \e line if it is relative (lower value
	is not 0) and \e filled if it is absolute.

	\ingroup Profiling
*/
class ProfilerGraph
{
private:
	/// One-frame slice of graph values.
	struct Piece
	{
		Piece(Profiler::GraphValues v) : values(std::move(v)) {}
		Profiler::GraphValues values;
	};
	/// Data for drawing one graph. Updates every frame.
	struct Meta
	{
		float min;
		float max;
		video::SColor color;
		Meta(float initial = 0,
				video::SColor color = video::SColor(255, 255, 255, 255)) :
				min(initial),
				max(initial), color(color)
		{
		}
	};
	std::deque<Piece> m_log;

public:
	u32 m_log_max_size = 200;

	ProfilerGraph() = default;

	/// Adds graph values to the end of graph (rendered at right side) and
	/// removes the oldest ones (beyond the `m_log_max_size`).
	void put(const Profiler::GraphValues &values);

	void draw(s32 x_left, s32 y_bottom, video::IVideoDriver *driver,
			gui::IGUIFont *font) const;
};
