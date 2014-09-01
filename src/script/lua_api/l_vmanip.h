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

#ifndef L_VMANIP_H_
#define L_VMANIP_H_

#include "lua_api/l_base.h"
#include "irr_v3d.h"
#include <map>

class Map;
class MapBlock;
class ManualMapVoxelManipulator;

/*
  VoxelManip
 */
class LuaVoxelManip : public ModApiBase {
private:
	ManualMapVoxelManipulator *vm;
	std::map<v3s16, MapBlock *> modified_blocks;
	bool is_mapgen_vm;

	static const char className[];
	static const luaL_reg methods[];

	static int gc_object(lua_State *L);

	static int l_read_from_map(lua_State *L);
	static int l_get_data(lua_State *L);
	static int l_set_data(lua_State *L);
	static int l_write_to_map(lua_State *L);

	static int l_get_node_at(lua_State *L);
	static int l_set_node_at(lua_State *L);

	static int l_update_map(lua_State *L);
	static int l_update_liquids(lua_State *L);

	static int l_calc_lighting(lua_State *L);
	static int l_set_lighting(lua_State *L);
	static int l_get_light_data(lua_State *L);
	static int l_set_light_data(lua_State *L);

	static int l_get_param2_data(lua_State *L);
	static int l_set_param2_data(lua_State *L);

	static int l_was_modified(lua_State *L);

public:
	LuaVoxelManip(ManualMapVoxelManipulator *mmvm, bool is_mapgen_vm);
	LuaVoxelManip(Map *map);
	~LuaVoxelManip();

	// LuaVoxelManip()
	// Creates a LuaVoxelManip and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaVoxelManip *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

#endif /* L_VMANIP_H_ */
