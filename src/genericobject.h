/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef GENERICOBJECT_HEADER
#define GENERICOBJECT_HEADER

#include <string>
#include "irrlichttypes_bloated.h"
#include <iostream>

#define GENERIC_CMD_SET_PROPERTIES 0
#define GENERIC_CMD_UPDATE_POSITION 1
#define GENERIC_CMD_SET_TEXTURE_MOD 2
#define GENERIC_CMD_SET_SPRITE 3
#define GENERIC_CMD_PUNCHED 4
#define GENERIC_CMD_UPDATE_ARMOR_GROUPS 5

#include "object_properties.h"
std::string gob_cmd_set_properties(const ObjectProperties &prop);
ObjectProperties gob_read_set_properties(std::istream &is);

std::string gob_cmd_update_position(
	v3f position,
	v3f velocity,
	v3f acceleration,
	f32 yaw,
	bool do_interpolate,
	bool is_movement_end,
	f32 update_interval
);

std::string gob_cmd_set_texture_mod(const std::string &mod);

std::string gob_cmd_set_sprite(
	v2s16 p,
	u16 num_frames,
	f32 framelength,
	bool select_horiz_by_yawpitch
);

std::string gob_cmd_punched(s16 damage, s16 result_hp);

#include "itemgroup.h"
std::string gob_cmd_update_armor_groups(const ItemGroupList &armor_groups);

#endif

