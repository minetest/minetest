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
	const core::rect<s32> &rectangle,
	const std::array<video::SColor, 4> &colors,
	const std::array<video::SColor, 4> &bordercolors,
	const std::array<s32, 4> &borderwidths) :
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

	std::array<s32, 4> negative_borders = {0, 0, 0, 0};
	std::array<s32, 4> positive_borders = {0, 0, 0, 0};

	for (size_t i = 0; i <= 3; i++) {
		if (m_borderwidths[i] > 0)
			positive_borders[i] = m_borderwidths[i];
		else
			negative_borders[i] = m_borderwidths[i];
	}

	v2s32 upperleft = AbsoluteRect.UpperLeftCorner;
	v2s32 lowerright = AbsoluteRect.LowerRightCorner;

	v2s32 topleft_border = {
		upperleft.X - positive_borders[3],
		upperleft.Y - positive_borders[0]
	};
	v2s32 topleft_rect = {
		upperleft.X - negative_borders[3],
		upperleft.Y - negative_borders[0]
	};

	v2s32 lowerright_border = {
		lowerright.X + positive_borders[1],
		lowerright.Y + positive_borders[2]
	};
	v2s32 lowerright_rect = {
		lowerright.X + negative_borders[1],
		lowerright.Y + negative_borders[2]
	};

	core::rect<s32> main_rect(
		topleft_rect.X,
		topleft_rect.Y,
		lowerright_rect.X,
		lowerright_rect.Y
	);

	std::array<core::rect<s32>, 4> border_rects;

	border_rects[0] = core::rect<s32>(
		topleft_border.X,
		topleft_border.Y,
		lowerright_border.X,
		topleft_rect.Y
	);

	border_rects[1] = core::rect<s32>(
		lowerright_rect.X,
		topleft_rect.Y,
		lowerright_border.X,
		lowerright_rect.Y
	);

	border_rects[2] = core::rect<s32>(
		topleft_border.X,
		lowerright_rect.Y,
		lowerright_border.X,
		lowerright_border.Y
	);

	border_rects[3] = core::rect<s32>(
		topleft_border.X,
		topleft_rect.Y,
		topleft_rect.X,
		lowerright_rect.Y
	);

	video::IVideoDriver *driver = Environment->getVideoDriver();

	driver->draw2DRectangle(main_rect, m_colors[0], m_colors[1], m_colors[3],
		m_colors[2], nullptr);

	for (size_t i = 0; i <= 3; i++)
		driver->draw2DRectangle(m_bordercolors[i], border_rects[i], nullptr);

	IGUIElement::draw();
}
