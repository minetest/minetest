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

#pragma once

#include <vector>
#include <array>
#include "irrlichttypes_extrabloated.h"

class GUIBox : public gui::IGUIElement
{
public:
	GUIBox(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const core::rect<s32> &rectangle,
		const std::array<video::SColor, 4> &colors,
		const std::array<video::SColor, 4> &bordercolors,
		const std::array<s32, 4> &borderwidths);

	virtual void draw() override;

private:
	std::array<video::SColor, 4> m_colors;
	std::array<video::SColor, 4> m_bordercolors;
	std::array<s32, 4> m_borderwidths;
};
