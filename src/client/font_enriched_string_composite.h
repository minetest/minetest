/*
Minetest
Copyright (C) 2024 cx384

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

#include "util/enriched_string.h"
#include "fontengine.h"
#include "rect.h"

// Note this is client code, because of the draw function.

// A text consisting of multiple enriched strings which are drawn with different fonts
struct FontEnrichedStringComposite {
	FontEnrichedStringComposite(const std::wstring &s,
		const video::SColor &initial_color = video::SColor(255, 255, 255, 255),
		const FontSpec &initial_font = FontSpec(FONT_SIZE_UNSPECIFIED, FM_Standard, false, false));

	void draw(core::rect<s32> position) const;

	core::dimension2d<u32> getDimension() const;

private:
	using Line = std::vector<std::pair<EnrichedString, FontSpec>>;
	std::vector<Line> m_lines;
};
