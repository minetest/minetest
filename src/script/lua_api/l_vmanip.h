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

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "irr_v3d.h"
#include "map.h"

/*
  VoxelManip
 */
class LuaVoxelManip
{
private:
	ManualMapVoxelManipulator *vm;
	std::map<v3s16, MapBlock *> modified_blocks;

	static const char className[];
	static const luaL_reg methods[];

	static int gc_object(lua_State *L);

	static int l_read_chunk(lua_State *L);
	static int l_write_chunk(lua_State *L);
	static int l_update_map(lua_State *L);
	static int l_update_liquids(lua_State *L);
	static int l_calc_lighting(lua_State *L);
	static int l_set_lighting(lua_State *L);

public:
	LuaVoxelManip(Map *map);
	~LuaVoxelManip();

	// LuaVoxelManip()
	// Creates a LuaVoxelManip and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaVoxelManip *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

#endif // L_VMANIP_H_
