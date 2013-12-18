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

#include "genericobject.h"
#include <sstream>
#include "util/serialize.h"

std::string gob_cmd_set_properties(const ObjectProperties &prop)
{
	std::ostringstream os(std::ios::binary);
	writeU8(os, GENERIC_CMD_SET_PROPERTIES);
	prop.serialize(os);
	return os.str();
}

ObjectProperties gob_read_set_properties(std::istream &is)
{
	ObjectProperties prop;
	prop.deSerialize(is);
	return prop;
}

std::string gob_cmd_update_position(
	v3f position,
	v3f velocity,
	v3f acceleration,
	f32 yaw,
	bool do_interpolate,
	bool is_movement_end,
	f32 update_interval
){
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, GENERIC_CMD_UPDATE_POSITION);
	// pos
	writeV3F1000(os, position);
	// velocity
	writeV3F1000(os, velocity);
	// acceleration
	writeV3F1000(os, acceleration);
	// yaw
	writeF1000(os, yaw);
	// do_interpolate
	writeU8(os, do_interpolate);
	// is_end_position (for interpolation)
	writeU8(os, is_movement_end);
	// update_interval (for interpolation)
	writeF1000(os, update_interval);
	return os.str();
}

std::string gob_cmd_set_texture_mod(const std::string &mod)
{
	std::ostringstream os(std::ios::binary);
	// command 
	writeU8(os, GENERIC_CMD_SET_TEXTURE_MOD);
	// parameters
	os<<serializeString(mod);
	return os.str();
}

std::string gob_cmd_set_sprite(
	v2s16 p,
	u16 num_frames,
	f32 framelength,
	bool select_horiz_by_yawpitch
){
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, GENERIC_CMD_SET_SPRITE);
	// parameters
	writeV2S16(os, p);
	writeU16(os, num_frames);
	writeF1000(os, framelength);
	writeU8(os, select_horiz_by_yawpitch);
	return os.str();
}

std::string gob_cmd_punched(s16 damage, s16 result_hp)
{
	std::ostringstream os(std::ios::binary);
	// command 
	writeU8(os, GENERIC_CMD_PUNCHED);
	// damage
	writeS16(os, damage);
	// result_hp
	writeS16(os, result_hp);
	return os.str();
}

std::string gob_cmd_update_armor_groups(const ItemGroupList &armor_groups)
{
	std::ostringstream os(std::ios::binary);
	writeU8(os, GENERIC_CMD_UPDATE_ARMOR_GROUPS);
	writeU16(os, armor_groups.size());
	for(ItemGroupList::const_iterator i = armor_groups.begin();
			i != armor_groups.end(); i++){
		os<<serializeString(i->first);
		writeS16(os, i->second);
	}
	return os.str();
}

std::string gob_cmd_update_physics_override(float physics_override_speed, float physics_override_jump,
		float physics_override_gravity, bool sneak, bool sneak_glitch)
{
	std::ostringstream os(std::ios::binary);
	// command 
	writeU8(os, GENERIC_CMD_SET_PHYSICS_OVERRIDE);
	// parameters
	writeF1000(os, physics_override_speed);
	writeF1000(os, physics_override_jump);
	writeF1000(os, physics_override_gravity);
	// these are sent inverted so we get true when the server sends nothing
	writeU8(os, !sneak);
	writeU8(os, !sneak_glitch);
	return os.str();
}

std::string gob_cmd_update_animation(v2f frames, float frame_speed, float frame_blend)
{
	std::ostringstream os(std::ios::binary);
	// command 
	writeU8(os, GENERIC_CMD_SET_ANIMATION);
	// parameters
	writeV2F1000(os, frames);
	writeF1000(os, frame_speed);
	writeF1000(os, frame_blend);
	return os.str();
}

std::string gob_cmd_update_bone_position(std::string bone, v3f position, v3f rotation)
{
	std::ostringstream os(std::ios::binary);
	// command 
	writeU8(os, GENERIC_CMD_SET_BONE_POSITION);
	// parameters
	os<<serializeString(bone);
	writeV3F1000(os, position);
	writeV3F1000(os, rotation);
	return os.str();
}

std::string gob_cmd_update_attachment(int parent_id, std::string bone, v3f position, v3f rotation)
{
	std::ostringstream os(std::ios::binary);
	// command 
	writeU8(os, GENERIC_CMD_SET_ATTACHMENT);
	// parameters
	writeS16(os, parent_id);
	os<<serializeString(bone);
	writeV3F1000(os, position);
	writeV3F1000(os, rotation);
	return os.str();
}

