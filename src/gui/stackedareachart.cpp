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

#include <set>
#include "porting.h"
#include "stackedareachart.h"
#include "util/string.h"

// Don't divide by a scale smaller than this
#define MIN_DIVISOR 0.0001

void StackedAreaChart::clear()
{
	m_smooth.clear();
	m_log.clear();
	m_log_dirty = true;
	m_colors.clear();
	m_used_colors.clear();
}

void StackedAreaChart::setTitle(const std::string &title)
{
	m_title = title;
}

void StackedAreaChart::updateSmoothData(const StackedAreaData &values)
{
	// Smooth the data. This is similiar to a sliding window,
	// but uses exponential decay.
	std::set<std::string> names;
	for (const auto &kv : values) {
		names.insert(kv.first);
	}
	for (const auto &kv : m_smooth) {
		names.insert(kv.first);
	}
	for (const auto &name : names) {
		auto &val = m_smooth[name];
		auto it = values.find(name);
		auto newval = (it != values.end()) ? it->second : (float)0;
		// Weighted average
		val = (0.99 * val + 1.0 * newval) / (0.99 + 1.0);
	}
}

void StackedAreaChart::put(const StackedAreaData &values)

{
	updateSmoothData(values);
	putLog(m_smooth);
}

void StackedAreaChart::putLog(const StackedAreaData &data)
{
	// Before inserting into the log, scale the time values
	// so that they sum to 1.0
	StackedAreaData final;
	float scale = 0;
	for (const auto &kv : data) {
		scale += kv.second;
	}
	if (scale > MIN_DIVISOR) {
		for (const auto &kv : data) {
			final[kv.first] = kv.second / scale;
		}
	}
	if (final.empty()) {
		final["No data"] = 1.0;
	}
	m_log.emplace_back(std::move(final));
	m_log_dirty = true;

	while (m_log.size() > m_log_max_size)
		m_log.pop_front();
}

// Visually distinct colors
// When these run-out, colors are chosen at random
static u32 colorMap[] = {
	0x800080, // purple
	0xff0000, // red
	0xffa500, // orange
	0xffff00, // yellow
	0x7cfc00, // lawngreen
	0x00ff7f, // springgreen
	0x00ffff, // aqua
	0x00bfff, // deepskyblue
	0xf4a460, // sandybrown
	0x0000ff, // blue
	0xff6347, // tomato
	0xb0c4de, // lightsteelblue
	0xff00ff, // fuchsia
	0x1e90ff, // dodgerblue
	0xf0e68c, // khaki
	0x90ee90, // lightgreen
	0xff1493, // deeppink
	0x7b68ee, // mediumslateblue
	0xee82ee, // violet
	0xffffe0, // lightyellow
	0xffc0cb, // pink
	0x696969, // dimgray
	0x800000, // maroon
	0x808000, // olive
	0x008000, // green
	0x000080, // navy
	0x9acd32, // yellowgreen
	0xb03060, // maroon3
};

video::SColor StackedAreaChart::findFreeColor()
{
	for (size_t i = 0; i < ARRLEN(colorMap); i++) {
		uint32_t val = colorMap[i];
		u32 r = (val >> 16) & 0xFF;
		u32 g = (val >> 8 ) & 0xFF;
		u32 b = (val >> 0 ) & 0xFF;
		auto color = video::SColor(255, r, g, b);
		if (!m_used_colors.count(color)) {
			return color;
		}
	}
	// Out of colors. Pick one at random.
	u32 val = myrand();
	u32 r = ((val >> 16) & 0xFF) | 0x80; // 128 to 255
	u32 g = ((val >> 8)  & 0xFF) | 0x80;
	u32 b = ((val >> 0)  & 0xFF) | 0x80;
	return video::SColor(255, r, g, b);
}

void StackedAreaChart::updateColorMap()
{
	std::set<std::string> alive_names;
	for (const auto & entry : m_log) {
		for (const auto &kv : entry) {
			alive_names.insert(kv.first);
		}
	}

	std::set<std::string> dead_names;
	for (const auto &kv : m_colors) {
		if (alive_names.count(kv.first) == 0) {
			dead_names.insert(kv.first);
		}
	}

	// Remove dead names / colors
	for (const auto &name : dead_names) {
		auto color = m_colors[name];
		m_colors.erase(name);
		m_used_colors.erase(color);
	}

	// Assign colors to new names
	for (const auto &name : alive_names) {
		if (m_colors.count(name) == 0) {
			auto color = findFreeColor();
			m_colors[name] = color;
			m_used_colors.insert(color);
		}
	}
}

void StackedAreaChart::updateTexture(video::IVideoDriver *driver)
{
	if (m_texture) {
		driver->removeTexture(m_texture);
		m_texture = nullptr;
	}

	m_pixels.clear();
	m_pixels_total = 0;

	u32 width = m_log_max_size;
	u32 height = m_log_max_size;
	core::dimension2d<u32> size(width, height);

	video::IImage *img = driver->createImage(video::ECF_A8R8G8B8, size);
	img->fill(video::SColor(0, 0, 0, 0));
	u32 x = width - m_log.size();
	for (const auto & entry : m_log) {
		float acc = 0;
		u32 y_start = 0;
		for (const auto & kv : entry) {
			auto color = m_colors[kv.first];
			acc += kv.second;
			u32 y_end = std::min((u32)(acc * height), height);
			for (u32 y = y_start; y < y_end; y++) {
				img->setPixel(x, y, color);
			}
			m_pixels[kv.first] += y_end - y_start;
			m_pixels_total += y_end - y_start;
			y_start = y_end;
		}
		++x;
	}
	m_texture = driver->addTexture("stackedarea", img);
	img->drop();
}


void StackedAreaChart::draw(
		const core::rect<s32> &dest,
		video::IVideoDriver *driver,
		gui::IGUIFont *font)
{
	if (!m_texture || m_log_dirty) {
		updateColorMap();
		updateTexture(driver);
		m_log_dirty = false;
	}

	s32 title_height = 25;

	s32 midX = (dest.UpperLeftCorner.X + dest.LowerRightCorner.X) / 2;

	core::rect<s32> title_rect = core::rect<s32>(
		dest.UpperLeftCorner.X,
		dest.UpperLeftCorner.Y,
		dest.LowerRightCorner.X,
		dest.UpperLeftCorner.Y + title_height);

	core::rect<s32> chart_rect = core::rect<s32>(
		dest.UpperLeftCorner.X,
		dest.UpperLeftCorner.Y + title_height,
		midX,
		dest.LowerRightCorner.Y);

	core::rect<s32> labels_rect = core::rect<s32>(
		midX + 2,
		dest.UpperLeftCorner.Y + title_height,
		dest.LowerRightCorner.X,
		dest.LowerRightCorner.Y);

	auto wh = m_texture->getOriginalSize();
	driver->draw2DImage(
		m_texture,
		chart_rect, // destRect
		core::rect<s32>(0, 0, wh.Width - 1, wh.Height - 1), // sourceRect
		nullptr, // clipRect
		nullptr, // colors
		true); // useAlphaChannelOfTexture

	auto white = video::SColor(255, 255, 255, 255);
	char buf[256];

	// Above the image print the title
	if (!m_title.empty()) {
		font->draw(utf8_to_wide(m_title).c_str(), title_rect, white,
			/* hcenter= */ true, /* vcenter= */ true);
	}

	// On the right half, print labels in alphabetic order
	s32 label_height = m_colors.size() ? labels_rect.getHeight() / m_colors.size() : 15;
	if (label_height < 15)
		label_height = 15;

	size_t label_index = 0;
	for (const auto &nc : m_colors) {
		s32 label_y_start = labels_rect.UpperLeftCorner.Y + label_index * label_height;
		s32 label_y_end = label_y_start + label_height;
		core::rect<s32> text_area(
			labels_rect.UpperLeftCorner.X,
			label_y_start,
			labels_rect.LowerRightCorner.X,
			label_y_end);

		// draw the label and percentage
		u32 pixcount = m_pixels[nc.first];
		porting::mt_snprintf(buf, sizeof(buf), "%s (%d %%)",
			nc.first.c_str(), (int)(100.0 * pixcount / m_pixels_total));
		font->draw(utf8_to_wide(buf).c_str(), text_area, nc.second);

		label_index++;
	}
}
