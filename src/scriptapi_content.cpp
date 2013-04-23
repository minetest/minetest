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

#include "scriptapi.h"
#include "scriptapi_content.h"
#include "scriptapi_types.h"
#include "scriptapi_common.h"
#include "scriptapi_node.h"


NodeBox read_nodebox(lua_State *L, int index)
{
	NodeBox nodebox;
	if(lua_istable(L, -1)){
		nodebox.type = (NodeBoxType)getenumfield(L, index, "type",
				es_NodeBoxType, NODEBOX_REGULAR);

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

/*
	SimpleSoundSpec
*/

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

struct EnumString es_TileAnimationType[] =
{
	{TAT_NONE, "none"},
	{TAT_VERTICAL_FRAMES, "vertical_frames"},
	{0, NULL},
};

/*
	TileDef
*/

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

/*
	ContentFeatures
*/

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

	f.drawtype = (NodeDrawType)getenumfield(L, index, "drawtype", es_DrawType,
			NDT_NORMAL);
	getfloatfield(L, index, "visual_scale", f.visual_scale);

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
			if(i==6){
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
			es_ContentParamType, CPT_NONE);
	f.param_type_2 = (ContentParamType2)getenumfield(L, index, "paramtype2",
			es_ContentParamType2, CPT2_NONE);

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
			es_LiquidType, LIQUID_NONE);
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
	getboolfield(L, index, "liquid_renewable", f.liquid_renewable);
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
