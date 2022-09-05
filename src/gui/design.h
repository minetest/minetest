/*
Minetest
Copyright (C) 2022 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include "drawable.h"
#include "json_reader.h"
#include "ui_types.h"
#include "util/string.h"

#include <array>
#include <string>
#include <unordered_map>

namespace ui
{
	enum class TileMode
	{
		NONE,
		REPEAT,
		ROUND
	};

	constexpr EnumNameMap<TileMode> TILE_MODE_NAME_MAP = {
		{"none", TileMode::NONE},
		{"repeat", TileMode::REPEAT},
		{"round", TileMode::ROUND}
	};

	class Design
	{
	private:
		Color m_color;

		Texture m_texture;
		Rect<float> m_src_rect;
		Color m_tint;

		s32 m_num_frames;
		u32 m_frame_time;
		Vec<float> m_frame_offset;

		Vec<float> m_scale;
		Vec<float> m_align;

		std::array<TileMode, 2> m_tile_mode;

	public:
		Design()
		{
			resetView();
		}

		void drawTo(Canvas &canvas, const Rect<float> &rect);

		void resetView();
		void applyView(const JsonReader &json);
	};
}
