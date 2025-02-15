// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "object_properties.h"
#include "irrlicht_changes/printing.h"
#include "irrlichttypes_bloated.h"
#include "exceptions.h"
#include "log.h"
#include "util/serialize.h"
#include <sstream>
#include <tuple>

static const video::SColor NULL_BGCOLOR{0, 1, 1, 1};

ObjectProperties::ObjectProperties()
{
	textures.emplace_back("no_texture.png");
}

std::string ObjectProperties::dump() const
{
	std::ostringstream os(std::ios::binary);
	os << "hp_max=" << hp_max;
	os << ", breath_max=" << breath_max;
	os << ", physical=" << physical;
	os << ", collideWithObjects=" << collideWithObjects;
	os << ", collisionbox=" << collisionbox.MinEdge << "," << collisionbox.MaxEdge;
	os << ", visual=" << visual;
	os << ", mesh=" << mesh;
	os << ", visual_size=" << visual_size;
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
	os << ", spritediv=" << spritediv;
	os << ", initial_sprite_basepos=" << initial_sprite_basepos;
	os << ", is_visible=" << is_visible;
	os << ", makes_footstep_sound=" << makes_footstep_sound;
	os << ", automatic_rotate="<< automatic_rotate;
	os << ", backface_culling="<< backface_culling;
	os << ", glow=" << glow;
	os << ", nametag=" << nametag;
	os << ", nametag_color=" << "\"" << nametag_color.getAlpha() << "," << nametag_color.getRed()
			<< "," << nametag_color.getGreen() << "," << nametag_color.getBlue() << "\" ";

	if (nametag_bgcolor)
		os << ", nametag_bgcolor=" << "\"" << nametag_color.getAlpha() << "," << nametag_color.getRed()
		   << "," << nametag_color.getGreen() << "," << nametag_color.getBlue() << "\" ";
	else
		os << ", nametag_bgcolor=null ";

	os << ", selectionbox=" << selectionbox.MinEdge << "," << selectionbox.MaxEdge;
	os << ", rotate_selectionbox=" << rotate_selectionbox;
	os << ", pointable=" << Pointabilities::toStringPointabilityType(pointable);
	os << ", static_save=" << static_save;
	os << ", eye_height=" << eye_height;
	os << ", zoom_fov=" << zoom_fov;
	os << ", use_texture_alpha=" << use_texture_alpha;
	os << ", damage_texture_modifier=" << damage_texture_modifier;
	os << ", shaded=" << shaded;
	os << ", show_on_minimap=" << show_on_minimap;
	return os.str();
}

static auto tie(const ObjectProperties &o)
{
	// Make sure to add new members to this list!
	return std::tie(
	o.textures, o.colors, o.collisionbox, o.selectionbox, o.visual, o.mesh,
	o.damage_texture_modifier, o.nametag, o.infotext, o.wield_item, o.visual_size,
	o.nametag_color, o.nametag_bgcolor, o.spritediv, o.initial_sprite_basepos,
	o.stepheight, o.automatic_rotate, o.automatic_face_movement_dir_offset,
	o.automatic_face_movement_max_rotation_per_sec, o.eye_height, o.zoom_fov,
	o.hp_max, o.breath_max, o.glow, o.pointable, o.physical, o.collideWithObjects,
	o.rotate_selectionbox, o.is_visible, o.makes_footstep_sound,
	o.automatic_face_movement_dir, o.backface_culling, o.static_save, o.use_texture_alpha,
	o.shaded, o.show_on_minimap
	);
}

bool ObjectProperties::operator==(const ObjectProperties &other) const
{
	return tie(*this) == tie(other);
}

bool ObjectProperties::validate()
{
	const char *func = "ObjectProperties::validate(): ";
	bool ret = true;

	// cf. where serializeString16 is used below
	for (u32 i = 0; i < textures.size(); i++) {
		if (textures[i].size() > U16_MAX) {
			warningstream << func << "texture " << (i+1) << " has excessive length, "
				"clearing it." << std::endl;
			textures[i].clear();
			ret = false;
		}
	}
	if (nametag.length() > U16_MAX) {
		warningstream << func << "nametag has excessive length, clearing it." << std::endl;
		nametag.clear();
		ret = false;
	}
	if (infotext.length() > U16_MAX) {
		warningstream << func << "infotext has excessive length, clearing it." << std::endl;
		infotext.clear();
		ret = false;
	}
	if (wield_item.length() > U16_MAX) {
		warningstream << func << "wield_item has excessive length, clearing it." << std::endl;
		wield_item.clear();
		ret = false;
	}

	return ret;
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
	Pointabilities::serializePointabilityType(os, pointable);
	os << serializeString16(visual);
	writeV3F32(os, visual_size);
	writeU16(os, textures.size());
	for (const std::string &texture : textures) {
		os << serializeString16(texture);
	}
	writeV2S16(os, spritediv);
	writeV2S16(os, initial_sprite_basepos);
	writeU8(os, is_visible);
	writeU8(os, makes_footstep_sound);
	writeF32(os, automatic_rotate);
	os << serializeString16(mesh);
	writeU16(os, colors.size());
	for (video::SColor color : colors) {
		writeARGB8(os, color);
	}
	writeU8(os, collideWithObjects);
	writeF32(os, stepheight);
	writeU8(os, automatic_face_movement_dir);
	writeF32(os, automatic_face_movement_dir_offset);
	writeU8(os, backface_culling);
	os << serializeString16(nametag);
	writeARGB8(os, nametag_color);
	writeF32(os, automatic_face_movement_max_rotation_per_sec);
	os << serializeString16(infotext);
	os << serializeString16(wield_item);
	writeS8(os, glow);
	writeU16(os, breath_max);
	writeF32(os, eye_height);
	writeF32(os, zoom_fov);
	writeU8(os, use_texture_alpha);
	os << serializeString16(damage_texture_modifier);
	writeU8(os, shaded);
	writeU8(os, show_on_minimap);

	if (!nametag_bgcolor)
		writeARGB8(os, NULL_BGCOLOR);
	else if (nametag_bgcolor.value().getAlpha() == 0)
		writeARGB8(os, video::SColor(0, 0, 0, 0));
	else
		writeARGB8(os, nametag_bgcolor.value());

	writeU8(os, rotate_selectionbox);
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
	pointable = Pointabilities::deSerializePointabilityType(is);
	visual = deSerializeString16(is);
	visual_size = readV3F32(is);
	textures.clear();
	u32 texture_count = readU16(is);
	for (u32 i = 0; i < texture_count; i++){
		textures.push_back(deSerializeString16(is));
	}
	spritediv = readV2S16(is);
	initial_sprite_basepos = readV2S16(is);
	is_visible = readU8(is);
	makes_footstep_sound = readU8(is);
	automatic_rotate = readF32(is);
	mesh = deSerializeString16(is);
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
	nametag = deSerializeString16(is);
	nametag_color = readARGB8(is);
	automatic_face_movement_max_rotation_per_sec = readF32(is);
	infotext = deSerializeString16(is);
	wield_item = deSerializeString16(is);
	glow = readS8(is);
	breath_max = readU16(is);
	eye_height = readF32(is);
	zoom_fov = readF32(is);
	use_texture_alpha = readU8(is);
	try {
		damage_texture_modifier = deSerializeString16(is);
		u8 tmp = readU8(is);
		if (is.eof())
			return;
		shaded = tmp;
		tmp = readU8(is);
		if (is.eof())
			return;
		show_on_minimap = tmp;

		auto bgcolor = readARGB8(is);
		if (bgcolor != NULL_BGCOLOR)
			nametag_bgcolor = bgcolor;
		else
			nametag_bgcolor = std::nullopt;

		tmp = readU8(is);
		if (is.eof())
			return;
		rotate_selectionbox = tmp;
	} catch (SerializationError &e) {}
}

ObjectProperties::ChangedProperties ObjectProperties::getChange(const ObjectProperties &other)
{
	ChangedProperties change;

	change[0] = textures != other.textures;
	change[1] = colors != other.colors;
	change[2] = collisionbox != other.collisionbox;
	change[3] = selectionbox != other.selectionbox;
	change[4] = visual != other.visual;
	change[5] = mesh != other.mesh;
	change[6] = damage_texture_modifier != other.damage_texture_modifier;
	change[7] = nametag != other.nametag;
	change[8] = infotext != other.infotext;
	change[9] = wield_item != other.wield_item;
	change[10] = visual_size != other.visual_size;
	change[11] = nametag_color != other.nametag_color;
	change[12] = nametag_bgcolor != other.nametag_bgcolor;
	change[13] = spritediv != other.spritediv;
	change[14] = initial_sprite_basepos != other.initial_sprite_basepos;
	change[15] = stepheight != other.stepheight;
	change[16] = automatic_rotate != other.automatic_rotate;
	change[17] = automatic_face_movement_dir_offset != other.automatic_face_movement_dir_offset;
	change[18] = automatic_face_movement_max_rotation_per_sec !=
		other.automatic_face_movement_max_rotation_per_sec;
	change[19] = eye_height != other.eye_height;
	change[20] = zoom_fov != other.zoom_fov;
	change[21] = hp_max != other.hp_max;
	change[22] = breath_max != other.breath_max;
	change[23] = glow != other.glow;
	change[24] = pointable != other.pointable;
	change[25] = physical != other.physical ||
		collideWithObjects != other.collideWithObjects ||
		rotate_selectionbox != other.rotate_selectionbox ||
		is_visible != other.is_visible ||
		makes_footstep_sound != other.makes_footstep_sound ||
		automatic_face_movement_dir != other.automatic_face_movement_dir ||
		backface_culling != other.backface_culling ||
		use_texture_alpha != other.use_texture_alpha;
	change[26] = shaded != other.shaded || show_on_minimap != other.show_on_minimap;

	return change;
}

void ObjectProperties::serializeChanges(std::ostream &os, const ChangedProperties &fields) const
{
	writeU8(os, 0); // Version 0 uses 27 bits (actually only a quarter of the last bit :-)

	writeU32(os, fields.to_ulong());

	if (fields[0]) {
		writeU16(os, textures.size());
		for (const std::string &texture : textures) {
			os << serializeString16(texture);
		}
	}
	if (fields[1]) {
		writeU16(os, colors.size());
		for (video::SColor color : colors) {
			writeARGB8(os, color);
		}
	}
	if (fields[2]) {
		writeV3F32(os, collisionbox.MinEdge);
		writeV3F32(os, collisionbox.MaxEdge);
	}
	if (fields[3]) {
		writeV3F32(os, selectionbox.MinEdge);
		writeV3F32(os, selectionbox.MaxEdge);
	}
	if (fields[4])
		os << serializeString16(visual);
	if (fields[5])
		os << serializeString16(mesh);
	if (fields[6])
		os << serializeString16(damage_texture_modifier);
	if (fields[7])
		os << serializeString16(nametag);
	if (fields[8])
		os << serializeString16(infotext);
	if (fields[9])
		os << serializeString16(wield_item);
	if (fields[10])
		writeV3F32(os, visual_size);
	if (fields[11])
		writeARGB8(os, nametag_color);
	if (fields[12]) {
		if (!nametag_bgcolor)
			writeARGB8(os, NULL_BGCOLOR);
		else if (nametag_bgcolor.value().getAlpha() == 0)
			writeARGB8(os, video::SColor(0, 0, 0, 0));
		else
			writeARGB8(os, nametag_bgcolor.value());
	}
	if (fields[13])
		writeV2S16(os, spritediv);
	if (fields[14])
		writeV2S16(os, initial_sprite_basepos);
	if (fields[15])
		writeF32(os, stepheight);
	if (fields[16])
		writeF32(os, automatic_rotate);
	if (fields[17])
		writeF32(os, automatic_face_movement_dir_offset);
	if (fields[18])
		writeF32(os, automatic_face_movement_max_rotation_per_sec);
	if (fields[19])
		writeF32(os, eye_height);
	if (fields[20])
		writeF32(os, zoom_fov);
	if (fields[21])
		writeU16(os, hp_max);
	if (fields[22])
		writeU16(os, breath_max);
	if (fields[23])
		writeS8(os, glow);
	if (fields[24])
		Pointabilities::serializePointabilityType(os, pointable);

	if (fields[25]) {
		u8 bits = 0;
		if (physical)
			bits |= 0b00000001;
		if (collideWithObjects)
			bits |= 0b00000010;
		if (rotate_selectionbox)
			bits |= 0b00000100;
		if (is_visible)
			bits |= 0b00001000;
		if (makes_footstep_sound)
			bits |= 0b00010000;
		if (automatic_face_movement_dir)
			bits |= 0b00100000;
		if (backface_culling)
			bits |= 0b01000000;
		if (use_texture_alpha)
			bits |= 0b10000000;
		writeU8(os, bits);
	}

	if (fields[26]) {
		u8 bits = 0;
		if (shaded)
			bits |= 0b00000001;
		if (show_on_minimap)
			bits |= 0b00000010;
		writeU8(os, bits);
	}
}

void ObjectProperties::deSerializeChanges(std::istream &is)
{
	u8 version = readU8(is);
	if (version != 0)
		throw SerializationError("unsupported ObjectProperties serializeChanges version");

	ChangedProperties fields(readU32(is));

	if (fields[0]) {
		textures.clear();
		u16 texture_count = readU16(is);
		for (u16 i = 0; i < texture_count; i++){
			textures.push_back(deSerializeString16(is));
		}
	}
	if (fields[1]) {
		colors.clear();
		u16 color_count = readU16(is);
		for (u16 i = 0; i < color_count; i++){
			colors.push_back(readARGB8(is));
		}
	}
	if (fields[2]) {
		collisionbox.MinEdge = readV3F32(is);
		collisionbox.MaxEdge = readV3F32(is);
	}
	if (fields[3]) {
		selectionbox.MinEdge = readV3F32(is);
		selectionbox.MaxEdge = readV3F32(is);
	}
	if (fields[4])
		visual = deSerializeString16(is);
	if (fields[5])
		mesh = deSerializeString16(is);
	if (fields[6])
		damage_texture_modifier = deSerializeString16(is);
	if (fields[7])
		nametag = deSerializeString16(is);
	if (fields[8])
		infotext = deSerializeString16(is);
	if (fields[9])
		wield_item = deSerializeString16(is);
	if (fields[10])
		visual_size = readV3F32(is);
	if (fields[11])
		nametag_color = readARGB8(is);
	if (fields[12]) {
		auto bgcolor = readARGB8(is);
		if (bgcolor != NULL_BGCOLOR)
			nametag_bgcolor = bgcolor;
		else
			nametag_bgcolor = std::nullopt;
	}
	if (fields[13])
		spritediv = readV2S16(is);
	if (fields[14])
		initial_sprite_basepos = readV2S16(is);
	if (fields[15])
		stepheight = readF32(is);
	if (fields[16])
		automatic_rotate = readF32(is);
	if (fields[17])
		automatic_face_movement_dir_offset = readF32(is);
	if (fields[18])
		automatic_face_movement_max_rotation_per_sec = readF32(is);
	if (fields[19])
		eye_height = readF32(is);
	if (fields[20])
		zoom_fov = readF32(is);
	if (fields[21])
		hp_max = readU16(is);
	if (fields[22])
		breath_max = readU16(is);
	if (fields[23])
		glow = readS8(is);
	if (fields[24])
		pointable = Pointabilities::deSerializePointabilityType(is);

	if (fields[25]) {
		u8 bits = readU8(is);
		physical = bits & 0b00000001;
		collideWithObjects = bits & 0b00000010;
		rotate_selectionbox = bits & 0b00000100;
		is_visible = bits & 0b00001000;
		makes_footstep_sound = bits & 0b00010000;
		automatic_face_movement_dir = bits & 0b00100000;
		backface_culling = bits & 0b01000000;
		use_texture_alpha = bits & 0b10000000;
	}

	if (fields[26]) {
		u8 bits = readU8(is);
		shaded = bits & 0b00000001;
		show_on_minimap = bits & 0b00000010;
	}
}
