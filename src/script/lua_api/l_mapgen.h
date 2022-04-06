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

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

typedef u16 biome_t;  // copy from mg_biome.h to avoid an unnecessary include

class ModApiMapgen : public ModApiBase
{
private:
	// get_biome_id(biomename)
	// returns the biome id as used in biomemap and returned by 'get_biome_data()'
	ENTRY_POINT_DECL(l_get_biome_id);

	// get_biome_name(biome_id)
	// returns the biome name string
	ENTRY_POINT_DECL(l_get_biome_name);

	// get_heat(pos)
	// returns the heat at the position
	ENTRY_POINT_DECL(l_get_heat);

	// get_humidity(pos)
	// returns the humidity at the position
	ENTRY_POINT_DECL(l_get_humidity);

	// get_biome_data(pos)
	// returns a table containing the biome id, heat and humidity at the position
	ENTRY_POINT_DECL(l_get_biome_data);

	// get_mapgen_object(objectname)
	// returns the requested object used during map generation
	ENTRY_POINT_DECL(l_get_mapgen_object);

	// get_spawn_level(x = num, z = num)
	ENTRY_POINT_DECL(l_get_spawn_level);

	// get_mapgen_params()
	// returns the currently active map generation parameter set
	ENTRY_POINT_DECL(l_get_mapgen_params);

	// set_mapgen_params(params)
	// set mapgen parameters
	ENTRY_POINT_DECL(l_set_mapgen_params);

	// get_mapgen_setting(name)
	ENTRY_POINT_DECL(l_get_mapgen_setting);

	// set_mapgen_setting(name, value, override_meta)
	ENTRY_POINT_DECL(l_set_mapgen_setting);

	// get_mapgen_setting_noiseparams(name)
	ENTRY_POINT_DECL(l_get_mapgen_setting_noiseparams);

	// set_mapgen_setting_noiseparams(name, value, override_meta)
	ENTRY_POINT_DECL(l_set_mapgen_setting_noiseparams);

	// set_noiseparam_defaults(name, noiseparams, set_default)
	ENTRY_POINT_DECL(l_set_noiseparams);

	// get_noiseparam_defaults(name)
	ENTRY_POINT_DECL(l_get_noiseparams);

	// set_gen_notify(flags, {deco_id_table})
	ENTRY_POINT_DECL(l_set_gen_notify);

	// get_gen_notify()
	ENTRY_POINT_DECL(l_get_gen_notify);

	// get_decoration_id(decoration_name)
	// returns the decoration ID as used in gennotify
	ENTRY_POINT_DECL(l_get_decoration_id);

	// register_biome({lots of stuff})
	ENTRY_POINT_DECL(l_register_biome);

	// register_decoration({lots of stuff})
	ENTRY_POINT_DECL(l_register_decoration);

	// register_ore({lots of stuff})
	ENTRY_POINT_DECL(l_register_ore);

	// register_schematic({schematic}, replacements={})
	ENTRY_POINT_DECL(l_register_schematic);

	// clear_registered_biomes()
	ENTRY_POINT_DECL(l_clear_registered_biomes);

	// clear_registered_decorations()
	ENTRY_POINT_DECL(l_clear_registered_decorations);

	// clear_registered_schematics()
	ENTRY_POINT_DECL(l_clear_registered_schematics);

	// generate_ores(vm, p1, p2)
	ENTRY_POINT_DECL(l_generate_ores);

	// generate_decorations(vm, p1, p2)
	ENTRY_POINT_DECL(l_generate_decorations);

	// clear_registered_ores
	ENTRY_POINT_DECL(l_clear_registered_ores);

	// create_schematic(p1, p2, probability_list, filename)
	ENTRY_POINT_DECL(l_create_schematic);

	// place_schematic(p, schematic, rotation,
	//     replacements, force_placement, flagstring)
	ENTRY_POINT_DECL(l_place_schematic);

	// place_schematic_on_vmanip(vm, p, schematic, rotation,
	//     replacements, force_placement, flagstring)
	ENTRY_POINT_DECL(l_place_schematic_on_vmanip);

	// serialize_schematic(schematic, format, options={...})
	ENTRY_POINT_DECL(l_serialize_schematic);

	// read_schematic(schematic, options={...})
	ENTRY_POINT_DECL(l_read_schematic);

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
