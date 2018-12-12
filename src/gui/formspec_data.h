/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <stack>

#include "irr_v2d.h"
#include "rect.h"

struct FormSpecData {
	u32 formspec_version = 0;

	v2s32 padding;
	v2f32 spacing;
	v2s32 imgsize;
	v2s32 offset;
	v2f32 pos_offset;

	core::rect<s32> absolute_rect;

	std::stack<v2f32> container_stack;
};

