/*
Minetest
Copyright (C) 2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include <map>
#include "irr_v3d.h"
#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class Map;
class MapBlock;
class MMVManip;

/*
  VoxelManip
 */
class LuaVoxelManip : public ModApiBase
{
private:
	std::map<v3s16, MapBlock *> modified_blocks;
	bool is_mapgen_vm = false;

	static const char className[];
	static const luaL_Reg methods[];

	ENTRY_POINT_DECL(gc_object);

	ENTRY_POINT_DECL(l_read_from_map);
	ENTRY_POINT_DECL(l_get_data);
	ENTRY_POINT_DECL(l_set_data);
	ENTRY_POINT_DECL(l_write_to_map);

	ENTRY_POINT_DECL(l_get_node_at);
	ENTRY_POINT_DECL(l_set_node_at);

	ENTRY_POINT_DECL(l_update_map);
	ENTRY_POINT_DECL(l_update_liquids);

	ENTRY_POINT_DECL(l_calc_lighting);
	ENTRY_POINT_DECL(l_set_lighting);
	ENTRY_POINT_DECL(l_get_light_data);
	ENTRY_POINT_DECL(l_set_light_data);

	ENTRY_POINT_DECL(l_get_param2_data);
	ENTRY_POINT_DECL(l_set_param2_data);

	ENTRY_POINT_DECL(l_was_modified);
	ENTRY_POINT_DECL(l_get_emerged_area);

public:
	MMVManip *vm = nullptr;

	LuaVoxelManip(MMVManip *mmvm, bool is_mapgen_vm);
	LuaVoxelManip(Map *map, v3s16 p1, v3s16 p2);
	LuaVoxelManip(Map *map);
	~LuaVoxelManip();

	// LuaVoxelManip()
	// Creates a LuaVoxelManip and leaves it on top of stack
	ENTRY_POINT_DECL(create_object);

	static LuaVoxelManip *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};
