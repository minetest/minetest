/*
Minetest
Copyright (C) 2020 Hugues Ross <hugues.ross@gmail.com>

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

#include <string>

#include "irrlichttypes_extrabloated.h"

struct TooltipSpec
{
	TooltipSpec() = default;
	TooltipSpec(const std::wstring &a_tooltip, irr::video::SColor a_bgcolor,
			irr::video::SColor a_color) :
			tooltip(translate_string(a_tooltip)),
			bgcolor(a_bgcolor), color(a_color)
	{
	}

	std::wstring tooltip;
	irr::video::SColor bgcolor;
	irr::video::SColor color;
};
