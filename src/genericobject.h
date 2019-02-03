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

#include <string>
#include "irrlichttypes_bloated.h"
#include <iostream>
#include "itemgroup.h"

enum GenericCMD {
	GENERIC_CMD_SET_PROPERTIES,
	GENERIC_CMD_UPDATE_POSITION,
	GENERIC_CMD_SET_TEXTURE_MOD,
	GENERIC_CMD_SET_SPRITE,
	GENERIC_CMD_PUNCHED,
	GENERIC_CMD_UPDATE_ARMOR_GROUPS,
	GENERIC_CMD_SET_ANIMATION,
	GENERIC_CMD_SET_BONE_POSITION,
	GENERIC_CMD_ATTACH_TO,
	GENERIC_CMD_SET_PHYSICS_OVERRIDE,
	GENERIC_CMD_UPDATE_NAMETAG_ATTRIBUTES,
	GENERIC_CMD_SPAWN_INFANT,
	GENERIC_CMD_SET_ANIMATION_SPEED
};

#include "object_properties.h"
std::string gob_cmd_set_properties(const ObjectProperties &prop);
ObjectProperties gob_read_set_properties(std::istream &is);

std::string gob_cmd_update_position(
	v3f position,
	v3f velocity,
	v3f acceleration,
	v3f rotation,
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

std::string gob_cmd_punched(u16 result_hp);

std::string gob_cmd_update_armor_groups(const ItemGroupList &armor_groups);

std::string gob_cmd_update_physics_override(float physics_override_speed,
		float physics_override_jump, float physics_override_gravity,
		bool sneak, bool sneak_glitch, bool new_move);

std::string gob_cmd_update_animation(v2f frames, float frame_speed, float frame_blend, bool frame_loop);

std::string gob_cmd_update_animation_speed(float frame_speed);

std::string gob_cmd_update_bone_position(const std::string &bone, v3f position,
		v3f rotation);

std::string gob_cmd_update_attachment(int parent_id, const std::string &bone,
		v3f position, v3f rotation);

std::string gob_cmd_update_nametag_attributes(video::SColor color);

std::string gob_cmd_update_infant(u16 id, u8 type,
		const std::string &client_initialization_data);
