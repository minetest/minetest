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

#include "nodedef.h"

#include "itemdef.h"
#ifndef SERVER
#include "client/mesh.h"
#include "client/shader.h"
#include "client/client.h"
#include "client/renderingengine.h"
#include "client/tile.h"
#include <IMeshManipulator.h>
#endif
#include "log.h"
#include "settings.h"
#include "nameidmapping.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "exceptions.h"
#include "debug.h"
#include "gamedef.h"
#include "mapnode.h"
#include <fstream> // Used in applyTextureOverrides()
#include <algorithm>
#include <cmath>

/*
	NodeBox
*/

void NodeBox::reset()
{
	type = NODEBOX_REGULAR;
	// default is empty
	fixed.clear();
	// default is sign/ladder-like
	wall_top = aabb3f(-BS/2, BS/2-BS/16., -BS/2, BS/2, BS/2, BS/2);
	wall_bottom = aabb3f(-BS/2, -BS/2, -BS/2, BS/2, -BS/2+BS/16., BS/2);
	wall_side = aabb3f(-BS/2, -BS/2, -BS/2, -BS/2+BS/16., BS/2, BS/2);
	// no default for other parts
	connect_top.clear();
	connect_bottom.clear();
	connect_front.clear();
	connect_left.clear();
	connect_back.clear();
	connect_right.clear();
	disconnected_top.clear();
	disconnected_bottom.clear();
	disconnected_front.clear();
	disconnected_left.clear();
	disconnected_back.clear();
	disconnected_right.clear();
	disconnected.clear();
	disconnected_sides.clear();
}

void NodeBox::serialize(std::ostream &os, u16 protocol_version) const
{
	// Protocol >= 36
	const u8 version = 6;
	writeU8(os, version);

	switch (type) {
	case NODEBOX_LEVELED:
	case NODEBOX_FIXED:
		writeU8(os, type);

		writeU16(os, fixed.size());
		for (const aabb3f &nodebox : fixed) {
			writeV3F32(os, nodebox.MinEdge);
			writeV3F32(os, nodebox.MaxEdge);
		}
		break;
	case NODEBOX_WALLMOUNTED:
		writeU8(os, type);

		writeV3F32(os, wall_top.MinEdge);
		writeV3F32(os, wall_top.MaxEdge);
		writeV3F32(os, wall_bottom.MinEdge);
		writeV3F32(os, wall_bottom.MaxEdge);
		writeV3F32(os, wall_side.MinEdge);
		writeV3F32(os, wall_side.MaxEdge);
		break;
	case NODEBOX_CONNECTED:
		writeU8(os, type);

#define WRITEBOX(box) \
		writeU16(os, (box).size()); \
		for (const aabb3f &i: (box)) { \
			writeV3F32(os, i.MinEdge); \
			writeV3F32(os, i.MaxEdge); \
		};

		WRITEBOX(fixed);
		WRITEBOX(connect_top);
		WRITEBOX(connect_bottom);
		WRITEBOX(connect_front);
		WRITEBOX(connect_left);
		WRITEBOX(connect_back);
		WRITEBOX(connect_right);
		WRITEBOX(disconnected_top);
		WRITEBOX(disconnected_bottom);
		WRITEBOX(disconnected_front);
		WRITEBOX(disconnected_left);
		WRITEBOX(disconnected_back);
		WRITEBOX(disconnected_right);
		WRITEBOX(disconnected);
		WRITEBOX(disconnected_sides);
		break;
	default:
		writeU8(os, type);
		break;
	}
}

void NodeBox::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if (version < 6)
		throw SerializationError("unsupported NodeBox version");

	reset();

	type = (enum NodeBoxType)readU8(is);

	if(type == NODEBOX_FIXED || type == NODEBOX_LEVELED)
	{
		u16 fixed_count = readU16(is);
		while(fixed_count--)
		{
			aabb3f box;
			box.MinEdge = readV3F32(is);
			box.MaxEdge = readV3F32(is);
			fixed.push_back(box);
		}
	}
	else if(type == NODEBOX_WALLMOUNTED)
	{
		wall_top.MinEdge = readV3F32(is);
		wall_top.MaxEdge = readV3F32(is);
		wall_bottom.MinEdge = readV3F32(is);
		wall_bottom.MaxEdge = readV3F32(is);
		wall_side.MinEdge = readV3F32(is);
		wall_side.MaxEdge = readV3F32(is);
	}
	else if (type == NODEBOX_CONNECTED)
	{
#define READBOXES(box) { \
		count = readU16(is); \
		(box).reserve(count); \
		while (count--) { \
			v3f min = readV3F32(is); \
			v3f max = readV3F32(is); \
			(box).emplace_back(min, max); }; }

		u16 count;

		READBOXES(fixed);
		READBOXES(connect_top);
		READBOXES(connect_bottom);
		READBOXES(connect_front);
		READBOXES(connect_left);
		READBOXES(connect_back);
		READBOXES(connect_right);
		READBOXES(disconnected_top);
		READBOXES(disconnected_bottom);
		READBOXES(disconnected_front);
		READBOXES(disconnected_left);
		READBOXES(disconnected_back);
		READBOXES(disconnected_right);
		READBOXES(disconnected);
		READBOXES(disconnected_sides);
	}
}

/*
	TileDef
*/

#define TILE_FLAG_BACKFACE_CULLING	(1 << 0)
#define TILE_FLAG_TILEABLE_HORIZONTAL	(1 << 1)
#define TILE_FLAG_TILEABLE_VERTICAL	(1 << 2)
#define TILE_FLAG_HAS_COLOR	(1 << 3)
#define TILE_FLAG_HAS_SCALE	(1 << 4)
#define TILE_FLAG_HAS_ALIGN_STYLE	(1 << 5)

void TileDef::serialize(std::ostream &os, u16 protocol_version) const
{
	// protocol_version >= 36
	u8 version = 6;
	writeU8(os, version);

	os << serializeString(name);
	animation.serialize(os, version);
	bool has_scale = scale > 0;
	u16 flags = 0;
	if (backface_culling)
		flags |= TILE_FLAG_BACKFACE_CULLING;
	if (tileable_horizontal)
		flags |= TILE_FLAG_TILEABLE_HORIZONTAL;
	if (tileable_vertical)
		flags |= TILE_FLAG_TILEABLE_VERTICAL;
	if (has_color)
		flags |= TILE_FLAG_HAS_COLOR;
	if (has_scale)
		flags |= TILE_FLAG_HAS_SCALE;
	if (align_style != ALIGN_STYLE_NODE)
		flags |= TILE_FLAG_HAS_ALIGN_STYLE;
	writeU16(os, flags);
	if (has_color) {
		writeU8(os, color.getRed());
		writeU8(os, color.getGreen());
		writeU8(os, color.getBlue());
	}
	if (has_scale)
		writeU8(os, scale);
	if (align_style != ALIGN_STYLE_NODE)
		writeU8(os, align_style);
}

void TileDef::deSerialize(std::istream &is, u8 contentfeatures_version,
	NodeDrawType drawtype)
{
	int version = readU8(is);
	if (version < 6)
		throw SerializationError("unsupported TileDef version");
	name = deSerializeString(is);
	animation.deSerialize(is, version);
	u16 flags = readU16(is);
	backface_culling = flags & TILE_FLAG_BACKFACE_CULLING;
	tileable_horizontal = flags & TILE_FLAG_TILEABLE_HORIZONTAL;
	tileable_vertical = flags & TILE_FLAG_TILEABLE_VERTICAL;
	has_color = flags & TILE_FLAG_HAS_COLOR;
	bool has_scale = flags & TILE_FLAG_HAS_SCALE;
	bool has_align_style = flags & TILE_FLAG_HAS_ALIGN_STYLE;
	if (has_color) {
		color.setRed(readU8(is));
		color.setGreen(readU8(is));
		color.setBlue(readU8(is));
	}
	scale = has_scale ? readU8(is) : 0;
	if (has_align_style)
		align_style = static_cast<AlignStyle>(readU8(is));
	else
		align_style = ALIGN_STYLE_NODE;
}

void TextureSettings::readSettings()
{
	connected_glass                = g_settings->getBool("connected_glass");
	opaque_water                   = g_settings->getBool("opaque_water");
	bool enable_shaders            = g_settings->getBool("enable_shaders");
	bool enable_bumpmapping        = g_settings->getBool("enable_bumpmapping");
	bool enable_parallax_occlusion = g_settings->getBool("enable_parallax_occlusion");
	bool smooth_lighting           = g_settings->getBool("smooth_lighting");
	enable_mesh_cache              = g_settings->getBool("enable_mesh_cache");
	enable_minimap                 = g_settings->getBool("enable_minimap");
	node_texture_size              = g_settings->getU16("texture_min_size");
	std::string leaves_style_str   = g_settings->get("leaves_style");
	std::string world_aligned_mode_str = g_settings->get("world_aligned_mode");
	std::string autoscale_mode_str = g_settings->get("autoscale_mode");

	// Mesh cache is not supported in combination with smooth lighting
	if (smooth_lighting)
		enable_mesh_cache = false;

	use_normal_texture = enable_shaders &&
		(enable_bumpmapping || enable_parallax_occlusion);
	if (leaves_style_str == "fancy") {
		leaves_style = LEAVES_FANCY;
	} else if (leaves_style_str == "simple") {
		leaves_style = LEAVES_SIMPLE;
	} else {
		leaves_style = LEAVES_OPAQUE;
	}

	if (world_aligned_mode_str == "enable")
		world_aligned_mode = WORLDALIGN_ENABLE;
	else if (world_aligned_mode_str == "force_solid")
		world_aligned_mode = WORLDALIGN_FORCE;
	else if (world_aligned_mode_str == "force_nodebox")
		world_aligned_mode = WORLDALIGN_FORCE_NODEBOX;
	else
		world_aligned_mode = WORLDALIGN_DISABLE;

	if (autoscale_mode_str == "enable")
		autoscale_mode = AUTOSCALE_ENABLE;
	else if (autoscale_mode_str == "force")
		autoscale_mode = AUTOSCALE_FORCE;
	else
		autoscale_mode = AUTOSCALE_DISABLE;
}

/*
	ContentFeatures
*/

ContentFeatures::ContentFeatures()
{
	reset();
}

ContentFeatures::~ContentFeatures()
{
#ifndef SERVER
	for (u16 j = 0; j < 6; j++) {
		delete tiles[j].layers[0].frames;
		delete tiles[j].layers[1].frames;
	}
	for (u16 j = 0; j < CF_SPECIAL_COUNT; j++)
		delete special_tiles[j].layers[0].frames;
#endif
}

void ContentFeatures::reset()
{
	/*
		Cached stuff
	*/
#ifndef SERVER
	solidness = 2;
	visual_solidness = 0;
	backface_culling = true;

#endif
	has_on_construct = false;
	has_on_destruct = false;
	has_after_destruct = false;
	/*
		Actual data

		NOTE: Most of this is always overridden by the default values given
		      in builtin.lua
	*/
	name = "";
	groups.clear();
	// Unknown nodes can be dug
	groups["dig_immediate"] = 2;
	drawtype = NDT_NORMAL;
	mesh = "";
#ifndef SERVER
	for (auto &i : mesh_ptr)
		i = NULL;
	minimap_color = video::SColor(0, 0, 0, 0);
#endif
	visual_scale = 1.0;
	for (auto &i : tiledef)
		i = TileDef();
	for (auto &j : tiledef_special)
		j = TileDef();
	alpha = 255;
	post_effect_color = video::SColor(0, 0, 0, 0);
	param_type = CPT_NONE;
	param_type_2 = CPT2_NONE;
	is_ground_content = false;
	light_propagates = false;
	sunlight_propagates = false;
	walkable = true;
	pointable = true;
	diggable = true;
	climbable = false;
	buildable_to = false;
	floodable = false;
	rightclickable = true;
	leveled = 0;
	leveled_max = LEVELED_MAX;
	liquid_type = LIQUID_NONE;
	liquid_alternative_flowing = "";
	liquid_alternative_flowing_id = CONTENT_IGNORE;
	liquid_alternative_source = "";
	liquid_alternative_source_id = CONTENT_IGNORE;
	liquid_viscosity = 0;
	liquid_renewable = true;
	liquid_range = LIQUID_LEVEL_MAX+1;
	drowning = 0;
	light_source = 0;
	damage_per_second = 0;
	node_box = NodeBox();
	selection_box = NodeBox();
	collision_box = NodeBox();
	waving = 0;
	legacy_facedir_simple = false;
	legacy_wallmounted = false;
	sound_footstep = SimpleSoundSpec();
	sound_dig = SimpleSoundSpec("__group");
	sound_dug = SimpleSoundSpec();
	connects_to.clear();
	connects_to_ids.clear();
	connect_sides = 0;
	color = video::SColor(0xFFFFFFFF);
	palette_name = "";
	palette = NULL;
	node_dig_prediction = "air";
}

void ContentFeatures::serialize(std::ostream &os, u16 protocol_version) const
{
	const u8 version = CONTENTFEATURES_VERSION;
	writeU8(os, version);

	// general
	os << serializeString(name);
	writeU16(os, groups.size());
	for (const auto &group : groups) {
		os << serializeString(group.first);
		writeS16(os, group.second);
	}
	writeU8(os, param_type);
	writeU8(os, param_type_2);

	// visual
	writeU8(os, drawtype);
	os << serializeString(mesh);
	writeF32(os, visual_scale);
	writeU8(os, 6);
	for (const TileDef &td : tiledef)
		td.serialize(os, protocol_version);
	for (const TileDef &td : tiledef_overlay)
		td.serialize(os, protocol_version);
	writeU8(os, CF_SPECIAL_COUNT);
	for (const TileDef &td : tiledef_special) {
		td.serialize(os, protocol_version);
	}
	writeU8(os, alpha);
	writeU8(os, color.getRed());
	writeU8(os, color.getGreen());
	writeU8(os, color.getBlue());
	os << serializeString(palette_name);
	writeU8(os, waving);
	writeU8(os, connect_sides);
	writeU16(os, connects_to_ids.size());
	for (u16 connects_to_id : connects_to_ids)
		writeU16(os, connects_to_id);
	writeARGB8(os, post_effect_color);
	writeU8(os, leveled);

	// lighting
	writeU8(os, light_propagates);
	writeU8(os, sunlight_propagates);
	writeU8(os, light_source);

	// map generation
	writeU8(os, is_ground_content);

	// interaction
	writeU8(os, walkable);
	writeU8(os, pointable);
	writeU8(os, diggable);
	writeU8(os, climbable);
	writeU8(os, buildable_to);
	writeU8(os, rightclickable);
	writeU32(os, damage_per_second);

	// liquid
	writeU8(os, liquid_type);
	os << serializeString(liquid_alternative_flowing);
	os << serializeString(liquid_alternative_source);
	writeU8(os, liquid_viscosity);
	writeU8(os, liquid_renewable);
	writeU8(os, liquid_range);
	writeU8(os, drowning);
	writeU8(os, floodable);

	// node boxes
	node_box.serialize(os, protocol_version);
	selection_box.serialize(os, protocol_version);
	collision_box.serialize(os, protocol_version);

	// sound
	sound_footstep.serialize(os, version);
	sound_dig.serialize(os, version);
	sound_dug.serialize(os, version);

	// legacy
	writeU8(os, legacy_facedir_simple);
	writeU8(os, legacy_wallmounted);

	os << serializeString(node_dig_prediction);
	writeU8(os, leveled_max);
}

void ContentFeatures::correctAlpha(TileDef *tiles, int length)
{
	// alpha == 0 means that the node is using texture alpha
	if (alpha == 0 || alpha == 255)
		return;

	for (int i = 0; i < length; i++) {
		if (tiles[i].name.empty())
			continue;
		std::stringstream s;
		s << tiles[i].name << "^[noalpha^[opacity:" << ((int)alpha);
		tiles[i].name = s.str();
	}
}

void ContentFeatures::deSerialize(std::istream &is)
{
	// version detection
	const u8 version = readU8(is);
	if (version < CONTENTFEATURES_VERSION)
		throw SerializationError("unsupported ContentFeatures version");

	// general
	name = deSerializeString(is);
	groups.clear();
	u32 groups_size = readU16(is);
	for (u32 i = 0; i < groups_size; i++) {
		std::string name = deSerializeString(is);
		int value = readS16(is);
		groups[name] = value;
	}
	param_type = (enum ContentParamType) readU8(is);
	param_type_2 = (enum ContentParamType2) readU8(is);

	// visual
	drawtype = (enum NodeDrawType) readU8(is);
	mesh = deSerializeString(is);
	visual_scale = readF32(is);
	if (readU8(is) != 6)
		throw SerializationError("unsupported tile count");
	for (TileDef &td : tiledef)
		td.deSerialize(is, version, drawtype);
	for (TileDef &td : tiledef_overlay)
		td.deSerialize(is, version, drawtype);
	if (readU8(is) != CF_SPECIAL_COUNT)
		throw SerializationError("unsupported CF_SPECIAL_COUNT");
	for (TileDef &td : tiledef_special)
		td.deSerialize(is, version, drawtype);
	alpha = readU8(is);
	color.setRed(readU8(is));
	color.setGreen(readU8(is));
	color.setBlue(readU8(is));
	palette_name = deSerializeString(is);
	waving = readU8(is);
	connect_sides = readU8(is);
	u16 connects_to_size = readU16(is);
	connects_to_ids.clear();
	for (u16 i = 0; i < connects_to_size; i++)
		connects_to_ids.push_back(readU16(is));
	post_effect_color = readARGB8(is);
	leveled = readU8(is);

	// lighting-related
	light_propagates = readU8(is);
	sunlight_propagates = readU8(is);
	light_source = readU8(is);
	light_source = MYMIN(light_source, LIGHT_MAX);

	// map generation
	is_ground_content = readU8(is);

	// interaction
	walkable = readU8(is);
	pointable = readU8(is);
	diggable = readU8(is);
	climbable = readU8(is);
	buildable_to = readU8(is);
	rightclickable = readU8(is);
	damage_per_second = readU32(is);

	// liquid
	liquid_type = (enum LiquidType) readU8(is);
	liquid_alternative_flowing = deSerializeString(is);
	liquid_alternative_source = deSerializeString(is);
	liquid_viscosity = readU8(is);
	liquid_renewable = readU8(is);
	liquid_range = readU8(is);
	drowning = readU8(is);
	floodable = readU8(is);

	// node boxes
	node_box.deSerialize(is);
	selection_box.deSerialize(is);
	collision_box.deSerialize(is);

	// sounds
	sound_footstep.deSerialize(is, version);
	sound_dig.deSerialize(is, version);
	sound_dug.deSerialize(is, version);

	// read legacy properties
	legacy_facedir_simple = readU8(is);
	legacy_wallmounted = readU8(is);

	try {
		node_dig_prediction = deSerializeString(is);
		u8 tmp_leveled_max = readU8(is);
		if (is.eof()) /* readU8 doesn't throw exceptions so we have to do this */
			throw SerializationError("");
		leveled_max = tmp_leveled_max;
	} catch(SerializationError &e) {};
}

#ifndef SERVER
static void fillTileAttribs(ITextureSource *tsrc, TileLayer *layer,
		const TileSpec &tile, const TileDef &tiledef, video::SColor color,
		u8 material_type, u32 shader_id, bool backface_culling,
		const TextureSettings &tsettings)
{
	layer->shader_id     = shader_id;
	layer->texture       = tsrc->getTextureForMesh(tiledef.name, &layer->texture_id);
	layer->material_type = material_type;

	bool has_scale = tiledef.scale > 0;
	bool use_autoscale = tsettings.autoscale_mode == AUTOSCALE_FORCE ||
		(tsettings.autoscale_mode == AUTOSCALE_ENABLE && !has_scale);
	if (use_autoscale && layer->texture) {
		auto texture_size = layer->texture->getOriginalSize();
		float base_size = tsettings.node_texture_size;
		float size = std::fmin(texture_size.Width, texture_size.Height);
		layer->scale = std::fmax(base_size, size) / base_size;
	} else if (has_scale) {
		layer->scale = tiledef.scale;
	} else {
		layer->scale = 1;
	}
	if (!tile.world_aligned)
		layer->scale = 1;

	// Normal texture and shader flags texture
	if (tsettings.use_normal_texture) {
		layer->normal_texture = tsrc->getNormalTexture(tiledef.name);
	}
	layer->flags_texture = tsrc->getShaderFlagsTexture(layer->normal_texture ? true : false);

	// Material flags
	layer->material_flags = 0;
	if (backface_culling)
		layer->material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
	if (tiledef.animation.type != TAT_NONE)
		layer->material_flags |= MATERIAL_FLAG_ANIMATION;
	if (tiledef.tileable_horizontal)
		layer->material_flags |= MATERIAL_FLAG_TILEABLE_HORIZONTAL;
	if (tiledef.tileable_vertical)
		layer->material_flags |= MATERIAL_FLAG_TILEABLE_VERTICAL;

	// Color
	layer->has_color = tiledef.has_color;
	if (tiledef.has_color)
		layer->color = tiledef.color;
	else
		layer->color = color;

	// Animation parameters
	int frame_count = 1;
	if (layer->material_flags & MATERIAL_FLAG_ANIMATION) {
		int frame_length_ms;
		tiledef.animation.determineParams(layer->texture->getOriginalSize(),
				&frame_count, &frame_length_ms, NULL);
		layer->animation_frame_count = frame_count;
		layer->animation_frame_length_ms = frame_length_ms;
	}

	if (frame_count == 1) {
		layer->material_flags &= ~MATERIAL_FLAG_ANIMATION;
	} else {
		std::ostringstream os(std::ios::binary);
		if (!layer->frames) {
			layer->frames = new std::vector<FrameSpec>();
		}
		layer->frames->resize(frame_count);

		for (int i = 0; i < frame_count; i++) {

			FrameSpec frame;

			os.str("");
			os << tiledef.name;
			tiledef.animation.getTextureModifer(os,
					layer->texture->getOriginalSize(), i);

			frame.texture = tsrc->getTextureForMesh(os.str(), &frame.texture_id);
			if (layer->normal_texture)
				frame.normal_texture = tsrc->getNormalTexture(os.str());
			frame.flags_texture = layer->flags_texture;
			(*layer->frames)[i] = frame;
		}
	}
}

bool ContentFeatures::textureAlphaCheck(ITextureSource *tsrc, const TileDef *tiles, int length)
{
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	static thread_local bool long_warning_printed = false;
	std::set<std::string> seen;
	for (int i = 0; i < length; i++) {
		if (seen.find(tiles[i].name) != seen.end())
			continue;
		seen.insert(tiles[i].name);

		// Load the texture and see if there's any transparent pixels
		video::ITexture *texture = tsrc->getTexture(tiles[i].name);
		video::IImage *image = driver->createImage(texture,
			core::position2d<s32>(0, 0), texture->getOriginalSize());
		if (!image)
			continue;
		core::dimension2d<u32> dim = image->getDimension();
		bool ok = true;
		for (u16 x = 0; x < dim.Width; x++) {
			for (u16 y = 0; y < dim.Height; y++) {
				if (image->getPixel(x, y).getAlpha() < 255) {
					ok = false;
					goto break_loop;
				}
			}
		}

break_loop:
		image->drop();
		if (!ok) {
			warningstream << "Texture \"" << tiles[i].name << "\" of "
				<< name << " has transparent pixels, assuming "
				"use_texture_alpha = true." << std::endl;
			if (!long_warning_printed) {
				warningstream << "  This warning can be a false-positive if "
					"unused pixels in the texture are transparent. However if "
					"it is meant to be transparent, you *MUST* update the "
					"nodedef and set use_texture_alpha = true! This compatibility "
					"code will be removed in a few releases." << std::endl;
				long_warning_printed = true;
			}
			return true;
		}
	}
	return false;
}

bool isWorldAligned(AlignStyle style, WorldAlignMode mode, NodeDrawType drawtype)
{
	if (style == ALIGN_STYLE_WORLD)
		return true;
	if (mode == WORLDALIGN_DISABLE)
		return false;
	if (style == ALIGN_STYLE_USER_DEFINED)
		return true;
	if (drawtype == NDT_NORMAL)
		return mode >= WORLDALIGN_FORCE;
	if (drawtype == NDT_NODEBOX)
		return mode >= WORLDALIGN_FORCE_NODEBOX;
	return false;
}

void ContentFeatures::updateTextures(ITextureSource *tsrc, IShaderSource *shdsrc,
	scene::IMeshManipulator *meshmanip, Client *client, const TextureSettings &tsettings)
{
	// minimap pixel color - the average color of a texture
	if (tsettings.enable_minimap && !tiledef[0].name.empty())
		minimap_color = tsrc->getTextureAverageColor(tiledef[0].name);

	// Figure out the actual tiles to use
	TileDef tdef[6];
	for (u32 j = 0; j < 6; j++) {
		tdef[j] = tiledef[j];
		if (tdef[j].name.empty())
			tdef[j].name = "unknown_node.png";
	}
	// also the overlay tiles
	TileDef tdef_overlay[6];
	for (u32 j = 0; j < 6; j++)
		tdef_overlay[j] = tiledef_overlay[j];
	// also the special tiles
	TileDef tdef_spec[6];
	for (u32 j = 0; j < CF_SPECIAL_COUNT; j++)
		tdef_spec[j] = tiledef_special[j];

	bool is_liquid = false;

	u8 material_type = (alpha == 255) ?
		TILE_MATERIAL_BASIC : TILE_MATERIAL_ALPHA;

	switch (drawtype) {
	default:
	case NDT_NORMAL:
		material_type = (alpha == 255) ?
			TILE_MATERIAL_OPAQUE : TILE_MATERIAL_ALPHA;
		solidness = 2;
		break;
	case NDT_AIRLIKE:
		solidness = 0;
		break;
	case NDT_LIQUID:
		assert(liquid_type == LIQUID_SOURCE);
		if (tsettings.opaque_water)
			alpha = 255;
		solidness = 1;
		is_liquid = true;
		break;
	case NDT_FLOWINGLIQUID:
		assert(liquid_type == LIQUID_FLOWING);
		solidness = 0;
		if (tsettings.opaque_water)
			alpha = 255;
		is_liquid = true;
		break;
	case NDT_GLASSLIKE:
		solidness = 0;
		visual_solidness = 1;
		break;
	case NDT_GLASSLIKE_FRAMED:
		solidness = 0;
		visual_solidness = 1;
		break;
	case NDT_GLASSLIKE_FRAMED_OPTIONAL:
		solidness = 0;
		visual_solidness = 1;
		drawtype = tsettings.connected_glass ? NDT_GLASSLIKE_FRAMED : NDT_GLASSLIKE;
		break;
	case NDT_ALLFACES:
		solidness = 0;
		visual_solidness = 1;
		break;
	case NDT_ALLFACES_OPTIONAL:
		if (tsettings.leaves_style == LEAVES_FANCY) {
			drawtype = NDT_ALLFACES;
			solidness = 0;
			visual_solidness = 1;
		} else if (tsettings.leaves_style == LEAVES_SIMPLE) {
			for (u32 j = 0; j < 6; j++) {
				if (!tdef_spec[j].name.empty())
					tdef[j].name = tdef_spec[j].name;
			}
			drawtype = NDT_GLASSLIKE;
			solidness = 0;
			visual_solidness = 1;
		} else {
			drawtype = NDT_NORMAL;
			solidness = 2;
			for (TileDef &td : tdef)
				td.name += std::string("^[noalpha");
		}
		if (waving >= 1)
			material_type = TILE_MATERIAL_WAVING_LEAVES;
		break;
	case NDT_PLANTLIKE:
		solidness = 0;
		if (waving >= 1)
			material_type = TILE_MATERIAL_WAVING_PLANTS;
		break;
	case NDT_FIRELIKE:
		solidness = 0;
		break;
	case NDT_MESH:
	case NDT_NODEBOX:
		if (alpha == 255 && textureAlphaCheck(tsrc, tdef, 6))
			alpha = 0;

		solidness = 0;
		if (waving == 1)
			material_type = TILE_MATERIAL_WAVING_PLANTS;
		else if (waving == 2)
			material_type = TILE_MATERIAL_WAVING_LEAVES;
		else if (waving == 3)
			material_type = (alpha == 255) ? TILE_MATERIAL_WAVING_LIQUID_OPAQUE :
				TILE_MATERIAL_WAVING_LIQUID_BASIC;
		else if (alpha == 255)
			material_type = TILE_MATERIAL_OPAQUE;
		break;
	case NDT_TORCHLIKE:
	case NDT_SIGNLIKE:
	case NDT_FENCELIKE:
	case NDT_RAILLIKE:
		solidness = 0;
		break;
	case NDT_PLANTLIKE_ROOTED:
		solidness = 2;
		break;
	}

	if (is_liquid) {
		// Vertex alpha is no longer supported, correct if necessary.
		correctAlpha(tdef, 6);
		correctAlpha(tdef_overlay, 6);
		correctAlpha(tdef_spec, CF_SPECIAL_COUNT);

		if (waving == 3) {
			material_type = (alpha == 255) ? TILE_MATERIAL_WAVING_LIQUID_OPAQUE :
				TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT;
		} else {
			material_type = (alpha == 255) ? TILE_MATERIAL_LIQUID_OPAQUE :
				TILE_MATERIAL_LIQUID_TRANSPARENT;
		}
	}

	u32 tile_shader = shdsrc->getShader("nodes_shader", material_type, drawtype);

	u8 overlay_material = material_type;
	if (overlay_material == TILE_MATERIAL_OPAQUE)
		overlay_material = TILE_MATERIAL_BASIC;
	else if (overlay_material == TILE_MATERIAL_LIQUID_OPAQUE)
		overlay_material = TILE_MATERIAL_LIQUID_TRANSPARENT;

	u32 overlay_shader = shdsrc->getShader("nodes_shader", overlay_material, drawtype);

	// Tiles (fill in f->tiles[])
	for (u16 j = 0; j < 6; j++) {
		tiles[j].world_aligned = isWorldAligned(tdef[j].align_style,
				tsettings.world_aligned_mode, drawtype);
		fillTileAttribs(tsrc, &tiles[j].layers[0], tiles[j], tdef[j],
				color, material_type, tile_shader,
				tdef[j].backface_culling, tsettings);
		if (!tdef_overlay[j].name.empty())
			fillTileAttribs(tsrc, &tiles[j].layers[1], tiles[j], tdef_overlay[j],
					color, overlay_material, overlay_shader,
					tdef[j].backface_culling, tsettings);
	}

	u8 special_material = material_type;
	if (drawtype == NDT_PLANTLIKE_ROOTED) {
		if (waving == 1)
			special_material = TILE_MATERIAL_WAVING_PLANTS;
		else if (waving == 2)
			special_material = TILE_MATERIAL_WAVING_LEAVES;
	}
	u32 special_shader = shdsrc->getShader("nodes_shader", special_material, drawtype);

	// Special tiles (fill in f->special_tiles[])
	for (u16 j = 0; j < CF_SPECIAL_COUNT; j++)
		fillTileAttribs(tsrc, &special_tiles[j].layers[0], special_tiles[j], tdef_spec[j],
				color, special_material, special_shader,
				tdef_spec[j].backface_culling, tsettings);

	if (param_type_2 == CPT2_COLOR ||
			param_type_2 == CPT2_COLORED_FACEDIR ||
			param_type_2 == CPT2_COLORED_WALLMOUNTED)
		palette = tsrc->getPalette(palette_name);

	if (drawtype == NDT_MESH && !mesh.empty()) {
		// Meshnode drawtype
		// Read the mesh and apply scale
		mesh_ptr[0] = client->getMesh(mesh);
		if (mesh_ptr[0]){
			v3f scale = v3f(1.0, 1.0, 1.0) * BS * visual_scale;
			scaleMesh(mesh_ptr[0], scale);
			recalculateBoundingBox(mesh_ptr[0]);
			meshmanip->recalculateNormals(mesh_ptr[0], true, false);
		}
	}

	//Cache 6dfacedir and wallmounted rotated clones of meshes
	if (tsettings.enable_mesh_cache && mesh_ptr[0] &&
			(param_type_2 == CPT2_FACEDIR
			|| param_type_2 == CPT2_COLORED_FACEDIR)) {
		for (u16 j = 1; j < 24; j++) {
			mesh_ptr[j] = cloneMesh(mesh_ptr[0]);
			rotateMeshBy6dFacedir(mesh_ptr[j], j);
			recalculateBoundingBox(mesh_ptr[j]);
			meshmanip->recalculateNormals(mesh_ptr[j], true, false);
		}
	} else if (tsettings.enable_mesh_cache && mesh_ptr[0]
			&& (param_type_2 == CPT2_WALLMOUNTED ||
			param_type_2 == CPT2_COLORED_WALLMOUNTED)) {
		static const u8 wm_to_6d[6] = { 20, 0, 16 + 1, 12 + 3, 8, 4 + 2 };
		for (u16 j = 1; j < 6; j++) {
			mesh_ptr[j] = cloneMesh(mesh_ptr[0]);
			rotateMeshBy6dFacedir(mesh_ptr[j], wm_to_6d[j]);
			recalculateBoundingBox(mesh_ptr[j]);
			meshmanip->recalculateNormals(mesh_ptr[j], true, false);
		}
		rotateMeshBy6dFacedir(mesh_ptr[0], wm_to_6d[0]);
		recalculateBoundingBox(mesh_ptr[0]);
		meshmanip->recalculateNormals(mesh_ptr[0], true, false);
	}
}
#endif

/*
	NodeDefManager
*/




NodeDefManager::NodeDefManager()
{
	clear();
}


NodeDefManager::~NodeDefManager()
{
#ifndef SERVER
	for (ContentFeatures &f : m_content_features) {
		for (auto &j : f.mesh_ptr) {
			if (j)
				j->drop();
		}
	}
#endif
}


void NodeDefManager::clear()
{
	m_content_features.clear();
	m_name_id_mapping.clear();
	m_name_id_mapping_with_aliases.clear();
	m_group_to_items.clear();
	m_next_id = 0;
	m_selection_box_union.reset(0,0,0);
	m_selection_box_int_union.reset(0,0,0);

	resetNodeResolveState();

	u32 initial_length = 0;
	initial_length = MYMAX(initial_length, CONTENT_UNKNOWN + 1);
	initial_length = MYMAX(initial_length, CONTENT_AIR + 1);
	initial_length = MYMAX(initial_length, CONTENT_IGNORE + 1);
	m_content_features.resize(initial_length);

	// Set CONTENT_UNKNOWN
	{
		ContentFeatures f;
		f.name = "unknown";
		// Insert directly into containers
		content_t c = CONTENT_UNKNOWN;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}

	// Set CONTENT_AIR
	{
		ContentFeatures f;
		f.name                = "air";
		f.drawtype            = NDT_AIRLIKE;
		f.param_type          = CPT_LIGHT;
		f.light_propagates    = true;
		f.sunlight_propagates = true;
		f.walkable            = false;
		f.pointable           = false;
		f.diggable            = false;
		f.buildable_to        = true;
		f.floodable           = true;
		f.is_ground_content   = true;
		// Insert directly into containers
		content_t c = CONTENT_AIR;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}

	// Set CONTENT_IGNORE
	{
		ContentFeatures f;
		f.name                = "ignore";
		f.drawtype            = NDT_AIRLIKE;
		f.param_type          = CPT_NONE;
		f.light_propagates    = false;
		f.sunlight_propagates = false;
		f.walkable            = false;
		f.pointable           = false;
		f.diggable            = false;
		f.buildable_to        = true; // A way to remove accidental CONTENT_IGNOREs
		f.is_ground_content   = true;
		// Insert directly into containers
		content_t c = CONTENT_IGNORE;
		m_content_features[c] = f;
		addNameIdMapping(c, f.name);
	}
}


bool NodeDefManager::getId(const std::string &name, content_t &result) const
{
	std::unordered_map<std::string, content_t>::const_iterator
		i = m_name_id_mapping_with_aliases.find(name);
	if(i == m_name_id_mapping_with_aliases.end())
		return false;
	result = i->second;
	return true;
}


content_t NodeDefManager::getId(const std::string &name) const
{
	content_t id = CONTENT_IGNORE;
	getId(name, id);
	return id;
}


bool NodeDefManager::getIds(const std::string &name,
		std::vector<content_t> &result) const
{
	//TimeTaker t("getIds", NULL, PRECISION_MICRO);
	if (name.substr(0,6) != "group:") {
		content_t id = CONTENT_IGNORE;
		bool exists = getId(name, id);
		if (exists)
			result.push_back(id);
		return exists;
	}
	std::string group = name.substr(6);

	std::unordered_map<std::string, std::vector<content_t>>::const_iterator
		i = m_group_to_items.find(group);
	if (i == m_group_to_items.end())
		return true;

	const std::vector<content_t> &items = i->second;
	result.insert(result.end(), items.begin(), items.end());
	//printf("getIds: %dus\n", t.stop());
	return true;
}


const ContentFeatures& NodeDefManager::get(const std::string &name) const
{
	content_t id = CONTENT_UNKNOWN;
	getId(name, id);
	return get(id);
}


// returns CONTENT_IGNORE if no free ID found
content_t NodeDefManager::allocateId()
{
	for (content_t id = m_next_id;
			id >= m_next_id; // overflow?
			++id) {
		while (id >= m_content_features.size()) {
			m_content_features.emplace_back();
		}
		const ContentFeatures &f = m_content_features[id];
		if (f.name.empty()) {
			m_next_id = id + 1;
			return id;
		}
	}
	// If we arrive here, an overflow occurred in id.
	// That means no ID was found
	return CONTENT_IGNORE;
}


/*!
 * Returns the smallest box that contains all boxes
 * in the vector. Box_union is expanded.
 * @param[in]      boxes     the vector containing the boxes
 * @param[in, out] box_union the union of the arguments
 */
void boxVectorUnion(const std::vector<aabb3f> &boxes, aabb3f *box_union)
{
	for (const aabb3f &box : boxes) {
		box_union->addInternalBox(box);
	}
}


/*!
 * Returns a box that contains the nodebox in every case.
 * The argument node_union is expanded.
 * @param[in]      nodebox  the nodebox to be measured
 * @param[in]      features  used to decide whether the nodebox
 * can be rotated
 * @param[in, out] box_union the union of the arguments
 */
void getNodeBoxUnion(const NodeBox &nodebox, const ContentFeatures &features,
	aabb3f *box_union)
{
	switch(nodebox.type) {
		case NODEBOX_FIXED:
		case NODEBOX_LEVELED: {
			// Raw union
			aabb3f half_processed(0, 0, 0, 0, 0, 0);
			boxVectorUnion(nodebox.fixed, &half_processed);
			// Set leveled boxes to maximal
			if (nodebox.type == NODEBOX_LEVELED) {
				half_processed.MaxEdge.Y = +BS / 2;
			}
			if (features.param_type_2 == CPT2_FACEDIR ||
					features.param_type_2 == CPT2_COLORED_FACEDIR) {
				// Get maximal coordinate
				f32 coords[] = {
					fabsf(half_processed.MinEdge.X),
					fabsf(half_processed.MinEdge.Y),
					fabsf(half_processed.MinEdge.Z),
					fabsf(half_processed.MaxEdge.X),
					fabsf(half_processed.MaxEdge.Y),
					fabsf(half_processed.MaxEdge.Z) };
				f32 max = 0;
				for (float coord : coords) {
					if (max < coord) {
						max = coord;
					}
				}
				// Add the union of all possible rotated boxes
				box_union->addInternalPoint(-max, -max, -max);
				box_union->addInternalPoint(+max, +max, +max);
			} else {
				box_union->addInternalBox(half_processed);
			}
			break;
		}
		case NODEBOX_WALLMOUNTED: {
			// Add fix boxes
			box_union->addInternalBox(nodebox.wall_top);
			box_union->addInternalBox(nodebox.wall_bottom);
			// Find maximal coordinate in the X-Z plane
			f32 coords[] = {
				fabsf(nodebox.wall_side.MinEdge.X),
				fabsf(nodebox.wall_side.MinEdge.Z),
				fabsf(nodebox.wall_side.MaxEdge.X),
				fabsf(nodebox.wall_side.MaxEdge.Z) };
			f32 max = 0;
			for (float coord : coords) {
				if (max < coord) {
					max = coord;
				}
			}
			// Add the union of all possible rotated boxes
			box_union->addInternalPoint(-max, nodebox.wall_side.MinEdge.Y, -max);
			box_union->addInternalPoint(max, nodebox.wall_side.MaxEdge.Y, max);
			break;
		}
		case NODEBOX_CONNECTED: {
			// Add all possible connected boxes
			boxVectorUnion(nodebox.fixed,               box_union);
			boxVectorUnion(nodebox.connect_top,         box_union);
			boxVectorUnion(nodebox.connect_bottom,      box_union);
			boxVectorUnion(nodebox.connect_front,       box_union);
			boxVectorUnion(nodebox.connect_left,        box_union);
			boxVectorUnion(nodebox.connect_back,        box_union);
			boxVectorUnion(nodebox.connect_right,       box_union);
			boxVectorUnion(nodebox.disconnected_top,    box_union);
			boxVectorUnion(nodebox.disconnected_bottom, box_union);
			boxVectorUnion(nodebox.disconnected_front,  box_union);
			boxVectorUnion(nodebox.disconnected_left,   box_union);
			boxVectorUnion(nodebox.disconnected_back,   box_union);
			boxVectorUnion(nodebox.disconnected_right,  box_union);
			boxVectorUnion(nodebox.disconnected,        box_union);
			boxVectorUnion(nodebox.disconnected_sides,  box_union);
			break;
		}
		default: {
			// NODEBOX_REGULAR
			box_union->addInternalPoint(-BS / 2, -BS / 2, -BS / 2);
			box_union->addInternalPoint(+BS / 2, +BS / 2, +BS / 2);
		}
	}
}


inline void NodeDefManager::fixSelectionBoxIntUnion()
{
	m_selection_box_int_union.MinEdge.X = floorf(
		m_selection_box_union.MinEdge.X / BS + 0.5f);
	m_selection_box_int_union.MinEdge.Y = floorf(
		m_selection_box_union.MinEdge.Y / BS + 0.5f);
	m_selection_box_int_union.MinEdge.Z = floorf(
		m_selection_box_union.MinEdge.Z / BS + 0.5f);
	m_selection_box_int_union.MaxEdge.X = ceilf(
		m_selection_box_union.MaxEdge.X / BS - 0.5f);
	m_selection_box_int_union.MaxEdge.Y = ceilf(
		m_selection_box_union.MaxEdge.Y / BS - 0.5f);
	m_selection_box_int_union.MaxEdge.Z = ceilf(
		m_selection_box_union.MaxEdge.Z / BS - 0.5f);
}


void NodeDefManager::eraseIdFromGroups(content_t id)
{
	// For all groups in m_group_to_items...
	for (auto iter_groups = m_group_to_items.begin();
			iter_groups != m_group_to_items.end();) {
		// Get the group items vector.
		std::vector<content_t> &items = iter_groups->second;

		// Remove any occurence of the id in the group items vector.
		items.erase(std::remove(items.begin(), items.end(), id), items.end());

		// If group is empty, erase its vector from the map.
		if (items.empty())
			iter_groups = m_group_to_items.erase(iter_groups);
		else
			++iter_groups;
	}
}


// IWritableNodeDefManager
content_t NodeDefManager::set(const std::string &name, const ContentFeatures &def)
{
	// Pre-conditions
	assert(name != "");
	assert(name != "ignore");
	assert(name == def.name);

	content_t id = CONTENT_IGNORE;
	if (!m_name_id_mapping.getId(name, id)) { // ignore aliases
		// Get new id
		id = allocateId();
		if (id == CONTENT_IGNORE) {
			warningstream << "NodeDefManager: Absolute "
				"limit reached" << std::endl;
			return CONTENT_IGNORE;
		}
		assert(id != CONTENT_IGNORE);
		addNameIdMapping(id, name);
	}

	// If there is already ContentFeatures registered for this id, clear old groups
	if (id < m_content_features.size())
		eraseIdFromGroups(id);

	m_content_features[id] = def;
	verbosestream << "NodeDefManager: registering content id \"" << id
		<< "\": name=\"" << def.name << "\""<<std::endl;

	getNodeBoxUnion(def.selection_box, def, &m_selection_box_union);
	fixSelectionBoxIntUnion();

	// Add this content to the list of all groups it belongs to
	for (const auto &group : def.groups) {
		const std::string &group_name = group.first;
		m_group_to_items[group_name].push_back(id);
	}

	return id;
}


content_t NodeDefManager::allocateDummy(const std::string &name)
{
	assert(name != "");	// Pre-condition
	ContentFeatures f;
	f.name = name;
	return set(name, f);
}


void NodeDefManager::removeNode(const std::string &name)
{
	// Pre-condition
	assert(name != "");

	// Erase name from name ID mapping
	content_t id = CONTENT_IGNORE;
	if (m_name_id_mapping.getId(name, id)) {
		m_name_id_mapping.eraseName(name);
		m_name_id_mapping_with_aliases.erase(name);
	}

	eraseIdFromGroups(id);
}


void NodeDefManager::updateAliases(IItemDefManager *idef)
{
	std::set<std::string> all;
	idef->getAll(all);
	m_name_id_mapping_with_aliases.clear();
	for (const std::string &name : all) {
		const std::string &convert_to = idef->getAlias(name);
		content_t id;
		if (m_name_id_mapping.getId(convert_to, id)) {
			m_name_id_mapping_with_aliases.insert(
				std::make_pair(name, id));
		}
	}
}

void NodeDefManager::applyTextureOverrides(const std::vector<TextureOverride> &overrides)
{
	infostream << "NodeDefManager::applyTextureOverrides(): Applying "
		"overrides to textures" << std::endl;

	for (const TextureOverride& texture_override : overrides) {
		content_t id;
		if (!getId(texture_override.id, id))
			continue; // Ignore unknown node

		ContentFeatures &nodedef = m_content_features[id];

		// Override tiles
		if (texture_override.hasTarget(OverrideTarget::TOP))
			nodedef.tiledef[0].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::BOTTOM))
			nodedef.tiledef[1].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::RIGHT))
			nodedef.tiledef[2].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::LEFT))
			nodedef.tiledef[3].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::BACK))
			nodedef.tiledef[4].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::FRONT))
			nodedef.tiledef[5].name = texture_override.texture;


		// Override special tiles, if applicable
		if (texture_override.hasTarget(OverrideTarget::SPECIAL_1))
			nodedef.tiledef_special[0].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::SPECIAL_2))
			nodedef.tiledef_special[1].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::SPECIAL_3))
			nodedef.tiledef_special[2].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::SPECIAL_4))
			nodedef.tiledef_special[3].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::SPECIAL_5))
			nodedef.tiledef_special[4].name = texture_override.texture;

		if (texture_override.hasTarget(OverrideTarget::SPECIAL_6))
			nodedef.tiledef_special[5].name = texture_override.texture;
	}
}

void NodeDefManager::updateTextures(IGameDef *gamedef,
	void (*progress_callback)(void *progress_args, u32 progress, u32 max_progress),
	void *progress_callback_args)
{
#ifndef SERVER
	infostream << "NodeDefManager::updateTextures(): Updating "
		"textures in node definitions" << std::endl;

	Client *client = (Client *)gamedef;
	ITextureSource *tsrc = client->tsrc();
	IShaderSource *shdsrc = client->getShaderSource();
	scene::IMeshManipulator *meshmanip =
		RenderingEngine::get_scene_manager()->getMeshManipulator();
	TextureSettings tsettings;
	tsettings.readSettings();

	u32 size = m_content_features.size();

	for (u32 i = 0; i < size; i++) {
		ContentFeatures *f = &(m_content_features[i]);
		f->updateTextures(tsrc, shdsrc, meshmanip, client, tsettings);
		progress_callback(progress_callback_args, i, size);
	}
#endif
}

void NodeDefManager::serialize(std::ostream &os, u16 protocol_version) const
{
	writeU8(os, 1); // version
	u16 count = 0;
	std::ostringstream os2(std::ios::binary);
	for (u32 i = 0; i < m_content_features.size(); i++) {
		if (i == CONTENT_IGNORE || i == CONTENT_AIR
				|| i == CONTENT_UNKNOWN)
			continue;
		const ContentFeatures *f = &m_content_features[i];
		if (f->name.empty())
			continue;
		writeU16(os2, i);
		// Wrap it in a string to allow different lengths without
		// strict version incompatibilities
		std::ostringstream wrapper_os(std::ios::binary);
		f->serialize(wrapper_os, protocol_version);
		os2<<serializeString(wrapper_os.str());

		// must not overflow
		u16 next = count + 1;
		FATAL_ERROR_IF(next < count, "Overflow");
		count++;
	}
	writeU16(os, count);
	os << serializeLongString(os2.str());
}


void NodeDefManager::deSerialize(std::istream &is)
{
	clear();
	int version = readU8(is);
	if (version != 1)
		throw SerializationError("unsupported NodeDefinitionManager version");
	u16 count = readU16(is);
	std::istringstream is2(deSerializeLongString(is), std::ios::binary);
	ContentFeatures f;
	for (u16 n = 0; n < count; n++) {
		u16 i = readU16(is2);

		// Read it from the string wrapper
		std::string wrapper = deSerializeString(is2);
		std::istringstream wrapper_is(wrapper, std::ios::binary);
		f.deSerialize(wrapper_is);

		// Check error conditions
		if (i == CONTENT_IGNORE || i == CONTENT_AIR || i == CONTENT_UNKNOWN) {
			warningstream << "NodeDefManager::deSerialize(): "
				"not changing builtin node " << i << std::endl;
			continue;
		}
		if (f.name.empty()) {
			warningstream << "NodeDefManager::deSerialize(): "
				"received empty name" << std::endl;
			continue;
		}

		// Ignore aliases
		u16 existing_id;
		if (m_name_id_mapping.getId(f.name, existing_id) && i != existing_id) {
			warningstream << "NodeDefManager::deSerialize(): "
				"already defined with different ID: " << f.name << std::endl;
			continue;
		}

		// All is ok, add node definition with the requested ID
		if (i >= m_content_features.size())
			m_content_features.resize((u32)(i) + 1);
		m_content_features[i] = f;
		addNameIdMapping(i, f.name);
		TRACESTREAM(<< "NodeDef: deserialized " << f.name << std::endl);

		getNodeBoxUnion(f.selection_box, f, &m_selection_box_union);
		fixSelectionBoxIntUnion();
	}

	// Since liquid_alternative_flowing_id and liquid_alternative_source_id
	// are not sent, resolve them client-side too.
	resolveCrossrefs();
}


void NodeDefManager::addNameIdMapping(content_t i, std::string name)
{
	m_name_id_mapping.set(i, name);
	m_name_id_mapping_with_aliases.insert(std::make_pair(name, i));
}


NodeDefManager *createNodeDefManager()
{
	return new NodeDefManager();
}


void NodeDefManager::pendNodeResolve(NodeResolver *nr) const
{
	nr->m_ndef = this;
	if (m_node_registration_complete)
		nr->nodeResolveInternal();
	else
		m_pending_resolve_callbacks.push_back(nr);
}


bool NodeDefManager::cancelNodeResolveCallback(NodeResolver *nr) const
{
	size_t len = m_pending_resolve_callbacks.size();
	for (size_t i = 0; i != len; i++) {
		if (nr != m_pending_resolve_callbacks[i])
			continue;

		len--;
		m_pending_resolve_callbacks[i] = m_pending_resolve_callbacks[len];
		m_pending_resolve_callbacks.resize(len);
		return true;
	}

	return false;
}


void NodeDefManager::runNodeResolveCallbacks()
{
	for (size_t i = 0; i != m_pending_resolve_callbacks.size(); i++) {
		NodeResolver *nr = m_pending_resolve_callbacks[i];
		nr->nodeResolveInternal();
	}

	m_pending_resolve_callbacks.clear();
}


void NodeDefManager::resetNodeResolveState()
{
	m_node_registration_complete = false;
	m_pending_resolve_callbacks.clear();
}

static void removeDupes(std::vector<content_t> &list)
{
	std::sort(list.begin(), list.end());
	auto new_end = std::unique(list.begin(), list.end());
	list.erase(new_end, list.end());
}

void NodeDefManager::resolveCrossrefs()
{
	for (ContentFeatures &f : m_content_features) {
		if (f.liquid_type != LIQUID_NONE) {
			f.liquid_alternative_flowing_id = getId(f.liquid_alternative_flowing);
			f.liquid_alternative_source_id = getId(f.liquid_alternative_source);
			continue;
		}
		if (f.drawtype != NDT_NODEBOX || f.node_box.type != NODEBOX_CONNECTED)
			continue;

		for (const std::string &name : f.connects_to) {
			getIds(name, f.connects_to_ids);
		}
		removeDupes(f.connects_to_ids);
	}
}

bool NodeDefManager::nodeboxConnects(MapNode from, MapNode to,
	u8 connect_face) const
{
	const ContentFeatures &f1 = get(from);

	if ((f1.drawtype != NDT_NODEBOX) || (f1.node_box.type != NODEBOX_CONNECTED))
		return false;

	// lookup target in connected set
	if (!CONTAINS(f1.connects_to_ids, to.param0))
		return false;

	const ContentFeatures &f2 = get(to);

	if ((f2.drawtype == NDT_NODEBOX) && (f2.node_box.type == NODEBOX_CONNECTED))
		// ignores actually looking if back connection exists
		return CONTAINS(f2.connects_to_ids, from.param0);

	// does to node declare usable faces?
	if (f2.connect_sides > 0) {
		if ((f2.param_type_2 == CPT2_FACEDIR ||
				f2.param_type_2 == CPT2_COLORED_FACEDIR)
				&& (connect_face >= 4)) {
			static const u8 rot[33 * 4] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 4, 32, 16, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, // 4 - back
				8, 4, 32, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, // 8 - right
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 8, 4, 32, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, // 16 - front
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 32, 16, 8, 4 // 32 - left
				};
			return (f2.connect_sides
				& rot[(connect_face * 4) + (to.param2 & 0x1F)]);
		}
		return (f2.connect_sides & connect_face);
	}
	// the target is just a regular node, so connect no matter back connection
	return true;
}

////
//// NodeResolver
////

NodeResolver::NodeResolver()
{
	m_nodenames.reserve(16);
	m_nnlistsizes.reserve(4);
}


NodeResolver::~NodeResolver()
{
	if (!m_resolve_done && m_ndef)
		m_ndef->cancelNodeResolveCallback(this);
}


void NodeResolver::cloneTo(NodeResolver *res) const
{
	FATAL_ERROR_IF(!m_resolve_done, "NodeResolver can only be cloned"
		" after resolving has completed");
	/* We don't actually do anything significant. Since the node resolving has
	 * already completed, the class that called us will already have the
	 * resolved IDs in its data structures (which it copies on its own) */
	res->m_ndef = m_ndef;
	res->m_resolve_done = true;
}


void NodeResolver::nodeResolveInternal()
{
	m_nodenames_idx   = 0;
	m_nnlistsizes_idx = 0;

	resolveNodeNames();
	m_resolve_done = true;

	m_nodenames.clear();
	m_nnlistsizes.clear();
}


bool NodeResolver::getIdFromNrBacklog(content_t *result_out,
	const std::string &node_alt, content_t c_fallback, bool error_on_fallback)
{
	if (m_nodenames_idx == m_nodenames.size()) {
		*result_out = c_fallback;
		errorstream << "NodeResolver: no more nodes in list" << std::endl;
		return false;
	}

	content_t c;
	std::string name = m_nodenames[m_nodenames_idx++];

	bool success = m_ndef->getId(name, c);
	if (!success && !node_alt.empty()) {
		name = node_alt;
		success = m_ndef->getId(name, c);
	}

	if (!success) {
		if (error_on_fallback)
			errorstream << "NodeResolver: failed to resolve node name '" << name
				<< "'." << std::endl;
		c = c_fallback;
	}

	*result_out = c;
	return success;
}


bool NodeResolver::getIdsFromNrBacklog(std::vector<content_t> *result_out,
	bool all_required, content_t c_fallback)
{
	bool success = true;

	if (m_nnlistsizes_idx == m_nnlistsizes.size()) {
		errorstream << "NodeResolver: no more node lists" << std::endl;
		return false;
	}

	size_t length = m_nnlistsizes[m_nnlistsizes_idx++];

	while (length--) {
		if (m_nodenames_idx == m_nodenames.size()) {
			errorstream << "NodeResolver: no more nodes in list" << std::endl;
			return false;
		}

		content_t c;
		std::string &name = m_nodenames[m_nodenames_idx++];

		if (name.substr(0,6) != "group:") {
			if (m_ndef->getId(name, c)) {
				result_out->push_back(c);
			} else if (all_required) {
				errorstream << "NodeResolver: failed to resolve node name '"
					<< name << "'." << std::endl;
				result_out->push_back(c_fallback);
				success = false;
			}
		} else {
			m_ndef->getIds(name, *result_out);
		}
	}

	return success;
}
