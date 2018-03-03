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

/* Profiler display */
class ProfilerGraph
{
private:
	struct Piece
	{
		Piece(Profiler::GraphValues v) : values(std::move(v)) {}
		Profiler::GraphValues values;
	};
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

	void put(const Profiler::GraphValues &values);

	void draw(s32 x_left, s32 y_bottom, video::IVideoDriver *driver,
			gui::IGUIFont *font) const;
};
