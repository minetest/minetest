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
#include "itemdef.h"
#include "object_properties.h"
#include "cpp_api/s_node.h"
#include "lua_api/l_object.h"
#include "lua_api/l_item.h"
#include "common/c_internal.h"
#include "server.h"
#include "log.h"
#include "tool.h"
#include "serverobject.h"
#include "porting.h"
#include "mapgen.h"
#include "json/json.h"

struct EnumString es_TileAnimationType[] =
{
	{TAT_NONE, "none"},
	{TAT_VERTICAL_FRAMES, "vertical_frames"},
	{0, NULL},
};

/******************************************************************************/
ItemDefinition read_item_definition(lua_State* L,int index,
		ItemDefinition default_def)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	// Read the item definition
	ItemDefinition def = default_def;

	def.type = (ItemType)getenumfield(L, index, "type",
			es_ItemType, ITEM_NONE);
	getstringfield(L, index, "name", def.name);
	getstringfield(L, index, "description", def.description);
	getstringfield(L, index, "inventory_image", def.inventory_image);
	getstringfield(L, index, "wield_image", def.wield_image);

	lua_getfield(L, index, "wield_scale");
	if(lua_istable(L, -1)){
		def.wield_scale = check_v3f(L, -1);
	}
	lua_pop(L, 1);

	def.stack_max = getintfield_default(L, index, "stack_max", def.stack_max);
	if(def.stack_max == 0)
		def.stack_max = 1;

	lua_getfield(L, index, "on_use");
	def.usable = lua_isfunction(L, -1);
	lua_pop(L, 1);

	getboolfield(L, index, "liquids_pointable", def.liquids_pointable);

	warn_if_field_exists(L, index, "tool_digging_properties",
			"deprecated: use tool_capabilities");

	lua_getfield(L, index, "tool_capabilities");
	if(lua_istable(L, -1)){
		def.tool_capabilities = new ToolCapabilities(
				read_tool_capabilities(L, -1));
	}

	// If name is "" (hand), ensure there are ToolCapabilities
	// because it will be looked up there whenever any other item has
	// no ToolCapabilities
	if(def.name == "" && def.tool_capabilities == NULL){
		def.tool_capabilities = new ToolCapabilities();
	}

	lua_getfield(L, index, "groups");
	read_groups(L, -1, def.groups);
	lua_pop(L, 1);

	lua_getfield(L, index, "sounds");
	if(lua_istable(L, -1)){
		lua_getfield(L, -1, "place");
		read_soundspec(L, -1, def.sound_place);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	def.range = getfloatfield_default(L, index, "range", def.range);

	// Client shall immediately place this node when player places the item.
	// Server will update the precise end result a moment later.
	// "" = no prediction
	getstringfield(L, index, "node_placement_prediction",
			def.node_placement_prediction);

	return def;
}

/******************************************************************************/
void read_object_properties(lua_State *L, int index,
		ObjectProperties *prop)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	if(!lua_istable(L, index))
		return;

	prop->hp_max = getintfield_default(L, -1, "hp_max", 10);

	getboolfield(L, -1, "physical", prop->physical);
	getboolfield(L, -1, "collide_with_objects", prop->collideWithObjects);

	getfloatfield(L, -1, "weight", prop->weight);

	lua_getfield(L, -1, "collisionbox");
	if(lua_istable(L, -1))
		prop->collisionbox = read_aabb3f(L, -1, 1.0);
	lua_pop(L, 1);

	getstringfield(L, -1, "visual", prop->visual);

	getstringfield(L, -1, "mesh", prop->mesh);

	lua_getfield(L, -1, "visual_size");
	if(lua_istable(L, -1))
		prop->visual_size = read_v2f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "textures");
	if(lua_istable(L, -1)){
		prop->textures.clear();
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			if(lua_isstring(L, -1))
				prop->textures.push_back(lua_tostring(L, -1));
			else
				prop->textures.push_back("");
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "colors");
	if(lua_istable(L, -1)){
		prop->colors.clear();
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			if(lua_isstring(L, -1))
				prop->colors.push_back(readARGB8(L, -1));
			else
				prop->colors.push_back(video::SColor(255, 255, 255, 255));
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
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
	getfloatfield(L, -1, "automatic_rotate", prop->automatic_rotate);
	getfloatfield(L, -1, "stepheight", prop->stepheight);
	prop->stepheight*=BS;
	lua_getfield(L, -1, "automatic_face_movement_dir");
	if (lua_isnumber(L, -1)) {
		prop->automatic_face_movement_dir = true;
		prop->automatic_face_movement_dir_offset = luaL_checknumber(L, -1);
	} else if (lua_isboolean(L, -1)) {
		prop->automatic_face_movement_dir = lua_toboolean(L, -1);
		prop->automatic_face_movement_dir_offset = 0.0;
	}
	lua_pop(L, 1);
}

/******************************************************************************/
TileDef read_tiledef(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	TileDef tiledef;

	// key at index -2 and value at index
	if(lua_isstring(L, index)){
		// "default_lava.png"
		tiledef.name = lua_tostring(L, index);
	}
	else if(lua_istable(L, index))
	{
		// {name="default_lava.png", animation={}}
		tiledef.name = "";
		getstringfield(L, index, "name", tiledef.name);
		getstringfield(L, index, "image", tiledef.name); // MaterialSpec compat.
		tiledef.backface_culling = getboolfield_default(
					L, index, "backface_culling", true);
		// animation = {}
		lua_getfield(L, index, "animation");
		if(lua_istable(L, -1)){
			// {type="vertical_frames", aspect_w=16, aspect_h=16, length=2.0}
			tiledef.animation.type = (TileAnimationType)
					getenumfield(L, -1, "type", es_TileAnimationType,
					TAT_NONE);
			tiledef.animation.aspect_w =
					getintfield_default(L, -1, "aspect_w", 16);
			tiledef.animation.aspect_h =
					getintfield_default(L, -1, "aspect_h", 16);
			tiledef.animation.length =
					getfloatfield_default(L, -1, "length", 1.0);
		}
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
			f.tiledef[i] = read_tiledef(L, -1);
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
			f.tiledef_special[i] = read_tiledef(L, -1);
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

	/* Other stuff */

	lua_getfield(L, index, "post_effect_color");
	if(!lua_isnil(L, -1))
		f.post_effect_color = readARGB8(L, -1);
	lua_pop(L, 1);

	f.param_type = (ContentParamType)getenumfield(L, index, "paramtype",
			ScriptApiNode::es_ContentParamType, CPT_NONE);
	f.param_type_2 = (ContentParamType2)getenumfield(L, index, "paramtype2",
			ScriptApiNode::es_ContentParamType2, CPT2_NONE);

	// Warn about some deprecated fields
	warn_if_field_exists(L, index, "wall_mounted",
			"deprecated: use paramtype2 = 'wallmounted'");
	warn_if_field_exists(L, index, "light_propagates",
			"deprecated: determined from paramtype");
	warn_if_field_exists(L, index, "dug_item",
			"deprecated: use 'drop' field");
	warn_if_field_exists(L, index, "extra_dug_item",
			"deprecated: use 'drop' field");
	warn_if_field_exists(L, index, "extra_dug_item_rarity",
			"deprecated: use 'drop' field");
	warn_if_field_exists(L, index, "metadata_name",
			"deprecated: use on_add and metadata callbacks");

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

	getboolfield(L, index, "liquid_renewable", f.liquid_renewable);
	getstringfield(L, index, "freezemelt", f.freezemelt);
	f.drowning = getintfield_default(L, index,
			"drowning", f.drowning);
	// Amount of light the node emits
	f.light_source = getintfield_default(L, index,
			"light_source", f.light_source);
	f.damage_per_second = getintfield_default(L, index,
			"damage_per_second", f.damage_per_second);

	lua_getfield(L, index, "node_box");
	if(lua_istable(L, -1))
		f.node_box = read_nodebox(L, -1);
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

	return f;
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
	}
}

/******************************************************************************/
void read_soundspec(lua_State *L, int index, SimpleSoundSpec &spec)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	if(lua_isnil(L, index)){
	} else if(lua_istable(L, index)){
		getstringfield(L, index, "name", spec.name);
		getfloatfield(L, index, "gain", spec.gain);
	} else if(lua_isstring(L, index)){
		spec.name = lua_tostring(L, index);
	}
}

/******************************************************************************/
NodeBox read_nodebox(lua_State *L, int index)
{
	NodeBox nodebox;
	if(lua_istable(L, -1)){
		nodebox.type = (NodeBoxType)getenumfield(L, index, "type",
				ScriptApiNode::es_NodeBoxType, NODEBOX_REGULAR);

		lua_getfield(L, index, "fixed");
		if(lua_istable(L, -1))
			nodebox.fixed = read_aabb3f_vector(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, index, "wall_top");
		if(lua_istable(L, -1))
			nodebox.wall_top = read_aabb3f(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, index, "wall_bottom");
		if(lua_istable(L, -1))
			nodebox.wall_bottom = read_aabb3f(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, index, "wall_side");
		if(lua_istable(L, -1))
			nodebox.wall_side = read_aabb3f(L, -1, BS);
		lua_pop(L, 1);
	}
	return nodebox;
}

/******************************************************************************/
MapNode readnode(lua_State *L, int index, INodeDefManager *ndef)
{
	lua_getfield(L, index, "name");
	const char *name = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	u8 param1;
	lua_getfield(L, index, "param1");
	if(lua_isnil(L, -1))
		param1 = 0;
	else
		param1 = lua_tonumber(L, -1);
	lua_pop(L, 1);
	u8 param2;
	lua_getfield(L, index, "param2");
	if(lua_isnil(L, -1))
		param2 = 0;
	else
		param2 = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return MapNode(ndef, name, param1, param2);
}

/******************************************************************************/
void pushnode(lua_State *L, const MapNode &n, INodeDefManager *ndef)
{
	lua_newtable(L);
	lua_pushstring(L, ndef->get(n).name.c_str());
	lua_setfield(L, -2, "name");
	lua_pushnumber(L, n.getParam1());
	lua_setfield(L, -2, "param1");
	lua_pushnumber(L, n.getParam2());
	lua_setfield(L, -2, "param2");
}

/******************************************************************************/
void warn_if_field_exists(lua_State *L, int table,
		const char *fieldname, const std::string &message)
{
	lua_getfield(L, table, fieldname);
	if(!lua_isnil(L, -1)){
//TODO find way to access backtrace fct from here
		//		infostream<<script_get_backtrace(L)<<std::endl;
		infostream<<"WARNING: field \""<<fieldname<<"\": "
				<<message<<std::endl;
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
		if(str == std::string(esp->str)){
			result = esp->num;
			return true;
		}
		esp++;
	}
	return false;
}

/******************************************************************************/
ItemStack read_item(lua_State* L, int index,Server* srv)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(lua_isnil(L, index))
	{
		return ItemStack();
	}
	else if(lua_isuserdata(L, index))
	{
		// Convert from LuaItemStack
		LuaItemStack *o = LuaItemStack::checkobject(L, index);
		return o->getItem();
	}
	else if(lua_isstring(L, index))
	{
		// Convert from itemstring
		std::string itemstring = lua_tostring(L, index);
		IItemDefManager *idef = srv->idef();
		try
		{
			ItemStack item;
			item.deSerialize(itemstring, idef);
			return item;
		}
		catch(SerializationError &e)
		{
			infostream<<"WARNING: unable to create item from itemstring"
					<<": "<<itemstring<<std::endl;
			return ItemStack();
		}
	}
	else if(lua_istable(L, index))
	{
		// Convert from table
		IItemDefManager *idef = srv->idef();
		std::string name = getstringfield_default(L, index, "name", "");
		int count = getintfield_default(L, index, "count", 1);
		int wear = getintfield_default(L, index, "wear", 0);
		std::string metadata = getstringfield_default(L, index, "metadata", "");
		return ItemStack(name, count, wear, metadata, idef);
	}
	else
	{
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
		// Create groupcaps table
		lua_newtable(L);
		// For each groupcap
		for(std::map<std::string, ToolGroupCap>::const_iterator
				i = toolcap.groupcaps.begin(); i != toolcap.groupcaps.end(); i++){
			// Create groupcap table
			lua_newtable(L);
			const std::string &name = i->first;
			const ToolGroupCap &groupcap = i->second;
			// Create subtable "times"
			lua_newtable(L);
			for(std::map<int, float>::const_iterator
					i = groupcap.times.begin(); i != groupcap.times.end(); i++){
				int rating = i->first;
				float time = i->second;
				lua_pushinteger(L, rating);
				lua_pushnumber(L, time);
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
		for(std::map<std::string, s16>::const_iterator
				i = toolcap.damageGroups.begin(); i != toolcap.damageGroups.end(); i++){
			// Create damage group table
			lua_pushinteger(L, i->second);
			lua_setfield(L, -2, i->first.c_str());
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
			i = items.begin(); i != items.end(); i++){
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
ToolCapabilities read_tool_capabilities(
		lua_State *L, int table)
{
	ToolCapabilities toolcap;
	getfloatfield(L, table, "full_punch_interval", toolcap.full_punch_interval);
	getintfield(L, table, "max_drop_level", toolcap.max_drop_level);
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
				if(getfloatfield(L, table_groupcap, "maxwear", maxwear)){
					if(maxwear != 0)
						groupcap.uses = 1.0/maxwear;
					else
						groupcap.uses = 0;
					infostream<<script_get_backtrace(L)<<std::endl;
					infostream<<"WARNING: field \"maxwear\" is deprecated; "
							<<"should replace with uses=1/maxwear"<<std::endl;
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
	lua_newtable(L);
	setboolfield(L, -1, "diggable", params.diggable);
	setfloatfield(L, -1, "time", params.time);
	setintfield(L, -1, "wear", params.wear);
}

/******************************************************************************/
void push_hit_params(lua_State *L,const HitParams &params)
{
	lua_newtable(L);
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

/******************************************************************************/
/* Lua Stored data!                                                           */
/******************************************************************************/

/******************************************************************************/
void read_groups(lua_State *L, int index,
		std::map<std::string, int> &result)
{
	if (!lua_istable(L,index))
		return;
	result.clear();
	lua_pushnil(L);
	if(index < 0)
		index -= 1;
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		std::string name = luaL_checkstring(L, -2);
		int rating = luaL_checkinteger(L, -1);
		result[name] = rating;
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

/******************************************************************************/
void push_items(lua_State *L, const std::vector<ItemStack> &items)
{
	// Create and fill table
	lua_createtable(L, items.size(), 0);
	std::vector<ItemStack>::const_iterator iter = items.begin();
	for (u32 i = 0; iter != items.end(); iter++) {
		LuaItemStack::create(L, *iter);
		lua_rawseti(L, -2, ++i);
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
		items[key - 1] = read_item(L, -1, srv);
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
	lua_pushnumber(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // Remove luaentities
	lua_remove(L, -2); // Remove core
}

/******************************************************************************/
NoiseParams *read_noiseparams(lua_State *L, int index)
{
	NoiseParams *np = new NoiseParams;

	if (!read_noiseparams_nc(L, index, np)) {
		delete np;
		np = NULL;
	}

	return np;
}

bool read_noiseparams_nc(lua_State *L, int index, NoiseParams *np)
{
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	if (!lua_istable(L, index))
		return false;

	np->offset  = getfloatfield_default(L, index, "offset",  0.0);
	np->scale   = getfloatfield_default(L, index, "scale",   0.0);
	np->persist = getfloatfield_default(L, index, "persist", 0.0);
	np->seed    = getintfield_default(L,   index, "seed",    0);
	np->octaves = getintfield_default(L,   index, "octaves", 0);

	lua_getfield(L, index, "spread");
	np->spread  = read_v3f(L, -1);
	lua_pop(L, 1);

	return true;
}

/******************************************************************************/
bool read_schematic(lua_State *L, int index, DecoSchematic *dschem, Server *server) {
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	INodeDefManager *ndef = server->getNodeDefManager();

	if (lua_istable(L, index)) {
		lua_getfield(L, index, "size");
		v3s16 size = read_v3s16(L, -1);
		lua_pop(L, 1);
		
		int numnodes = size.X * size.Y * size.Z;
		MapNode *schemdata = new MapNode[numnodes];
		int i = 0;
		
		// Get schematic data
		lua_getfield(L, index, "data");
		luaL_checktype(L, -1, LUA_TTABLE);
		
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			if (i < numnodes) {
				// same as readnode, except param1 default is MTSCHEM_PROB_CONST
				lua_getfield(L, -1, "name");
				const char *name = luaL_checkstring(L, -1);
				lua_pop(L, 1);
				
				u8 param1;
				lua_getfield(L, -1, "param1");
				param1 = !lua_isnil(L, -1) ? lua_tonumber(L, -1) : MTSCHEM_PROB_ALWAYS;
				lua_pop(L, 1);
	
				u8 param2;
				lua_getfield(L, -1, "param2");
				param2 = !lua_isnil(L, -1) ? lua_tonumber(L, -1) : 0;
				lua_pop(L, 1);
				
				schemdata[i] = MapNode(ndef, name, param1, param2);
			}
			
			i++;
			lua_pop(L, 1);
		}
		
		if (i != numnodes) {
			errorstream << "read_schematic: incorrect number of "
				"nodes provided in raw schematic data (got " << i <<
				", expected " << numnodes << ")." << std::endl;
			return false;
		}

		u8 *sliceprobs = new u8[size.Y];
		for (i = 0; i != size.Y; i++)
			sliceprobs[i] = MTSCHEM_PROB_ALWAYS;

		// Get Y-slice probability values (if present)
		lua_getfield(L, index, "yslice_prob");
		if (lua_istable(L, -1)) {
			lua_pushnil(L);
			while (lua_next(L, -2)) {
				if (getintfield(L, -1, "ypos", i) && i >= 0 && i < size.Y) {
					sliceprobs[i] = getintfield_default(L, -1,
						"prob", MTSCHEM_PROB_ALWAYS);
				}
				lua_pop(L, 1);
			}
		}

		dschem->size        = size;
		dschem->schematic   = schemdata;
		dschem->slice_probs = sliceprobs;

	} else if (lua_isstring(L, index)) {
		dschem->filename = std::string(lua_tostring(L, index));
	} else {
		errorstream << "read_schematic: missing schematic "
			"filename or raw schematic data" << std::endl;
		return false;
	}
	
	return true;
}

/******************************************************************************/
// Returns depth of json value tree
static int push_json_value_getdepth(const Json::Value &value)
{
	if (!value.isArray() && !value.isObject())
		return 1;

	int maxdepth = 0;
	for (Json::Value::const_iterator it = value.begin();
			it != value.end(); ++it) {
		int elemdepth = push_json_value_getdepth(*it);
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
			lua_pushinteger(L, value.asInt());
			break;
		case Json::uintValue:
			lua_pushinteger(L, value.asUInt());
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
			lua_newtable(L);
			for (Json::Value::const_iterator it = value.begin();
					it != value.end(); ++it) {
				push_json_value_helper(L, *it, nullindex);
				lua_rawseti(L, -2, it.index() + 1);
			}
			break;
		case Json::objectValue:
			lua_newtable(L);
			for (Json::Value::const_iterator it = value.begin();
					it != value.end(); ++it) {
				const char *str = it.memberName();
				lua_pushstring(L, str ? str : "");
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
	else
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

