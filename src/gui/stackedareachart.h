/*
Minetest
Copyright (C) 2022 The Minetest Authors

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

#include <deque>
#include <utility>
#include <SColor.h>
#include <IVideoDriver.h>
#include <IGUIFont.h>
#include "profiler.h"

class StackedAreaChart
{
public:
	using StackedAreaData = std::map<std::string, float>;

	u32 m_log_max_size = 256;

	StackedAreaChart() = default;

	void clear();

	void setTitle(const std::string &title);

	void put(const StackedAreaData &values);

	void draw(
		const core::rect<s32> &dest,
		video::IVideoDriver *driver,
		gui::IGUIFont *font);
private:
	std::deque<StackedAreaData> m_log;
	bool m_log_dirty = false;

	// Used to smooth the data
	StackedAreaData m_smooth;

	// Color assignments
	std::map<std::string, video::SColor> m_colors;
	std::set<video::SColor> m_used_colors;

	// Pixels per label
	std::map<std::string, u32> m_pixels;
	size_t m_pixels_total;

	std::string m_title;

	video::ITexture *m_texture = nullptr;

	video::SColor findFreeColor();
	void updateColorMap();
	void updateTexture(video::IVideoDriver *driver);
	void updateSmoothData(const StackedAreaData &values);
	void putLog(const StackedAreaData &data);
};
