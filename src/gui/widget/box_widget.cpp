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

#include "gui/parser_data.h"
#include "gui/widget/box_widget.h"
#include "log.h"
#include "network/networkprotocol.h"
#include "util/string.h"

void BoxWidget::draw(video::IVideoDriver *driver) const
{
	core::rect<s32> rect(m_pos.X, m_pos.Y, m_pos.X + m_geom.X, m_pos.Y + m_geom.Y);

	driver->draw2DRectangle(m_color, rect, 0);
}

void BoxWidget::parse(parserData* data, const std::string &element,
		const FormSpecData &formdata, std::vector<BoxWidget> &widgets)
{
	std::vector<std::string> parts = split(element, ';');

	if ((parts.size() == 3) ||
			((parts.size() > 3) && (formdata.formspec_version > FORMSPEC_API_VERSION)))
	{
		std::vector<std::string> v_pos = split(parts[0], ',');
		std::vector<std::string> v_geom = split(parts[1], ',');

		MY_CHECKPOS("box", 0);
		MY_CHECKGEOM("box", 1);

		v2s32 pos = Widget::getElementBasePos(true, formdata, &v_pos);
		v2s32 geom;
		geom.X = stof(v_geom[0]) * formdata.spacing.X;
		geom.Y = stof(v_geom[1]) * formdata.spacing.Y;

		video::SColor tmp_color;

		if (parseColorString(parts[2], tmp_color, false, 0x8C)) {
			widgets.emplace_back(pos, geom, tmp_color);
		}
		else {
			errorstream<< "Invalid Box element(" << parts.size() << "): '" << element << "'  INVALID COLOR"  << std::endl;
		}

		return;
	}

	errorstream<< "Invalid Box element(" << parts.size() << "): '" << element << "'"  << std::endl;
}

