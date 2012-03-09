/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef LUAENTITY_COMMON_HEADER
#define LUAENTITY_COMMON_HEADER

#include <string>
#include "irrlichttypes.h"
#include <iostream>

struct LuaEntityProperties
{
	// Values are BS=1
	s16 hp_max;
	bool physical;
	float weight;
	core::aabbox3d<f32> collisionbox;
	std::string visual;
	v2f visual_size;
	core::array<std::string> textures;
	v2s16 spritediv;
	v2s16 initial_sprite_basepos;

	LuaEntityProperties();
	std::string dump();
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
};

#define LUAENTITY_CMD_UPDATE_POSITION 0
#define LUAENTITY_CMD_SET_TEXTURE_MOD 1
#define LUAENTITY_CMD_SET_SPRITE 2
#define LUAENTITY_CMD_PUNCHED 3
#define LUAENTITY_CMD_UPDATE_ARMOR_GROUPS 4

#endif

