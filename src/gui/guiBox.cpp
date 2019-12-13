/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiBox.h"

GUIBox::GUIBox(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
	const core::rect<s32> &rectangle, const std::vector<video::SColor> &colors,
	const std::vector<video::SColor> &bordercolors, const std::vector<s32> &borderwidths) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_colors(colors),
	m_bordercolors(bordercolors),
	m_borderwidths(borderwidths)
{
}

void GUIBox::draw()
{
	if (!IsVisible)
		return;

	std::vector<s32> inner_bd = {0, 0, 0, 0};
	std::vector<s32> outer_bd = {0, 0, 0, 0};

	for (unsigned int i = 0; i <= 3; i++) {
		if (m_borderwidths[i] > 0)
			outer_bd[i] = m_borderwidths[i];
		else
			inner_bd[i] = m_borderwidths[i];
	}

	v2s32 pos = AbsoluteRect.UpperLeftCorner;
	v2s32 geom = AbsoluteRect.LowerRightCorner;

	v2s32 bd_pos = {
		pos.X - outer_bd[3],
		pos.Y - outer_bd[0]
	};

	v2s32 rect_pos = {
		pos.X - inner_bd[3],
		pos.Y - inner_bd[0]
	};

	v2s32 bd_geom = {
		geom.X + outer_bd[1],
		geom.Y + outer_bd[2]
	};
	v2s32 rect_geom = {
		geom.X + inner_bd[1],
		geom.Y + inner_bd[2]
	};

	core::rect<s32> main_rect(
		rect_pos.X,
		rect_pos.Y,
		rect_geom.X,
		rect_geom.Y
	);

	std::vector<core::rect<s32>> border_rects(4);

	border_rects[0] = core::rect<s32>(
		bd_pos.X,
		bd_pos.Y,
		bd_geom.X,
		rect_pos.Y
	);

	border_rects[1] = core::rect<s32>(
		bd_pos.X,
		rect_geom.Y,
		bd_geom.X,
		bd_geom.Y
	);

	border_rects[2] = core::rect<s32>(
		rect_geom.X,
		rect_pos.Y,
		bd_geom.X,
		rect_geom.Y
	);

	border_rects[3] = core::rect<s32>(
		bd_pos.X,
		rect_pos.Y,
		rect_pos.X,
		rect_geom.Y
	);

	video::IVideoDriver *driver = Environment->getVideoDriver();

	driver->draw2DRectangle(main_rect, m_colors[0], m_colors[1], m_colors[3],
		m_colors[2], nullptr);

	for (unsigned int i = 0; i <= 3; i++)
		driver->draw2DRectangle(m_bordercolors[i], border_rects[i], nullptr);

	IGUIElement::draw();
}
