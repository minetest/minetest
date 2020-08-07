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
#include "common/c_content.h"
#include "common/c_converter.h"
#include "common/c_types.h"
#include "nodedef.h"
#include "object_properties.h"
#include "collision.h"
#include "cpp_api/s_node.h"
#include "lua_api/l_object.h"
#include "lua_api/l_item.h"
#include "common/c_internal.h"
#include "server.h"
#include "log.h"
#include "tool.h"
#include "porting.h"
#include "mapgen/mg_schematic.h"
#include "noise.h"
#include "server/player_sao.h"
#include "util/pointedthing.h"
#include "debug.h" // For FATAL_ERROR
#include <json/json.h>

struct EnumString es_TileAnimationType[] =
{
	{TAT_NONE, "none"},
	{TAT_VERTICAL_FRAMES, "vertical_frames"},
	{TAT_SHEET_2D, "sheet_2d"},
	{0, NULL},
};

/******************************************************************************/
void read_item_definition(lua_State* L, int index,
		const ItemDefinition &default_def, ItemDefinition &def)
{
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	def.type = (ItemType)getenumfield(L, index, "type",
			es_ItemType, ITEM_NONE);
	getstringfield(L, index, "name", def.name);
	getstringfield(L, index, "description", def.description);
	getstringfield(L, index, "inventory_image", def.inventory_image);
	getstringfield(L, index, "inventory_overlay", def.inventory_overlay);
	getstringfield(L, index, "wield_image", def.wield_image);
	getstringfield(L, index, "wield_overlay", def.wield_overlay);
	getstringfield(L, index, "palette", def.palette_image);

	// Read item color.
	lua_getfield(L, index, "color");
	read_color(L, -1, &def.color);
	lua_pop(L, 1);

	lua_getfield(L, index, "wield_scale");
	if(lua_istable(L, -1)){
		def.wield_scale = check_v3f(L, -1);
	}
	lua_pop(L, 1);

	int stack_max = getintfield_default(L, index, "stack_max", def.stack_max);
	def.stack_max = rangelim(stack_max, 1, U16_MAX);

	lua_getfield(L, index, "on_use");
	def.usable = lua_isfunction(L, -1);
	lua_pop(L, 1);

	getboolfield(L, index, "liquids_pointable", def.liquids_pointable);

	warn_if_field_exists(L, index, "tool_digging_properties",
			"Obsolete; use tool_capabilities");

	lua_getfield(L, index, "tool_capabilities");
	if(lua_istable(L, -1)){
		def.tool_capabilities = new ToolCapabilities(
				read_tool_capabilities(L, -1));
	}

	// If name is "" (hand), ensure there are ToolCapabilities
	// because it will be looked up there whenever any other item has
	// no ToolCapabilities
	if (def.name.empty() && def.tool_capabilities == NULL){
		def.tool_capabilities = new ToolCapabilities();
	}

	lua_getfield(L, index, "groups");
	read_groups(L, -1, def.groups);
	lua_pop(L, 1);

	lua_getfield(L, index, "sounds");
	if (!lua_isnil(L, -1)) {
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "place");
		read_soundspec(L, -1, def.sound_place);
		lua_pop(L, 1);
		lua_getfield(L, -1, "place_failed");
		read_soundspec(L, -1, def.sound_place_failed);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	def.range = getfloatfield_default(L, index, "range", def.range);

	// Client shall immediately place this node when player places the item.
	// Server will update the precise end result a moment later.
	// "" = no prediction
	getstringfield(L, index, "node_placement_prediction",
			def.node_placement_prediction);
}

/******************************************************************************/
void push_item_definition(lua_State *L, const ItemDefinition &i)
{
	lua_newtable(L);
	lua_pushstring(L, i.name.c_str());
	lua_setfield(L, -2, "name");
	lua_pushstring(L, i.description.c_str());
	lua_setfield(L, -2, "description");
}

void push_item_definition_full(lua_State *L, const ItemDefinition &i)
{
	std::string type(es_ItemType[(int)i.type].str);

	lua_newtable(L);
	lua_pushstring(L, i.name.c_str());
	lua_setfield(L, -2, "name");
	lua_pushstring(L, i.description.c_str());
	lua_setfield(L, -2, "description");
	lua_pushstring(L, type.c_str());
	lua_setfield(L, -2, "type");
	lua_pushstring(L, i.inventory_image.c_str());
	lua_setfield(L, -2, "inventory_image");
	lua_pushstring(L, i.inventory_overlay.c_str());
	lua_setfield(L, -2, "inventory_overlay");
	lua_pushstring(L, i.wield_image.c_str());
	lua_setfield(L, -2, "wield_image");
	lua_pushstring(L, i.wield_overlay.c_str());
	lua_setfield(L, -2, "wield_overlay");
	lua_pushstring(L, i.palette_image.c_str());
	lua_setfield(L, -2, "palette_image");
	push_ARGB8(L, i.color);
	lua_setfield(L, -2, "color");
	push_v3f(L, i.wield_scale);
	lua_setfield(L, -2, "wield_scale");
	lua_pushinteger(L, i.stack_max);
	lua_setfield(L, -2, "stack_max");
	lua_pushboolean(L, i.usable);
	lua_setfield(L, -2, "usable");
	lua_pushboolean(L, i.liquids_pointable);
	lua_setfield(L, -2, "liquids_pointable");
	if (i.tool_capabilities) {
		push_tool_capabilities(L, *i.tool_capabilities);
		lua_setfield(L, -2, "tool_capabilities");
	}
	push_groups(L, i.groups);
	lua_setfield(L, -2, "groups");
	push_soundspec(L, i.sound_place);
	lua_setfield(L, -2, "sound_place");
	push_soundspec(L, i.sound_place_failed);
	lua_setfield(L, -2, "sound_place_failed");
	lua_pushstring(L, i.node_placement_prediction.c_str());
	lua_setfield(L, -2, "node_placement_prediction");
}

/******************************************************************************/
void read_object_properties(lua_State *L, int index,
		ServerActiveObject *sao, ObjectProperties *prop, IItemDefManager *idef)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	if (lua_isnil(L, index))
		return;

	luaL_checktype(L, -1, LUA_TTABLE);

	int hp_max = 0;
	if (getintfield(L, -1, "hp_max", hp_max)) {
		prop->hp_max = (u16)rangelim(hp_max, 0, U16_MAX);

		if (prop->hp_max < sao->getHP()) {
			PlayerHPChangeReason reason(PlayerHPChangeReason::SET_HP);
			sao->setHP(prop->hp_max, reason);
			if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER)
				sao->getEnv()->getGameDef()->SendPlayerHPOrDie((PlayerSAO *)sao, reason);
		}
	}

	if (getintfield(L, -1, "breath_max", prop->breath_max)) {
		if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			PlayerSAO *player = (PlayerSAO *)sao;
			if (prop->breath_max < player->getBreath())
				player->setBreath(prop->breath_max);
		}
	}
	getboolfield(L, -1, "physical", prop->physical);
	getboolfield(L, -1, "collide_with_objects", prop->collideWithObjects);

	lua_getfield(L, -1, "collisionbox");
	bool collisionbox_defined = lua_istable(L, -1);
	if (collisionbox_defined)
		prop->collisionbox = read_aabb3f(L, -1, 1.0);
	lua_pop(L, 1);

	lua_getfield(L, -1, "selectionbox");
	if (lua_istable(L, -1))
		prop->selectionbox = read_aabb3f(L, -1, 1.0);
	else if (collisionbox_defined)
		prop->selectionbox = prop->collisionbox;
	lua_pop(L, 1);

	getboolfield(L, -1, "pointable", prop->pointable);
	getstringfield(L, -1, "visual", prop->visual);

	getstringfield(L, -1, "mesh", prop->mesh);

	lua_getfield(L, -1, "visual_size");
	if (lua_istable(L, -1)) {
		// Backwards compatibility: Also accept { x = ?, y = ? }
		v2f scale_xy = read_v2f(L, -1);

		f32 scale_z = scale_xy.X;
		lua_getfield(L, -1, "z");
		if (lua_isnumber(L, -1))
			scale_z = lua_tonumber(L, -1);
		lua_pop(L, 1);

		prop->visual_size = v3f(scale_xy.X, scale_xy.Y, scale_z);
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "textures");
	if(lua_istable(L, -1)){
		prop->textures.clear();
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			if(lua_isstring(L, -1))
				prop->textures.emplace_back(lua_tostring(L, -1));
			else
				prop->textures.emplace_back("");
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "colors");
	if (lua_istable(L, -1)) {
		int table = lua_gettop(L);
		prop->colors.clear();
		for (lua_pushnil(L); lua_next(L, table); lua_pop(L, 1)) {
			video::SColor color(255, 255, 255, 255);
			read_color(L, -1, &color);
			prop->colors.push_back(color);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "spritediv");
	if(lua_istable(L, -1))
		prop->spritediv = read_v2s16(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "initial_sprite_basepos");
	if(lua_istable(L, -1))
		prop->initial_sprite_basepos = read_v2s16(L, -1);
	lua_pop(L, 1);

	getboolfield(L, -1, "is_visible", prop->is_visible);
	getboolfield(L, -1, "makes_footstep_sound", prop->makes_footstep_sound);
	if (getfloatfield(L, -1, "stepheight", prop->stepheight))
		prop->stepheight *= BS;
	getfloatfield(L, -1, "eye_height", prop->eye_height);

	getfloatfield(L, -1, "automatic_rotate", prop->automatic_rotate);
	lua_getfield(L, -1, "automatic_face_movement_dir");
	if (lua_isnumber(L, -1)) {
		prop->automatic_face_movement_dir = true;
		prop->automatic_face_movement_dir_offset = luaL_checknumber(L, -1);
	} else if (lua_isboolean(L, -1)) {
		prop->automatic_face_movement_dir = lua_toboolean(L, -1);
		prop->automatic_face_movement_dir_offset = 0.0;
	}
	lua_pop(L, 1);
	getboolfield(L, -1, "backface_culling", prop->backface_culling);
	getintfield(L, -1, "glow", prop->glow);

	getstringfield(L, -1, "nametag", prop->nametag);
	lua_getfield(L, -1, "nametag_color");
	if (!lua_isnil(L, -1)) {
		video::SColor color = prop->nametag_color;
		if (read_color(L, -1, &color))
			prop->nametag_color = color;
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "automatic_face_movement_max_rotation_per_sec");
	if (lua_isnumber(L, -1)) {
		prop->automatic_face_movement_max_rotation_per_sec = luaL_checknumber(L, -1);
	}
	lua_pop(L, 1);

	getstringfield(L, -1, "infotext", prop->infotext);
	getboolfield(L, -1, "static_save", prop->static_save);

	lua_getfield(L, -1, "wield_item");
	if (!lua_isnil(L, -1))
		prop->wield_item = read_item(L, -1, idef).getItemString();
	lua_pop(L, 1);

	getfloatfield(L, -1, "zoom_fov", prop->zoom_fov);
	getboolfield(L, -1, "use_texture_alpha", prop->use_texture_alpha);
	getboolfield(L, -1, "shaded", prop->shaded);

	getstringfield(L, -1, "damage_texture_modifier", prop->damage_texture_modifier);
}

/******************************************************************************/
void push_object_properties(lua_State *L, ObjectProperties *prop)
{
	lua_newtable(L);
	lua_pushnumber(L, prop->hp_max);
	lua_setfield(L, -2, "hp_max");
	lua_pushnumber(L, prop->breath_max);
	lua_setfield(L, -2, "breath_max");
	lua_pushboolean(L, prop->physical);
	lua_setfield(L, -2, "physical");
	lua_pushboolean(L, prop->collideWithObjects);
	lua_setfield(L, -2, "collide_with_objects");
	push_aabb3f(L, prop->collisionbox);
	lua_setfield(L, -2, "collisionbox");
	push_aabb3f(L, prop->selectionbox);
	lua_setfield(L, -2, "selectionbox");
	lua_pushboolean(L, prop->pointable);
	lua_setfield(L, -2, "pointable");
	lua_pushlstring(L, prop->visual.c_str(), prop->visual.size());
	lua_setfield(L, -2, "visual");
	lua_pushlstring(L, prop->mesh.c_str(), prop->mesh.size());
	lua_setfield(L, -2, "mesh");
	push_v3f(L, prop->visual_size);
	lua_setfield(L, -2, "visual_size");

	lua_createtable(L, prop->textures.size(), 0);
	u16 i = 1;
	for (const std::string &texture : prop->textures) {
		lua_pushlstring(L, texture.c_str(), texture.size());
		lua_rawseti(L, -2, i++);
	}
	lua_setfield(L, -2, "textures");

	lua_createtable(L, prop->colors.size(), 0);
	i = 1;
	for (const video::SColor &color : prop->colors) {
		push_ARGB8(L, color);
		lua_rawseti(L, -2, i++);
	}
	lua_setfield(L, -2, "colors");

	push_v2s16(L, prop->spritediv);
	lua_setfield(L, -2, "spritediv");
	push_v2s16(L, prop->initial_sprite_basepos);
	lua_setfield(L, -2, "initial_sprite_basepos");
	lua_pushboolean(L, prop->is_visible);
	lua_setfield(L, -2, "is_visible");
	lua_pushboolean(L, prop->makes_footstep_sound);
	lua_setfield(L, -2, "makes_footstep_sound");
	lua_pushnumber(L, prop->stepheight / BS);
	lua_setfield(L, -2, "stepheight");
	lua_pushnumber(L, prop->eye_height);
	lua_setfield(L, -2, "eye_height");
	lua_pushnumber(L, prop->automatic_rotate);
	lua_setfield(L, -2, "automatic_rotate");
	if (prop->automatic_face_movement_dir)
		lua_pushnumber(L, prop->automatic_face_movement_dir_offset);
	else
		lua_pushboolean(L, false);
	lua_setfield(L, -2, "automatic_face_movement_dir");
	lua_pushboolean(L, prop->backface_culling);
	lua_setfield(L, -2, "backface_culling");
	lua_pushnumber(L, prop->glow);
	lua_setfield(L, -2, "glow");
	lua_pushlstring(L, prop->nametag.c_str(), prop->nametag.size());
	lua_setfield(L, -2, "nametag");
	push_ARGB8(L, prop->nametag_color);
	lua_setfield(L, -2, "nametag_color");
	lua_pushnumber(L, prop->automatic_face_movement_max_rotation_per_sec);
	lua_setfield(L, -2, "automatic_face_movement_max_rotation_per_sec");
	lua_pushlstring(L, prop->infotext.c_str(), prop->infotext.size());
	lua_setfield(L, -2, "infotext");
	lua_pushboolean(L, prop->static_save);
	lua_setfield(L, -2, "static_save");
	lua_pushlstring(L, prop->wield_item.c_str(), prop->wield_item.size());
	lua_setfield(L, -2, "wield_item");
	lua_pushnumber(L, prop->zoom_fov);
	lua_setfield(L, -2, "zoom_fov");
	lua_pushboolean(L, prop->use_texture_alpha);
	lua_setfield(L, -2, "use_texture_alpha");
	lua_pushboolean(L, prop->shaded);
	lua_setfield(L, -2, "shaded");
	lua_pushlstring(L, prop->damage_texture_modifier.c_str(), prop->damage_texture_modifier.size());
	lua_setfield(L, -2, "damage_texture_modifier");
}

/******************************************************************************/
TileDef read_tiledef(lua_State *L, int index, u8 drawtype)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	TileDef tiledef;

	bool default_tiling = true;
	bool default_culling = true;
	switch (drawtype) {
		case NDT_PLANTLIKE:
		case NDT_PLANTLIKE_ROOTED:
		case NDT_FIRELIKE:
			default_tiling = false;
			// "break" is omitted here intentionaly, as PLANTLIKE
			// FIRELIKE drawtype both should default to having
			// backface_culling to false.
		case NDT_MESH:
		case NDT_LIQUID:
			default_culling = false;
			break;
		default:
			break;
	}

	// key at index -2 and value at index
	if(lua_isstring(L, index)){
		// "default_lava.png"
		tiledef.name = lua_tostring(L, index);
		tiledef.tileable_vertical = default_tiling;
		tiledef.tileable_horizontal = default_tiling;
		tiledef.backface_culling = default_culling;
	}
	else if(lua_istable(L, index))
	{
		// name="default_lava.png"
		tiledef.name = "";
		getstringfield(L, index, "name", tiledef.name);
		getstringfield(L, index, "image", tiledef.name); // MaterialSpec compat.
		tiledef.backface_culling = getboolfield_default(
			L, index, "backface_culling", default_culling);
		tiledef.tileable_horizontal = getboolfield_default(
			L, index, "tileable_horizontal", default_tiling);
		tiledef.tileable_vertical = getboolfield_default(
			L, index, "tileable_vertical", default_tiling);
		std::string align_style;
		if (getstringfield(L, index, "align_style", align_style)) {
			if (align_style == "user")
				tiledef.align_style = ALIGN_STYLE_USER_DEFINED;
			else if (align_style == "world")
				tiledef.align_style = ALIGN_STYLE_WORLD;
			else
				tiledef.align_style = ALIGN_STYLE_NODE;
		}
		tiledef.scale = getintfield_default(L, index, "scale", 0);
		// color = ...
		lua_getfield(L, index, "color");
		tiledef.has_color = read_color(L, -1, &tiledef.color);
		lua_pop(L, 1);
		// animation = {}
		lua_getfield(L, index, "animation");
		tiledef.animation = read_animation_definition(L, -1);
		lua_pop(L, 1);
	}

	return tiledef;
}

/******************************************************************************/
ContentFeatures read_content_features(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	ContentFeatures f;

	/* Cache existence of some callbacks */
	lua_getfield(L, index, "on_construct");
	if(!lua_isnil(L, -1)) f.has_on_construct = true;
	lua_pop(L, 1);
	lua_getfield(L, index, "on_destruct");
	if(!lua_isnil(L, -1)) f.has_on_destruct = true;
	lua_pop(L, 1);
	lua_getfield(L, index, "after_destruct");
	if(!lua_isnil(L, -1)) f.has_after_destruct = true;
	lua_pop(L, 1);

	lua_getfield(L, index, "on_rightclick");
	f.rightclickable = lua_isfunction(L, -1);
	lua_pop(L, 1);

	/* Name */
	getstringfield(L, index, "name", f.name);

	/* Groups */
	lua_getfield(L, index, "groups");
	read_groups(L, -1, f.groups);
	lua_pop(L, 1);

	/* Visual definition */

	f.drawtype = (NodeDrawType)getenumfield(L, index, "drawtype",
			ScriptApiNode::es_DrawType,NDT_NORMAL);
	getfloatfield(L, index, "visual_scale", f.visual_scale);

	/* Meshnode model filename */
	getstringfield(L, index, "mesh", f.mesh);

	// tiles = {}
	lua_getfield(L, index, "tiles");
	// If nil, try the deprecated name "tile_images" instead
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		warn_if_field_exists(L, index, "tile_images",
				"Deprecated; new name is \"tiles\".");
		lua_getfield(L, index, "tile_images");
	}
	if(lua_istable(L, -1)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while(lua_next(L, table) != 0){
			// Read tiledef from value
			f.tiledef[i] = read_tiledef(L, -1, f.drawtype);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			i++;
			if(i==6){
				lua_pop(L, 1);
				break;
			}
		}
		// Copy last value to all remaining textures
		if(i >= 1){
			TileDef lasttile = f.tiledef[i-1];
			while(i < 6){
				f.tiledef[i] = lasttile;
				i++;
			}
		}
	}
	lua_pop(L, 1);

	// overlay_tiles = {}
	lua_getfield(L, index, "overlay_tiles");
	if (lua_istable(L, -1)) {
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while (lua_next(L, table) != 0) {
			// Read tiledef from value
			f.tiledef_overlay[i] = read_tiledef(L, -1, f.drawtype);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			i++;
			if (i == 6) {
				lua_pop(L, 1);
				break;
			}
		}
		// Copy last value to all remaining textures
		if (i >= 1) {
			TileDef lasttile = f.tiledef_overlay[i - 1];
			while (i < 6) {
				f.tiledef_overlay[i] = lasttile;
				i++;
			}
		}
	}
	lua_pop(L, 1);

	// special_tiles = {}
	lua_getfield(L, index, "special_tiles");
	// If nil, try the deprecated name "special_materials" instead
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		warn_if_field_exists(L, index, "special_materials",
				"Deprecated; new name is \"special_tiles\".");
		lua_getfield(L, index, "special_materials");
	}
	if(lua_istable(L, -1)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while(lua_next(L, table) != 0){
			// Read tiledef from value
			f.tiledef_special[i] = read_tiledef(L, -1, f.drawtype);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			i++;
			if(i==CF_SPECIAL_COUNT){
				lua_pop(L, 1);
				break;
			}
		}
	}
	lua_pop(L, 1);

	f.alpha = getintfield_default(L, index, "alpha", 255);

	bool usealpha = getboolfield_default(L, index,
			"use_texture_alpha", false);
	if (usealpha)
		f.alpha = 0;

	// Read node color.
	lua_getfield(L, index, "color");
	read_color(L, -1, &f.color);
	lua_pop(L, 1);

	getstringfield(L, index, "palette", f.palette_name);

	/* Other stuff */

	lua_getfield(L, index, "post_effect_color");
	read_color(L, -1, &f.post_effect_color);
	lua_pop(L, 1);

	f.param_type = (ContentParamType)getenumfield(L, index, "paramtype",
			ScriptApiNode::es_ContentParamType, CPT_NONE);
	f.param_type_2 = (ContentParamType2)getenumfield(L, index, "paramtype2",
			ScriptApiNode::es_ContentParamType2, CPT2_NONE);

	if (!f.palette_name.empty() &&
			!(f.param_type_2 == CPT2_COLOR ||
			f.param_type_2 == CPT2_COLORED_FACEDIR ||
			f.param_type_2 == CPT2_COLORED_WALLMOUNTED))
		warningstream << "Node " << f.name.c_str()
			<< " has a palette, but not a suitable paramtype2." << std::endl;

	// Warn about some obsolete fields
	warn_if_field_exists(L, index, "wall_mounted",
			"Obsolete; use paramtype2 = 'wallmounted'");
	warn_if_field_exists(L, index, "light_propagates",
			"Obsolete; determined from paramtype");
	warn_if_field_exists(L, index, "dug_item",
			"Obsolete; use 'drop' field");
	warn_if_field_exists(L, index, "extra_dug_item",
			"Obsolete; use 'drop' field");
	warn_if_field_exists(L, index, "extra_dug_item_rarity",
			"Obsolete; use 'drop' field");
	warn_if_field_exists(L, index, "metadata_name",
			"Obsolete; use on_add and metadata callbacks");

	// True for all ground-like things like stone and mud, false for eg. trees
	getboolfield(L, index, "is_ground_content", f.is_ground_content);
	f.light_propagates = (f.param_type == CPT_LIGHT);
	getboolfield(L, index, "sunlight_propagates", f.sunlight_propagates);
	// This is used for collision detection.
	// Also for general solidness queries.
	getboolfield(L, index, "walkable", f.walkable);
	// Player can point to these
	getboolfield(L, index, "pointable", f.pointable);
	// Player can dig these
	getboolfield(L, index, "diggable", f.diggable);
	// Player can climb these
	getboolfield(L, index, "climbable", f.climbable);
	// Player can build on these
	getboolfield(L, index, "buildable_to", f.buildable_to);
	// Liquids flow into and replace node
	getboolfield(L, index, "floodable", f.floodable);
	// Whether the node is non-liquid, source liquid or flowing liquid
	f.liquid_type = (LiquidType)getenumfield(L, index, "liquidtype",
			ScriptApiNode::es_LiquidType, LIQUID_NONE);
	// If the content is liquid, this is the flowing version of the liquid.
	getstringfield(L, index, "liquid_alternative_flowing",
			f.liquid_alternative_flowing);
	// If the content is liquid, this is the source version of the liquid.
	getstringfield(L, index, "liquid_alternative_source",
			f.liquid_alternative_source);
	// Viscosity for fluid flow, ranging from 1 to 7, with
	// 1 giving almost instantaneous propagation and 7 being
	// the slowest possible
	f.liquid_viscosity = getintfield_default(L, index,
			"liquid_viscosity", f.liquid_viscosity);
	f.liquid_range = getintfield_default(L, index,
			"liquid_range", f.liquid_range);
	f.leveled = getintfield_default(L, index, "leveled", f.leveled);
	f.leveled_max = getintfield_default(L, index,
			"leveled_max", f.leveled_max);

	getboolfield(L, index, "liquid_renewable", f.liquid_renewable);
	f.drowning = getintfield_default(L, index,
			"drowning", f.drowning);
	// Amount of light the node emits
	f.light_source = getintfield_default(L, index,
			"light_source", f.light_source);
	if (f.light_source > LIGHT_MAX) {
		warningstream << "Node " << f.name.c_str()
			<< " had greater light_source than " << LIGHT_MAX
			<< ", it was reduced." << std::endl;
		f.light_source = LIGHT_MAX;
	}
	f.damage_per_second = getintfield_default(L, index,
			"damage_per_second", f.damage_per_second);

	lua_getfield(L, index, "node_box");
	if(lua_istable(L, -1))
		f.node_box = read_nodebox(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "connects_to");
	if (lua_istable(L, -1)) {
		int table = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, table) != 0) {
			// Value at -1
			f.connects_to.emplace_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, index, "connect_sides");
	if (lua_istable(L, -1)) {
		int table = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, table) != 0) {
			// Value at -1
			std::string side(lua_tostring(L, -1));
			// Note faces are flipped to make checking easier
			if (side == "top")
				f.connect_sides |= 2;
			else if (side == "bottom")
				f.connect_sides |= 1;
			else if (side == "front")
				f.connect_sides |= 16;
			else if (side == "left")
				f.connect_sides |= 32;
			else if (side == "back")
				f.connect_sides |= 4;
			else if (side == "right")
				f.connect_sides |= 8;
			else
				warningstream << "Unknown value for \"connect_sides\": "
					<< side << std::endl;
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, index, "selection_box");
	if(lua_istable(L, -1))
		f.selection_box = read_nodebox(L, -1);
 	lua_pop(L, 1);

	lua_getfield(L, index, "collision_box");
	if(lua_istable(L, -1))
		f.collision_box = read_nodebox(L, -1);
	lua_pop(L, 1);

	f.waving = getintfield_default(L, index,
			"waving", f.waving);

	// Set to true if paramtype used to be 'facedir_simple'
	getboolfield(L, index, "legacy_facedir_simple", f.legacy_facedir_simple);
	// Set to true if wall_mounted used to be set to true
	getboolfield(L, index, "legacy_wallmounted", f.legacy_wallmounted);

	// Sound table
	lua_getfield(L, index, "sounds");
	if(lua_istable(L, -1)){
		lua_getfield(L, -1, "footstep");
		read_soundspec(L, -1, f.sound_footstep);
		lua_pop(L, 1);
		lua_getfield(L, -1, "dig");
		read_soundspec(L, -1, f.sound_dig);
		lua_pop(L, 1);
		lua_getfield(L, -1, "dug");
		read_soundspec(L, -1, f.sound_dug);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// Node immediately placed by client when node is dug
	getstringfield(L, index, "node_dig_prediction",
		f.node_dig_prediction);

	return f;
}

void push_content_features(lua_State *L, const ContentFeatures &c)
{
	std::string paramtype(ScriptApiNode::es_ContentParamType[(int)c.param_type].str);
	std::string paramtype2(ScriptApiNode::es_ContentParamType2[(int)c.param_type_2].str);
	std::string drawtype(ScriptApiNode::es_DrawType[(int)c.drawtype].str);
	std::string liquid_type(ScriptApiNode::es_LiquidType[(int)c.liquid_type].str);

	/* Missing "tiles" because I don't see a usecase (at least not yet). */

	lua_newtable(L);
	lua_pushboolean(L, c.has_on_construct);
	lua_setfield(L, -2, "has_on_construct");
	lua_pushboolean(L, c.has_on_destruct);
	lua_setfield(L, -2, "has_on_destruct");
	lua_pushboolean(L, c.has_after_destruct);
	lua_setfield(L, -2, "has_after_destruct");
	lua_pushstring(L, c.name.c_str());
	lua_setfield(L, -2, "name");
	push_groups(L, c.groups);
	lua_setfield(L, -2, "groups");
	lua_pushstring(L, paramtype.c_str());
	lua_setfield(L, -2, "paramtype");
	lua_pushstring(L, paramtype2.c_str());
	lua_setfield(L, -2, "paramtype2");
	lua_pushstring(L, drawtype.c_str());
	lua_setfield(L, -2, "drawtype");
	if (!c.mesh.empty()) {
		lua_pushstring(L, c.mesh.c_str());
		lua_setfield(L, -2, "mesh");
	}
#ifndef SERVER
	push_ARGB8(L, c.minimap_color);       // I know this is not set-able w/ register_node,
	lua_setfield(L, -2, "minimap_color"); // but the people need to know!
#endif
	lua_pushnumber(L, c.visual_scale);
	lua_setfield(L, -2, "visual_scale");
	lua_pushnumber(L, c.alpha);
	lua_setfield(L, -2, "alpha");
	if (!c.palette_name.empty()) {
		push_ARGB8(L, c.color);
		lua_setfield(L, -2, "color");

		lua_pushstring(L, c.palette_name.c_str());
		lua_setfield(L, -2, "palette_name");

		push_palette(L, c.palette);
		lua_setfield(L, -2, "palette");
	}
	lua_pushnumber(L, c.waving);
	lua_setfield(L, -2, "waving");
	lua_pushnumber(L, c.connect_sides);
	lua_setfield(L, -2, "connect_sides");

	lua_createtable(L, c.connects_to.size(), 0);
	u16 i = 1;
	for (const std::string &it : c.connects_to) {
		lua_pushlstring(L, it.c_str(), it.size());
		lua_rawseti(L, -2, i++);
	}
	lua_setfield(L, -2, "connects_to");

	push_ARGB8(L, c.post_effect_color);
	lua_setfield(L, -2, "post_effect_color");
	lua_pushnumber(L, c.leveled);
	lua_setfield(L, -2, "leveled");
	lua_pushnumber(L, c.leveled_max);
	lua_setfield(L, -2, "leveled_max");
	lua_pushboolean(L, c.sunlight_propagates);
	lua_setfield(L, -2, "sunlight_propagates");
	lua_pushnumber(L, c.light_source);
	lua_setfield(L, -2, "light_source");
	lua_pushboolean(L, c.is_ground_content);
	lua_setfield(L, -2, "is_ground_content");
	lua_pushboolean(L, c.walkable);
	lua_setfield(L, -2, "walkable");
	lua_pushboolean(L, c.pointable);
	lua_setfield(L, -2, "pointable");
	lua_pushboolean(L, c.diggable);
	lua_setfield(L, -2, "diggable");
	lua_pushboolean(L, c.climbable);
	lua_setfield(L, -2, "climbable");
	lua_pushboolean(L, c.buildable_to);
	lua_setfield(L, -2, "buildable_to");
	lua_pushboolean(L, c.rightclickable);
	lua_setfield(L, -2, "rightclickable");
	lua_pushnumber(L, c.damage_per_second);
	lua_setfield(L, -2, "damage_per_second");
	if (c.isLiquid()) {
		lua_pushstring(L, liquid_type.c_str());
		lua_setfield(L, -2, "liquid_type");
		lua_pushstring(L, c.liquid_alternative_flowing.c_str());
		lua_setfield(L, -2, "liquid_alternative_flowing");
		lua_pushstring(L, c.liquid_alternative_source.c_str());
		lua_setfield(L, -2, "liquid_alternative_source");
		lua_pushnumber(L, c.liquid_viscosity);
		lua_setfield(L, -2, "liquid_viscosity");
		lua_pushboolean(L, c.liquid_renewable);
		lua_setfield(L, -2, "liquid_renewable");
		lua_pushnumber(L, c.liquid_range);
		lua_setfield(L, -2, "liquid_range");
	}
	lua_pushnumber(L, c.drowning);
	lua_setfield(L, -2, "drowning");
	lua_pushboolean(L, c.floodable);
	lua_setfield(L, -2, "floodable");
	push_nodebox(L, c.node_box);
	lua_setfield(L, -2, "node_box");
	push_nodebox(L, c.selection_box);
	lua_setfield(L, -2, "selection_box");
	push_nodebox(L, c.collision_box);
	lua_setfield(L, -2, "collision_box");
	lua_newtable(L);
	push_soundspec(L, c.sound_footstep);
	lua_setfield(L, -2, "sound_footstep");
	push_soundspec(L, c.sound_dig);
	lua_setfield(L, -2, "sound_dig");
	push_soundspec(L, c.sound_dug);
	lua_setfield(L, -2, "sound_dug");
	lua_setfield(L, -2, "sounds");
	lua_pushboolean(L, c.legacy_facedir_simple);
	lua_setfield(L, -2, "legacy_facedir_simple");
	lua_pushboolean(L, c.legacy_wallmounted);
	lua_setfield(L, -2, "legacy_wallmounted");
	lua_pushstring(L, c.node_dig_prediction.c_str());
	lua_setfield(L, -2, "node_dig_prediction");
}

/******************************************************************************/
void push_nodebox(lua_State *L, const NodeBox &box)
{
	lua_newtable(L);
	switch (box.type)
	{
		case NODEBOX_REGULAR:
			lua_pushstring(L, "regular");
			lua_setfield(L, -2, "type");
			break;
		case NODEBOX_LEVELED:
		case NODEBOX_FIXED:
			lua_pushstring(L, "fixed");
			lua_setfield(L, -2, "type");
			push_box(L, box.fixed);
			lua_setfield(L, -2, "fixed");
			break;
		case NODEBOX_WALLMOUNTED:
			lua_pushstring(L, "wallmounted");
			lua_setfield(L, -2, "type");
			push_aabb3f(L, box.wall_top);
			lua_setfield(L, -2, "wall_top");
			push_aabb3f(L, box.wall_bottom);
			lua_setfield(L, -2, "wall_bottom");
			push_aabb3f(L, box.wall_side);
			lua_setfield(L, -2, "wall_side");
			break;
		case NODEBOX_CONNECTED:
			lua_pushstring(L, "connected");
			lua_setfield(L, -2, "type");
			push_box(L, box.connect_top);
			lua_setfield(L, -2, "connect_top");
			push_box(L, box.connect_bottom);
			lua_setfield(L, -2, "connect_bottom");
			push_box(L, box.connect_front);
			lua_setfield(L, -2, "connect_front");
			push_box(L, box.connect_back);
			lua_setfield(L, -2, "connect_back");
			push_box(L, box.connect_left);
			lua_setfield(L, -2, "connect_left");
			push_box(L, box.connect_right);
			lua_setfield(L, -2, "connect_right");
			break;
		default:
			FATAL_ERROR("Invalid box.type");
			break;
	}
}

void push_box(lua_State *L, const std::vector<aabb3f> &box)
{
	lua_createtable(L, box.size(), 0);
	u8 i = 1;
	for (const aabb3f &it : box) {
		push_aabb3f(L, it);
		lua_rawseti(L, -2, i++);
	}
}

/******************************************************************************/
void push_palette(lua_State *L, const std::vector<video::SColor> *palette)
{
	lua_createtable(L, palette->size(), 0);
	int newTable = lua_gettop(L);
	int index = 1;
	std::vector<video::SColor>::const_iterator iter;
	for (iter = palette->begin(); iter != palette->end(); ++iter) {
		push_ARGB8(L, (*iter));
		lua_rawseti(L, newTable, index);
		index++;
	}
}

/******************************************************************************/
void read_server_sound_params(lua_State *L, int index,
		ServerSoundParams &params)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	// Clear
	params = ServerSoundParams();
	if(lua_istable(L, index)){
		getfloatfield(L, index, "gain", params.gain);
		getstringfield(L, index, "to_player", params.to_player);
		getfloatfield(L, index, "fade", params.fade);
		getfloatfield(L, index, "pitch", params.pitch);
		lua_getfield(L, index, "pos");
		if(!lua_isnil(L, -1)){
			v3f p = read_v3f(L, -1)*BS;
			params.pos = p;
			params.type = ServerSoundParams::SSP_POSITIONAL;
		}
		lua_pop(L, 1);
		lua_getfield(L, index, "object");
		if(!lua_isnil(L, -1)){
			ObjectRef *ref = ObjectRef::checkobject(L, -1);
			ServerActiveObject *sao = ObjectRef::getobject(ref);
			if(sao){
				params.object = sao->getId();
				params.type = ServerSoundParams::SSP_OBJECT;
			}
		}
		lua_pop(L, 1);
		params.max_hear_distance = BS*getfloatfield_default(L, index,
				"max_hear_distance", params.max_hear_distance/BS);
		getboolfield(L, index, "loop", params.loop);
		getstringfield(L, index, "exclude_player", params.exclude_player);
	}
}

/******************************************************************************/
void read_soundspec(lua_State *L, int index, SimpleSoundSpec &spec)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	if (lua_isnil(L, index))
		return;

	if (lua_istable(L, index)) {
		getstringfield(L, index, "name", spec.name);
		getfloatfield(L, index, "gain", spec.gain);
		getfloatfield(L, index, "fade", spec.fade);
		getfloatfield(L, index, "pitch", spec.pitch);
	} else if (lua_isstring(L, index)) {
		spec.name = lua_tostring(L, index);
	}
}

void push_soundspec(lua_State *L, const SimpleSoundSpec &spec)
{
	lua_createtable(L, 0, 3);
	lua_pushstring(L, spec.name.c_str());
	lua_setfield(L, -2, "name");
	lua_pushnumber(L, spec.gain);
	lua_setfield(L, -2, "gain");
	lua_pushnumber(L, spec.fade);
	lua_setfield(L, -2, "fade");
	lua_pushnumber(L, spec.pitch);
	lua_setfield(L, -2, "pitch");
}

/******************************************************************************/
NodeBox read_nodebox(lua_State *L, int index)
{
	NodeBox nodebox;
	if (lua_isnil(L, -1))
		return nodebox;

	luaL_checktype(L, -1, LUA_TTABLE);

	nodebox.type = (NodeBoxType)getenumfield(L, index, "type",
			ScriptApiNode::es_NodeBoxType, NODEBOX_REGULAR);

#define NODEBOXREAD(n, s){ \
		lua_getfield(L, index, (s)); \
		if (lua_istable(L, -1)) \
			(n) = read_aabb3f(L, -1, BS); \
		lua_pop(L, 1); \
	}

#define NODEBOXREADVEC(n, s) \
	lua_getfield(L, index, (s)); \
	if (lua_istable(L, -1)) \
		(n) = read_aabb3f_vector(L, -1, BS); \
	lua_pop(L, 1);

	NODEBOXREADVEC(nodebox.fixed, "fixed");
	NODEBOXREAD(nodebox.wall_top, "wall_top");
	NODEBOXREAD(nodebox.wall_bottom, "wall_bottom");
	NODEBOXREAD(nodebox.wall_side, "wall_side");
	NODEBOXREADVEC(nodebox.connect_top, "connect_top");
	NODEBOXREADVEC(nodebox.connect_bottom, "connect_bottom");
	NODEBOXREADVEC(nodebox.connect_front, "connect_front");
	NODEBOXREADVEC(nodebox.connect_left, "connect_left");
	NODEBOXREADVEC(nodebox.connect_back, "connect_back");
	NODEBOXREADVEC(nodebox.connect_right, "connect_right");
	NODEBOXREADVEC(nodebox.disconnected_top, "disconnected_top");
	NODEBOXREADVEC(nodebox.disconnected_bottom, "disconnected_bottom");
	NODEBOXREADVEC(nodebox.disconnected_front, "disconnected_front");
	NODEBOXREADVEC(nodebox.disconnected_left, "disconnected_left");
	NODEBOXREADVEC(nodebox.disconnected_back, "disconnected_back");
	NODEBOXREADVEC(nodebox.disconnected_right, "disconnected_right");
	NODEBOXREADVEC(nodebox.disconnected, "disconnected");
	NODEBOXREADVEC(nodebox.disconnected_sides, "disconnected_sides");

	return nodebox;
}

/******************************************************************************/
MapNode readnode(lua_State *L, int index, const NodeDefManager *ndef)
{
	lua_getfield(L, index, "name");
	if (!lua_isstring(L, -1))
		throw LuaError("Node name is not set or is not a string!");
	std::string name = lua_tostring(L, -1);
	lua_pop(L, 1);

	u8 param1 = 0;
	lua_getfield(L, index, "param1");
	if (!lua_isnil(L, -1))
		param1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	u8 param2 = 0;
	lua_getfield(L, index, "param2");
	if (!lua_isnil(L, -1))
		param2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	content_t id = CONTENT_IGNORE;
	if (!ndef->getId(name, id))
		throw LuaError("\"" + name + "\" is not a registered node!");

	return {id, param1, param2};
}

/******************************************************************************/
void pushnode(lua_State *L, const MapNode &n, const NodeDefManager *ndef)
{
	lua_createtable(L, 0, 3);
	lua_pushstring(L, ndef->get(n).name.c_str());
	lua_setfield(L, -2, "name");
	lua_pushinteger(L, n.getParam1());
	lua_setfield(L, -2, "param1");
	lua_pushinteger(L, n.getParam2());
	lua_setfield(L, -2, "param2");
}

/******************************************************************************/
void warn_if_field_exists(lua_State *L, int table,
		const char *name, const std::string &message)
{
	lua_getfield(L, table, name);
	if (!lua_isnil(L, -1)) {
		warningstream << "Field \"" << name << "\": "
				<< message << std::endl;
		infostream << script_get_backtrace(L) << std::endl;
	}
	lua_pop(L, 1);
}

/******************************************************************************/
int getenumfield(lua_State *L, int table,
		const char *fieldname, const EnumString *spec, int default_)
{
	int result = default_;
	string_to_enum(spec, result,
			getstringfield_default(L, table, fieldname, ""));
	return result;
}

/******************************************************************************/
bool string_to_enum(const EnumString *spec, int &result,
		const std::string &str)
{
	const EnumString *esp = spec;
	while(esp->str){
		if (!strcmp(str.c_str(), esp->str)) {
			result = esp->num;
			return true;
		}
		esp++;
	}
	return false;
}

/******************************************************************************/
ItemStack read_item(lua_State* L, int index, IItemDefManager *idef)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if (lua_isnil(L, index)) {
		return ItemStack();
	}

	if (lua_isuserdata(L, index)) {
		// Convert from LuaItemStack
		LuaItemStack *o = LuaItemStack::checkobject(L, index);
		return o->getItem();
	}

	if (lua_isstring(L, index)) {
		// Convert from itemstring
		std::string itemstring = lua_tostring(L, index);
		try
		{
			ItemStack item;
			item.deSerialize(itemstring, idef);
			return item;
		}
		catch(SerializationError &e)
		{
			warningstream<<"unable to create item from itemstring"
					<<": "<<itemstring<<std::endl;
			return ItemStack();
		}
	}
	else if(lua_istable(L, index))
	{
		// Convert from table
		std::string name = getstringfield_default(L, index, "name", "");
		int count = getintfield_default(L, index, "count", 1);
		int wear = getintfield_default(L, index, "wear", 0);

		ItemStack istack(name, count, wear, idef);

		// BACKWARDS COMPATIBLITY
		std::string value = getstringfield_default(L, index, "metadata", "");
		istack.metadata.setString("", value);

		// Get meta
		lua_getfield(L, index, "meta");
		int fieldstable = lua_gettop(L);
		if (lua_istable(L, fieldstable)) {
			lua_pushnil(L);
			while (lua_next(L, fieldstable) != 0) {
				// key at index -2 and value at index -1
				std::string key = lua_tostring(L, -2);
				size_t value_len;
				const char *value_cs = lua_tolstring(L, -1, &value_len);
				std::string value(value_cs, value_len);
				istack.metadata.setString(key, value);
				lua_pop(L, 1); // removes value, keeps key for next iteration
			}
		}

		return istack;
	} else {
		throw LuaError("Expecting itemstack, itemstring, table or nil");
	}
}

/******************************************************************************/
void push_tool_capabilities(lua_State *L,
		const ToolCapabilities &toolcap)
{
	lua_newtable(L);
	setfloatfield(L, -1, "full_punch_interval", toolcap.full_punch_interval);
	setintfield(L, -1, "max_drop_level", toolcap.max_drop_level);
	setintfield(L, -1, "punch_attack_uses", toolcap.punch_attack_uses);
		// Create groupcaps table
		lua_newtable(L);
		// For each groupcap
		for (const auto &gc_it : toolcap.groupcaps) {
			// Create groupcap table
			lua_newtable(L);
			const std::string &name = gc_it.first;
			const ToolGroupCap &groupcap = gc_it.second;
			// Create subtable "times"
			lua_newtable(L);
			for (auto time : groupcap.times) {
				lua_pushinteger(L, time.first);
				lua_pushnumber(L, time.second);
				lua_settable(L, -3);
			}
			// Set subtable "times"
			lua_setfield(L, -2, "times");
			// Set simple parameters
			setintfield(L, -1, "maxlevel", groupcap.maxlevel);
			setintfield(L, -1, "uses", groupcap.uses);
			// Insert groupcap table into groupcaps table
			lua_setfield(L, -2, name.c_str());
		}
		// Set groupcaps table
		lua_setfield(L, -2, "groupcaps");
		//Create damage_groups table
		lua_newtable(L);
		// For each damage group
		for (const auto &damageGroup : toolcap.damageGroups) {
			// Create damage group table
			lua_pushinteger(L, damageGroup.second);
			lua_setfield(L, -2, damageGroup.first.c_str());
		}
		lua_setfield(L, -2, "damage_groups");
}

/******************************************************************************/
void push_inventory_list(lua_State *L, Inventory *inv, const char *name)
{
	InventoryList *invlist = inv->getList(name);
	if(invlist == NULL){
		lua_pushnil(L);
		return;
	}
	std::vector<ItemStack> items;
	for(u32 i=0; i<invlist->getSize(); i++)
		items.push_back(invlist->getItem(i));
	push_items(L, items);
}

/******************************************************************************/
void read_inventory_list(lua_State *L, int tableindex,
		Inventory *inv, const char *name, Server* srv, int forcesize)
{
	if(tableindex < 0)
		tableindex = lua_gettop(L) + 1 + tableindex;
	// If nil, delete list
	if(lua_isnil(L, tableindex)){
		inv->deleteList(name);
		return;
	}
	// Otherwise set list
	std::vector<ItemStack> items = read_items(L, tableindex,srv);
	int listsize = (forcesize != -1) ? forcesize : items.size();
	InventoryList *invlist = inv->addList(name, listsize);
	int index = 0;
	for(std::vector<ItemStack>::const_iterator
			i = items.begin(); i != items.end(); ++i){
		if(forcesize != -1 && index == forcesize)
			break;
		invlist->changeItem(index, *i);
		index++;
	}
	while(forcesize != -1 && index < forcesize){
		invlist->deleteItem(index);
		index++;
	}
}

/******************************************************************************/
struct TileAnimationParams read_animation_definition(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	struct TileAnimationParams anim;
	anim.type = TAT_NONE;
	if (!lua_istable(L, index))
		return anim;

	anim.type = (TileAnimationType)
		getenumfield(L, index, "type", es_TileAnimationType,
		TAT_NONE);
	if (anim.type == TAT_VERTICAL_FRAMES) {
		// {type="vertical_frames", aspect_w=16, aspect_h=16, length=2.0}
		anim.vertical_frames.aspect_w =
			getintfield_default(L, index, "aspect_w", 16);
		anim.vertical_frames.aspect_h =
			getintfield_default(L, index, "aspect_h", 16);
		anim.vertical_frames.length =
			getfloatfield_default(L, index, "length", 1.0);
	} else if (anim.type == TAT_SHEET_2D) {
		// {type="sheet_2d", frames_w=5, frames_h=3, frame_length=0.5}
		getintfield(L, index, "frames_w",
			anim.sheet_2d.frames_w);
		getintfield(L, index, "frames_h",
			anim.sheet_2d.frames_h);
		getfloatfield(L, index, "frame_length",
			anim.sheet_2d.frame_length);
	}

	return anim;
}

/******************************************************************************/
ToolCapabilities read_tool_capabilities(
		lua_State *L, int table)
{
	ToolCapabilities toolcap;
	getfloatfield(L, table, "full_punch_interval", toolcap.full_punch_interval);
	getintfield(L, table, "max_drop_level", toolcap.max_drop_level);
	getintfield(L, table, "punch_attack_uses", toolcap.punch_attack_uses);
	lua_getfield(L, table, "groupcaps");
	if(lua_istable(L, -1)){
		int table_groupcaps = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table_groupcaps) != 0){
			// key at index -2 and value at index -1
			std::string groupname = luaL_checkstring(L, -2);
			if(lua_istable(L, -1)){
				int table_groupcap = lua_gettop(L);
				// This will be created
				ToolGroupCap groupcap;
				// Read simple parameters
				getintfield(L, table_groupcap, "maxlevel", groupcap.maxlevel);
				getintfield(L, table_groupcap, "uses", groupcap.uses);
				// DEPRECATED: maxwear
				float maxwear = 0;
				if (getfloatfield(L, table_groupcap, "maxwear", maxwear)){
					if (maxwear != 0)
						groupcap.uses = 1.0/maxwear;
					else
						groupcap.uses = 0;
					warningstream << "Field \"maxwear\" is deprecated; "
							<< "replace with uses=1/maxwear" << std::endl;
					infostream << script_get_backtrace(L) << std::endl;
				}
				// Read "times" table
				lua_getfield(L, table_groupcap, "times");
				if(lua_istable(L, -1)){
					int table_times = lua_gettop(L);
					lua_pushnil(L);
					while(lua_next(L, table_times) != 0){
						// key at index -2 and value at index -1
						int rating = luaL_checkinteger(L, -2);
						float time = luaL_checknumber(L, -1);
						groupcap.times[rating] = time;
						// removes value, keeps key for next iteration
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1);
				// Insert groupcap into toolcap
				toolcap.groupcaps[groupname] = groupcap;
			}
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, table, "damage_groups");
	if(lua_istable(L, -1)){
		int table_damage_groups = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table_damage_groups) != 0){
			// key at index -2 and value at index -1
			std::string groupname = luaL_checkstring(L, -2);
			u16 value = luaL_checkinteger(L, -1);
			toolcap.damageGroups[groupname] = value;
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	return toolcap;
}

/******************************************************************************/
void push_dig_params(lua_State *L,const DigParams &params)
{
	lua_createtable(L, 0, 3);
	setboolfield(L, -1, "diggable", params.diggable);
	setfloatfield(L, -1, "time", params.time);
	setintfield(L, -1, "wear", params.wear);
}

/******************************************************************************/
void push_hit_params(lua_State *L,const HitParams &params)
{
	lua_createtable(L, 0, 3);
	setintfield(L, -1, "hp", params.hp);
	setintfield(L, -1, "wear", params.wear);
}

/******************************************************************************/

bool getflagsfield(lua_State *L, int table, const char *fieldname,
	FlagDesc *flagdesc, u32 *flags, u32 *flagmask)
{
	lua_getfield(L, table, fieldname);

	bool success = read_flags(L, -1, flagdesc, flags, flagmask);

	lua_pop(L, 1);

	return success;
}

bool read_flags(lua_State *L, int index, FlagDesc *flagdesc,
	u32 *flags, u32 *flagmask)
{
	if (lua_isstring(L, index)) {
		std::string flagstr = lua_tostring(L, index);
		*flags = readFlagString(flagstr, flagdesc, flagmask);
	} else if (lua_istable(L, index)) {
		*flags = read_flags_table(L, index, flagdesc, flagmask);
	} else {
		return false;
	}

	return true;
}

u32 read_flags_table(lua_State *L, int table, FlagDesc *flagdesc, u32 *flagmask)
{
	u32 flags = 0, mask = 0;
	char fnamebuf[64] = "no";

	for (int i = 0; flagdesc[i].name; i++) {
		bool result;

		if (getboolfield(L, table, flagdesc[i].name, result)) {
			mask |= flagdesc[i].flag;
			if (result)
				flags |= flagdesc[i].flag;
		}

		strlcpy(fnamebuf + 2, flagdesc[i].name, sizeof(fnamebuf) - 2);
		if (getboolfield(L, table, fnamebuf, result))
			mask |= flagdesc[i].flag;
	}

	if (flagmask)
		*flagmask = mask;

	return flags;
}

void push_flags_string(lua_State *L, FlagDesc *flagdesc, u32 flags, u32 flagmask)
{
	std::string flagstring = writeFlagString(flags, flagdesc, flagmask);
	lua_pushlstring(L, flagstring.c_str(), flagstring.size());
}

/******************************************************************************/
/* Lua Stored data!                                                           */
/******************************************************************************/

/******************************************************************************/
void read_groups(lua_State *L, int index, ItemGroupList &result)
{
	if (lua_isnil(L, index))
		return;

	luaL_checktype(L, index, LUA_TTABLE);

	result.clear();
	lua_pushnil(L);
	if (index < 0)
		index -= 1;
	while (lua_next(L, index) != 0) {
		// key at index -2 and value at index -1
		std::string name = luaL_checkstring(L, -2);
		int rating = luaL_checkinteger(L, -1);
		// zero rating indicates not in the group
		if (rating != 0)
			result[name] = rating;
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

/******************************************************************************/
void push_groups(lua_State *L, const ItemGroupList &groups)
{
	lua_createtable(L, 0, groups.size());
	for (const auto &group : groups) {
		lua_pushinteger(L, group.second);
		lua_setfield(L, -2, group.first.c_str());
	}
}

/******************************************************************************/
void push_items(lua_State *L, const std::vector<ItemStack> &items)
{
	lua_createtable(L, items.size(), 0);
	for (u32 i = 0; i != items.size(); i++) {
		LuaItemStack::create(L, items[i]);
		lua_rawseti(L, -2, i + 1);
	}
}

/******************************************************************************/
std::vector<ItemStack> read_items(lua_State *L, int index, Server *srv)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	std::vector<ItemStack> items;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_pushnil(L);
	while (lua_next(L, index)) {
		s32 key = luaL_checkinteger(L, -2);
		if (key < 1) {
			throw LuaError("Invalid inventory list index");
		}
		if (items.size() < (u32) key) {
			items.resize(key);
		}
		items[key - 1] = read_item(L, -1, srv->idef());
		lua_pop(L, 1);
	}
	return items;
}

/******************************************************************************/
void luaentity_get(lua_State *L, u16 id)
{
	// Get luaentities[i]
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushinteger(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // Remove luaentities
	lua_remove(L, -2); // Remove core
}

/******************************************************************************/
bool read_noiseparams(lua_State *L, int index, NoiseParams *np)
{
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	if (!lua_istable(L, index))
		return false;

	getfloatfield(L, index, "offset",      np->offset);
	getfloatfield(L, index, "scale",       np->scale);
	getfloatfield(L, index, "persist",     np->persist);
	getfloatfield(L, index, "persistence", np->persist);
	getfloatfield(L, index, "lacunarity",  np->lacunarity);
	getintfield(L,   index, "seed",        np->seed);
	getintfield(L,   index, "octaves",     np->octaves);

	u32 flags    = 0;
	u32 flagmask = 0;
	np->flags = getflagsfield(L, index, "flags", flagdesc_noiseparams,
		&flags, &flagmask) ? flags : NOISE_FLAG_DEFAULTS;

	lua_getfield(L, index, "spread");
	np->spread  = read_v3f(L, -1);
	lua_pop(L, 1);

	return true;
}

void push_noiseparams(lua_State *L, NoiseParams *np)
{
	lua_newtable(L);
	push_float_string(L, np->offset);
	lua_setfield(L, -2, "offset");
	push_float_string(L, np->scale);
	lua_setfield(L, -2, "scale");
	push_float_string(L, np->persist);
	lua_setfield(L, -2, "persistence");
	push_float_string(L, np->lacunarity);
	lua_setfield(L, -2, "lacunarity");
	lua_pushnumber(L, np->seed);
	lua_setfield(L, -2, "seed");
	lua_pushnumber(L, np->octaves);
	lua_setfield(L, -2, "octaves");

	push_flags_string(L, flagdesc_noiseparams, np->flags,
		np->flags);
	lua_setfield(L, -2, "flags");

	push_v3_float_string(L, np->spread);
	lua_setfield(L, -2, "spread");
}

/******************************************************************************/
// Returns depth of json value tree
static int push_json_value_getdepth(const Json::Value &value)
{
	if (!value.isArray() && !value.isObject())
		return 1;

	int maxdepth = 0;
	for (const auto &it : value) {
		int elemdepth = push_json_value_getdepth(it);
		if (elemdepth > maxdepth)
			maxdepth = elemdepth;
	}
	return maxdepth + 1;
}
// Recursive function to convert JSON --> Lua table
static bool push_json_value_helper(lua_State *L, const Json::Value &value,
		int nullindex)
{
	switch(value.type()) {
		case Json::nullValue:
		default:
			lua_pushvalue(L, nullindex);
			break;
		case Json::intValue:
			lua_pushinteger(L, value.asLargestInt());
			break;
		case Json::uintValue:
			lua_pushinteger(L, value.asLargestUInt());
			break;
		case Json::realValue:
			lua_pushnumber(L, value.asDouble());
			break;
		case Json::stringValue:
			{
				const char *str = value.asCString();
				lua_pushstring(L, str ? str : "");
			}
			break;
		case Json::booleanValue:
			lua_pushboolean(L, value.asInt());
			break;
		case Json::arrayValue:
			lua_createtable(L, value.size(), 0);
			for (Json::Value::const_iterator it = value.begin();
					it != value.end(); ++it) {
				push_json_value_helper(L, *it, nullindex);
				lua_rawseti(L, -2, it.index() + 1);
			}
			break;
		case Json::objectValue:
			lua_createtable(L, 0, value.size());
			for (Json::Value::const_iterator it = value.begin();
					it != value.end(); ++it) {
#if !defined(JSONCPP_STRING) && (JSONCPP_VERSION_MAJOR < 1 || JSONCPP_VERSION_MINOR < 9)
				const char *str = it.memberName();
				lua_pushstring(L, str ? str : "");
#else
				std::string str = it.name();
				lua_pushstring(L, str.c_str());
#endif
				push_json_value_helper(L, *it, nullindex);
				lua_rawset(L, -3);
			}
			break;
	}
	return true;
}
// converts JSON --> Lua table; returns false if lua stack limit exceeded
// nullindex: Lua stack index of value to use in place of JSON null
bool push_json_value(lua_State *L, const Json::Value &value, int nullindex)
{
	if(nullindex < 0)
		nullindex = lua_gettop(L) + 1 + nullindex;

	int depth = push_json_value_getdepth(value);

	// The maximum number of Lua stack slots used at each recursion level
	// of push_json_value_helper is 2, so make sure there a depth * 2 slots
	if (lua_checkstack(L, depth * 2))
		return push_json_value_helper(L, value, nullindex);

	return false;
}

// Converts Lua table --> JSON
void read_json_value(lua_State *L, Json::Value &root, int index, u8 recursion)
{
	if (recursion > 16) {
		throw SerializationError("Maximum recursion depth exceeded");
	}
	int type = lua_type(L, index);
	if (type == LUA_TBOOLEAN) {
		root = (bool) lua_toboolean(L, index);
	} else if (type == LUA_TNUMBER) {
		root = lua_tonumber(L, index);
	} else if (type == LUA_TSTRING) {
		size_t len;
		const char *str = lua_tolstring(L, index, &len);
		root = std::string(str, len);
	} else if (type == LUA_TTABLE) {
		lua_pushnil(L);
		while (lua_next(L, index)) {
			// Key is at -2 and value is at -1
			Json::Value value;
			read_json_value(L, value, lua_gettop(L), recursion + 1);

			Json::ValueType roottype = root.type();
			int keytype = lua_type(L, -1);
			if (keytype == LUA_TNUMBER) {
				lua_Number key = lua_tonumber(L, -1);
				if (roottype != Json::nullValue && roottype != Json::arrayValue) {
					throw SerializationError("Can't mix array and object values in JSON");
				} else if (key < 1) {
					throw SerializationError("Can't use zero-based or negative indexes in JSON");
				} else if (floor(key) != key) {
					throw SerializationError("Can't use indexes with a fractional part in JSON");
				}
				root[(Json::ArrayIndex) key - 1] = value;
			} else if (keytype == LUA_TSTRING) {
				if (roottype != Json::nullValue && roottype != Json::objectValue) {
					throw SerializationError("Can't mix array and object values in JSON");
				}
				root[lua_tostring(L, -1)] = value;
			} else {
				throw SerializationError("Lua key to convert to JSON is not a string or number");
			}
		}
	} else if (type == LUA_TNIL) {
		root = Json::nullValue;
	} else {
		throw SerializationError("Can only store booleans, numbers, strings, objects, arrays, and null in JSON");
	}
	lua_pop(L, 1); // Pop value
}

void push_pointed_thing(lua_State *L, const PointedThing &pointed, bool csm,
	bool hitpoint)
{
	lua_newtable(L);
	if (pointed.type == POINTEDTHING_NODE) {
		lua_pushstring(L, "node");
		lua_setfield(L, -2, "type");
		push_v3s16(L, pointed.node_undersurface);
		lua_setfield(L, -2, "under");
		push_v3s16(L, pointed.node_abovesurface);
		lua_setfield(L, -2, "above");
	} else if (pointed.type == POINTEDTHING_OBJECT) {
		lua_pushstring(L, "object");
		lua_setfield(L, -2, "type");

		if (csm) {
			lua_pushinteger(L, pointed.object_id);
			lua_setfield(L, -2, "id");
		} else {
			push_objectRef(L, pointed.object_id);
			lua_setfield(L, -2, "ref");
		}
	} else {
		lua_pushstring(L, "nothing");
		lua_setfield(L, -2, "type");
	}
	if (hitpoint && (pointed.type != POINTEDTHING_NOTHING)) {
		push_v3f(L, pointed.intersection_point / BS); // convert to node coords
		lua_setfield(L, -2, "intersection_point");
		push_v3s16(L, pointed.intersection_normal);
		lua_setfield(L, -2, "intersection_normal");
		lua_pushinteger(L, pointed.box_id + 1); // change to Lua array index
		lua_setfield(L, -2, "box_id");
	}
}

void push_objectRef(lua_State *L, const u16 id)
{
	// Get core.object_refs[i]
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "object_refs");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushinteger(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // object_refs
	lua_remove(L, -2); // core
}

void read_hud_element(lua_State *L, HudElement *elem)
{
	elem->type = (HudElementType)getenumfield(L, 2, "hud_elem_type",
									es_HudElementType, HUD_ELEM_TEXT);

	lua_getfield(L, 2, "position");
	elem->pos = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "scale");
	elem->scale = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "size");
	elem->size = lua_istable(L, -1) ? read_v2s32(L, -1) : v2s32();
	lua_pop(L, 1);

	elem->name    = getstringfield_default(L, 2, "name", "");
	elem->text    = getstringfield_default(L, 2, "text", "");
	elem->number  = getintfield_default(L, 2, "number", 0);
	if (elem->type == HUD_ELEM_WAYPOINT)
		// waypoints reuse the item field to store precision, item = precision + 1
		elem->item = getintfield_default(L, 2, "precision", -1) + 1;
	else
		elem->item = getintfield_default(L, 2, "item", 0);
	elem->dir     = getintfield_default(L, 2, "direction", 0);
	elem->z_index = MYMAX(S16_MIN, MYMIN(S16_MAX,
			getintfield_default(L, 2, "z_index", 0)));
	elem->text2   = getstringfield_default(L, 2, "text2", "");

	// Deprecated, only for compatibility's sake
	if (elem->dir == 0)
		elem->dir = getintfield_default(L, 2, "dir", 0);

	lua_getfield(L, 2, "alignment");
	elem->align = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "offset");
	elem->offset = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "world_pos");
	elem->world_pos = lua_istable(L, -1) ? read_v3f(L, -1) : v3f();
	lua_pop(L, 1);

	/* check for known deprecated element usage */
	if ((elem->type  == HUD_ELEM_STATBAR) && (elem->size == v2s32()))
		log_deprecated(L,"Deprecated usage of statbar without size!");
}

void push_hud_element(lua_State *L, HudElement *elem)
{
	lua_newtable(L);

	lua_pushstring(L, es_HudElementType[(u8)elem->type].str);
	lua_setfield(L, -2, "type");

	push_v2f(L, elem->pos);
	lua_setfield(L, -2, "position");

	lua_pushstring(L, elem->name.c_str());
	lua_setfield(L, -2, "name");

	push_v2f(L, elem->scale);
	lua_setfield(L, -2, "scale");

	lua_pushstring(L, elem->text.c_str());
	lua_setfield(L, -2, "text");

	lua_pushnumber(L, elem->number);
	lua_setfield(L, -2, "number");

	lua_pushnumber(L, elem->item);
	lua_setfield(L, -2, "item");

	lua_pushnumber(L, elem->dir);
	lua_setfield(L, -2, "direction");

	push_v2f(L, elem->offset);
	lua_setfield(L, -2, "offset");

	push_v2f(L, elem->align);
	lua_setfield(L, -2, "alignment");

	push_v2s32(L, elem->size);
	lua_setfield(L, -2, "size");

	// Deprecated, only for compatibility's sake
	lua_pushnumber(L, elem->dir);
	lua_setfield(L, -2, "dir");

	push_v3f(L, elem->world_pos);
	lua_setfield(L, -2, "world_pos");

	lua_pushnumber(L, elem->z_index);
	lua_setfield(L, -2, "z_index");

	lua_pushstring(L, elem->text2.c_str());
	lua_setfield(L, -2, "text2");
}

HudElementStat read_hud_change(lua_State *L, HudElement *elem, void **value)
{
	HudElementStat stat = HUD_STAT_NUMBER;
	std::string statstr;
	if (lua_isstring(L, 3)) {
		int statint;
		statstr = lua_tostring(L, 3);
		stat = string_to_enum(es_HudElementStat, statint, statstr) ?
				(HudElementStat)statint : stat;
	}

	switch (stat) {
		case HUD_STAT_POS:
			elem->pos = read_v2f(L, 4);
			*value = &elem->pos;
			break;
		case HUD_STAT_NAME:
			elem->name = luaL_checkstring(L, 4);
			*value = &elem->name;
			break;
		case HUD_STAT_SCALE:
			elem->scale = read_v2f(L, 4);
			*value = &elem->scale;
			break;
		case HUD_STAT_TEXT:
			elem->text = luaL_checkstring(L, 4);
			*value = &elem->text;
			break;
		case HUD_STAT_NUMBER:
			elem->number = luaL_checknumber(L, 4);
			*value = &elem->number;
			break;
		case HUD_STAT_ITEM:
			elem->item = luaL_checknumber(L, 4);
			if (elem->type == HUD_ELEM_WAYPOINT && statstr == "precision")
				elem->item++;
			*value = &elem->item;
			break;
		case HUD_STAT_DIR:
			elem->dir = luaL_checknumber(L, 4);
			*value = &elem->dir;
			break;
		case HUD_STAT_ALIGN:
			elem->align = read_v2f(L, 4);
			*value = &elem->align;
			break;
		case HUD_STAT_OFFSET:
			elem->offset = read_v2f(L, 4);
			*value = &elem->offset;
			break;
		case HUD_STAT_WORLD_POS:
			elem->world_pos = read_v3f(L, 4);
			*value = &elem->world_pos;
			break;
		case HUD_STAT_SIZE:
			elem->size = read_v2s32(L, 4);
			*value = &elem->size;
			break;
		case HUD_STAT_Z_INDEX:
			elem->z_index = MYMAX(S16_MIN, MYMIN(S16_MAX, luaL_checknumber(L, 4)));
			*value = &elem->z_index;
			break;
		case HUD_STAT_TEXT2:
			elem->text2 = luaL_checkstring(L, 4);
			*value = &elem->text2;
			break;
	}
	return stat;
}

/******************************************************************************/

// Indices must match values in `enum CollisionType` exactly!!
static const char *collision_type_str[] = {
	"node",
	"object",
};

// Indices must match values in `enum CollisionAxis` exactly!!
static const char *collision_axis_str[] = {
	"x",
	"y",
	"z",
};

void push_collision_move_result(lua_State *L, const collisionMoveResult &res)
{
	lua_createtable(L, 0, 4);

	setboolfield(L, -1, "touching_ground", res.touching_ground);
	setboolfield(L, -1, "collides", res.collides);
	setboolfield(L, -1, "standing_on_object", res.standing_on_object);

	/* collisions */
	lua_createtable(L, res.collisions.size(), 0);
	int i = 1;
	for (const auto &c : res.collisions) {
		lua_createtable(L, 0, 5);

		lua_pushstring(L, collision_type_str[c.type]);
		lua_setfield(L, -2, "type");

		assert(c.axis != COLLISION_AXIS_NONE);
		lua_pushstring(L, collision_axis_str[c.axis]);
		lua_setfield(L, -2, "axis");

		if (c.type == COLLISION_NODE) {
			push_v3s16(L, c.node_p);
			lua_setfield(L, -2, "node_pos");
		} else if (c.type == COLLISION_OBJECT) {
			push_objectRef(L, c.object->getId());
			lua_setfield(L, -2, "object");
		}

		push_v3f(L, c.old_speed / BS);
		lua_setfield(L, -2, "old_velocity");

		push_v3f(L, c.new_speed / BS);
		lua_setfield(L, -2, "new_velocity");

		lua_rawseti(L, -2, i++);
	}
	lua_setfield(L, -2, "collisions");
	/**/
}
