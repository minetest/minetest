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

#include "object_properties.h"
#include "util/serialize.h"
#include <sstream>
#include <map>

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"
#define PP2(x) "("<<(x).X<<","<<(x).Y<<")"

ObjectProperties::ObjectProperties():
	hp_max(1),
	physical(false),
	weight(5),
	collisionbox(-0.5,-0.5,-0.5, 0.5,0.5,0.5),
	visual("sprite"),
	mesh(""),
	visual_size(1,1),
	spritediv(1,1),
	initial_sprite_basepos(0,0),
	is_visible(true),
	makes_footstep_sound(false),
	automatic_rotate(0)
{
	// Nothing to do for animation_bone_position
	// Nothing to do for animation_bone_rotation
	textures.push_back("unknown_object.png");
}

std::string ObjectProperties::dump()
{
	std::ostringstream os(std::ios::binary);
	os<<"hp_max="<<hp_max;
	os<<", physical="<<physical;
	os<<", weight="<<weight;
	os<<", collisionbox="<<PP(collisionbox.MinEdge)<<","<<PP(collisionbox.MaxEdge);
	os<<", visual="<<visual;
	os<<", mesh="<<mesh;
	os<<", visual_size="<<PP2(visual_size);

	os<<", animation_bone_position=[";
	for(std::map<std::string, v3f>::const_iterator ii = animation_bone_position.begin(); ii != animation_bone_position.end(); ++ii){
		std::string bone_name = (*ii).first;
		v3f bone_pos = (*ii).second;
		os<<bone_name<<" "<<bone_pos.X<<","<<bone_pos.Y<<","<<bone_pos.Z<<"\"";
	}
	os<<"]";
	os<<", animation_bone_rotation=[";
	for(std::map<std::string, v3f>::const_iterator ii = animation_bone_rotation.begin(); ii != animation_bone_rotation.end(); ++ii){
		std::string bone_name = (*ii).first;
		v3f bone_rot = (*ii).second;
		os<<bone_name<<" "<<bone_rot.X<<","<<bone_rot.Y<<","<<bone_rot.Z<<"\"";
	}
	os<<"]";

	os<<", textures=[";
	for(u32 i=0; i<textures.size(); i++){
		os<<"\""<<textures[i]<<"\" ";
	}
	os<<"]";
	os<<", spritediv="<<PP2(spritediv);
	os<<", initial_sprite_basepos="<<PP2(initial_sprite_basepos);
	os<<", is_visible="<<is_visible;
	os<<", makes_footstep_sound="<<makes_footstep_sound;
	os<<", automatic_rotate="<<automatic_rotate;
	return os.str();
}

void ObjectProperties::serialize(std::ostream &os) const
{
	writeU8(os, 1); // version
	writeS16(os, hp_max);
	writeU8(os, physical);
	writeF1000(os, weight);
	writeV3F1000(os, collisionbox.MinEdge);
	writeV3F1000(os, collisionbox.MaxEdge);
	os<<serializeString(visual);
	os<<serializeString(mesh);

	writeU16(os, animation_bone_position.size());
	for(std::map<std::string, v3f>::const_iterator ii = animation_bone_position.begin(); ii != animation_bone_position.end(); ++ii){
		os<<serializeString((*ii).first);
		writeV3F1000(os, (*ii).second);
	}
	writeU16(os, animation_bone_rotation.size());
	for(std::map<std::string, v3f>::const_iterator ii = animation_bone_rotation.begin(); ii != animation_bone_rotation.end(); ++ii){
		os<<serializeString((*ii).first);
		writeV3F1000(os, (*ii).second);
	}

	writeV2F1000(os, visual_size);
	writeU16(os, textures.size());
	for(u32 i=0; i<textures.size(); i++){
		os<<serializeString(textures[i]);
	}

	writeV2S16(os, spritediv);
	writeV2S16(os, initial_sprite_basepos);
	writeU8(os, is_visible);
	writeU8(os, makes_footstep_sound);
	writeF1000(os, automatic_rotate);
}

void ObjectProperties::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 1) throw SerializationError(
			"unsupported ObjectProperties version");
	hp_max = readS16(is);
	physical = readU8(is);
	weight = readF1000(is);
	collisionbox.MinEdge = readV3F1000(is);
	collisionbox.MaxEdge = readV3F1000(is);
	visual = deSerializeString(is);
	mesh = deSerializeString(is);

	u32 animation_bone_position_count = readU16(is);
	for(u32 i=0; i<animation_bone_position_count; i++){
		std::string bone_name = deSerializeString(is);
		v3f bone_pos = readV3F1000(is);
		animation_bone_position[bone_name] = bone_pos;
	}
	u32 animation_bone_rotation_count = readU16(is);
	for(u32 i=0; i<animation_bone_rotation_count; i++){
		std::string bone_name = deSerializeString(is);
		v3f bone_rot = readV3F1000(is);
		animation_bone_rotation[bone_name] = bone_rot;
	}

	visual_size = readV2F1000(is);
	textures.clear();
	u32 texture_count = readU16(is);
	for(u32 i=0; i<texture_count; i++){
		textures.push_back(deSerializeString(is));
	}

	spritediv = readV2S16(is);
	initial_sprite_basepos = readV2S16(is);
	is_visible = readU8(is);
	makes_footstep_sound = readU8(is);
	try{
		automatic_rotate = readF1000(is);
	}catch(SerializationError &e){}
}


