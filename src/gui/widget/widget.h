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

#include <string>
#include <vector>

#include "irrlichttypes_extrabloated.h"

#include "gui/formspec_data.h"

class Widget
{
public:
	virtual ~Widget() = default;

	virtual void draw(video::IVideoDriver *driver) const = 0;

	static v2s32 getElementBasePos(bool absolute, const FormSpecData &formdata, const std::vector<std::string> *v_pos)
	{
		v2s32 pos = formdata.padding;
		if (absolute)
			pos += formdata.absolute_rect.UpperLeftCorner;

		v2f32 pos_f = v2f32(pos.X, pos.Y) + formdata.pos_offset * formdata.spacing;
		if (v_pos) {
			pos_f.X += stof((*v_pos)[0]) * formdata.spacing.X;
			pos_f.Y += stof((*v_pos)[1]) * formdata.spacing.Y;
		}
		return v2s32(pos_f.X, pos_f.Y);
	}
};

