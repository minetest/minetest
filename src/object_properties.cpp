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

#include "object_properties.h"
#include "irrlichttypes_bloated.h"
#include "exceptions.h"
#include "util/serialize.h"
#include "util/basic_macros.h"
#include <sstream>

ObjectProperties::ObjectProperties()
{
	textures.emplace_back("unknown_object.png");
	colors.emplace_back(255,255,255,255);
}

std::string ObjectProperties::dump()
{
	std::ostringstream os(std::ios::binary);
	os << "hp_max=" << hp_max;
	os << ", breath_max=" << breath_max;
	os << ", physical=" << physical;
	os << ", collideWithObjects=" << collideWithObjects;
	os << ", collisionbox=" << PP(collisionbox.MinEdge) << "," << PP(collisionbox.MaxEdge);
	os << ", visual=" << visual;
	os << ", mesh=" << mesh;
	os << ", visual_size=" << PP(visual_size);
	os << ", textures=[";
	for (const std::string &texture : textures) {
		os << "\"" << texture << "\" ";
	}
	os << "]";
	os << ", colors=[";
	for (const video::SColor &color : colors) {
		os << "\"" << color.getAlpha() << "," << color.getRed() << ","
			<< color.getGreen() << "," << color.getBlue() << "\" ";
	}
	os << "]";
	os << ", spritediv=" << PP2(spritediv);
	os << ", initial_sprite_basepos=" << PP2(initial_sprite_basepos);
	os << ", is_visible=" << is_visible;
	os << ", makes_footstep_sound=" << makes_footstep_sound;
	os << ", automatic_rotate="<< automatic_rotate;
	os << ", backface_culling="<< backface_culling;
	os << ", glow=" << glow;
	os << ", nametag=" << nametag;
	os << ", nametag_color=" << "\"" << nametag_color.getAlpha() << "," << nametag_color.getRed()
			<< "," << nametag_color.getGreen() << "," << nametag_color.getBlue() << "\" ";
	os << ", selectionbox=" << PP(selectionbox.MinEdge) << "," << PP(selectionbox.MaxEdge);
	os << ", pointable=" << pointable;
	os << ", static_save=" << static_save;
	os << ", eye_height=" << eye_height;
	os << ", zoom_fov=" << zoom_fov;
	os << ", use_texture_alpha=" << use_texture_alpha;
	return os.str();
}

void ObjectProperties::serialize(std::ostream &os) const
{
	writeU8(os, 4); // PROTOCOL_VERSION >= 37
	writeU16(os, hp_max);
	writeU8(os, physical);
	writeF32(os, 0.f); // Removed property (weight)
	writeV3F32(os, collisionbox.MinEdge);
	writeV3F32(os, collisionbox.MaxEdge);
	writeV3F32(os, selectionbox.MinEdge);
	writeV3F32(os, selectionbox.MaxEdge);
	writeU8(os, pointable);
	os << serializeString(visual);
	writeV3F32(os, visual_size);
	writeU16(os, textures.size());
	for (const std::string &texture : textures) {
		os << serializeString(texture);
	}
	writeV2S16(os, spritediv);
	writeV2S16(os, initial_sprite_basepos);
	writeU8(os, is_visible);
	writeU8(os, makes_footstep_sound);
	writeF32(os, automatic_rotate);
	// Added in protocol version 14
	os << serializeString(mesh);
	writeU16(os, colors.size());
	for (video::SColor color : colors) {
		writeARGB8(os, color);
	}
	writeU8(os, collideWithObjects);
	writeF32(os, stepheight);
	writeU8(os, automatic_face_movement_dir);
	writeF32(os, automatic_face_movement_dir_offset);
	writeU8(os, backface_culling);
	os << serializeString(nametag);
	writeARGB8(os, nametag_color);
	writeF32(os, automatic_face_movement_max_rotation_per_sec);
	os << serializeString(infotext);
	os << serializeString(wield_item);
	writeS8(os, glow);
	writeU16(os, breath_max);
	writeF32(os, eye_height);
	writeF32(os, zoom_fov);
	writeU8(os, use_texture_alpha);

	// Add stuff only at the bottom.
	// Never remove anything, because we don't want new versions of this
}

void ObjectProperties::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version != 4)
		throw SerializationError("unsupported ObjectProperties version");

	hp_max = readU16(is);
	physical = readU8(is);
	readU32(is); // removed property (weight)
	collisionbox.MinEdge = readV3F32(is);
	collisionbox.MaxEdge = readV3F32(is);
	selectionbox.MinEdge = readV3F32(is);
	selectionbox.MaxEdge = readV3F32(is);
	pointable = readU8(is);
	visual = deSerializeString(is);
	visual_size = readV3F32(is);
	textures.clear();
	u32 texture_count = readU16(is);
	for (u32 i = 0; i < texture_count; i++){
		textures.push_back(deSerializeString(is));
	}
	spritediv = readV2S16(is);
	initial_sprite_basepos = readV2S16(is);
	is_visible = readU8(is);
	makes_footstep_sound = readU8(is);
	automatic_rotate = readF32(is);
	mesh = deSerializeString(is);
	colors.clear();
	u32 color_count = readU16(is);
	for (u32 i = 0; i < color_count; i++){
		colors.push_back(readARGB8(is));
	}
	collideWithObjects = readU8(is);
	stepheight = readF32(is);
	automatic_face_movement_dir = readU8(is);
	automatic_face_movement_dir_offset = readF32(is);
	backface_culling = readU8(is);
	nametag = deSerializeString(is);
	nametag_color = readARGB8(is);
	automatic_face_movement_max_rotation_per_sec = readF32(is);
	infotext = deSerializeString(is);
	wield_item = deSerializeString(is);
	glow = readS8(is);
	breath_max = readU16(is);
	eye_height = readF32(is);
	zoom_fov = readF32(is);
	use_texture_alpha = readU8(is);
}
