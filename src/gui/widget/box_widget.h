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

#include "gui/widget/widget.h"

struct parserData;

class BoxWidget : public Widget
{
public:
	BoxWidget(v2s32 pos, v2s32 geom, irr::video::SColor color):
		m_pos(pos), m_geom(geom), m_color(color)
	{}

	void draw(video::IVideoDriver *driver) const override;

	static void parse(parserData* data, const std::string &element,
			const FormSpecData &formdata, std::vector<BoxWidget> &widgets);

private:
	v2s32 m_pos;
	v2s32 m_geom;
	irr::video::SColor m_color;
};

