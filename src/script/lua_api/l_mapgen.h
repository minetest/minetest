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

#ifndef L_MAPGEN_H_
#define L_MAPGEN_H_

#include "lua_api/l_base.h"

class ModApiMapgen : public ModApiBase
{
private:
	// get_biome_id(biomename)
	// returns the biome id used in biomemap
	static int l_get_biome_id(lua_State *L);

	// get_mapgen_object(objectname)
	// returns the requested object used during map generation
	static int l_get_mapgen_object(lua_State *L);

	// get_mapgen_params()
	// returns the currently active map generation parameter set
	static int l_get_mapgen_params(lua_State *L);

	// set_mapgen_params(params)
	// set mapgen parameters
	static int l_set_mapgen_params(lua_State *L);

	// get_mapgen_setting(name)
	static int l_get_mapgen_setting(lua_State *L);

	// set_mapgen_setting(name, value, override_meta)
	static int l_set_mapgen_setting(lua_State *L);

	// get_mapgen_setting_noiseparams(name)
	static int l_get_mapgen_setting_noiseparams(lua_State *L);

	// set_mapgen_setting_noiseparams(name, value, override_meta)
	static int l_set_mapgen_setting_noiseparams(lua_State *L);

	// set_noiseparam_defaults(name, noiseparams, set_default)
	static int l_set_noiseparams(lua_State *L);

	// get_noiseparam_defaults(name)
	static int l_get_noiseparams(lua_State *L);

	// set_gen_notify(flagstring)
	static int l_set_gen_notify(lua_State *L);

	// set_gen_notify(flagstring)
	static int l_get_gen_notify(lua_State *L);

	// register_biome({lots of stuff})
	static int l_register_biome(lua_State *L);

	// register_decoration({lots of stuff})
	static int l_register_decoration(lua_State *L);

	// register_ore({lots of stuff})
	static int l_register_ore(lua_State *L);

	// register_schematic({schematic}, replacements={})
	static int l_register_schematic(lua_State *L);

	// clear_registered_biomes()
	static int l_clear_registered_biomes(lua_State *L);

	// clear_registered_decorations()
	static int l_clear_registered_decorations(lua_State *L);

	// clear_registered_schematics()
	static int l_clear_registered_schematics(lua_State *L);

	// generate_ores(vm, p1, p2)
	static int l_generate_ores(lua_State *L);

	// generate_decorations(vm, p1, p2)
	static int l_generate_decorations(lua_State *L);

	// clear_registered_ores
	static int l_clear_registered_ores(lua_State *L);

	// create_schematic(p1, p2, probability_list, filename)
	static int l_create_schematic(lua_State *L);

	// place_schematic(p, schematic, rotation, replacements, force_placement)
	static int l_place_schematic(lua_State *L);

	// place_schematic_on_vmanip(vm, p, schematic,
	//     rotation, replacements, force_placement)
	static int l_place_schematic_on_vmanip(lua_State *L);

	// serialize_schematic(schematic, format, options={...})
	static int l_serialize_schematic(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);

	static struct EnumString es_BiomeTerrainType[];
	static struct EnumString es_DecorationType[];
	static struct EnumString es_MapgenObject[];
	static struct EnumString es_OreType[];
	static struct EnumString es_Rotation[];
	static struct EnumString es_SchematicFormatType[];
	static struct EnumString es_NodeResolveMethod[];
};

#endif /* L_MAPGEN_H_ */
