/*
Minetest-c55
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

#ifndef LUA_ENVIRONMENT_H_
#define LUA_ENVIRONMENT_H_

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "environment.h"

/*
	EnvRef
*/

class EnvRef
{
private:
	ServerEnvironment *m_env;

	static const char className[];
	static const luaL_reg methods[];

	static int gc_object(lua_State *L) ;

	static EnvRef *checkobject(lua_State *L, int narg);

	// Exported functions

	// EnvRef:set_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_set_node(lua_State *L);

	static int l_add_node(lua_State *L);

	// EnvRef:remove_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_remove_node(lua_State *L);

	// EnvRef:get_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node(lua_State *L);

	// EnvRef:get_node_or_nil(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node_or_nil(lua_State *L);

	// EnvRef:get_node_light(pos, timeofday)
	// pos = {x=num, y=num, z=num}
	// timeofday: nil = current time, 0 = night, 0.5 = day
	static int l_get_node_light(lua_State *L);

	// EnvRef:place_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_place_node(lua_State *L);

	// EnvRef:dig_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_dig_node(lua_State *L);

	// EnvRef:punch_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_punch_node(lua_State *L);

	// EnvRef:get_meta(pos)
	static int l_get_meta(lua_State *L);

	// EnvRef:get_node_timer(pos)
	static int l_get_node_timer(lua_State *L);

	// EnvRef:add_entity(pos, entityname) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	static int l_add_entity(lua_State *L);

	// EnvRef:add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	static int l_add_item(lua_State *L);

	// EnvRef:add_rat(pos)
	// pos = {x=num, y=num, z=num}
	static int l_add_rat(lua_State *L);

	// EnvRef:add_firefly(pos)
	// pos = {x=num, y=num, z=num}
	static int l_add_firefly(lua_State *L);

	// EnvRef:get_player_by_name(name)
	static int l_get_player_by_name(lua_State *L);

	// EnvRef:get_objects_inside_radius(pos, radius)
	static int l_get_objects_inside_radius(lua_State *L);

	// EnvRef:set_timeofday(val)
	// val = 0...1
	static int l_set_timeofday(lua_State *L);

	// EnvRef:get_timeofday() -> 0...1
	static int l_get_timeofday(lua_State *L);

	// EnvRef:find_node_near(pos, radius, nodenames) -> pos or nil
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	static int l_find_node_near(lua_State *L);

	// EnvRef:find_nodes_in_area(minp, maxp, nodenames) -> list of positions
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	static int l_find_nodes_in_area(lua_State *L);

	//	EnvRef:get_perlin(seeddiff, octaves, persistence, scale)
	//  returns world-specific PerlinNoise
	static int l_get_perlin(lua_State *L);

	//  EnvRef:get_perlin_map(noiseparams, size)
	//  returns world-specific PerlinNoiseMap
	static int l_get_perlin_map(lua_State *L);

	// EnvRef:clear_objects()
	// clear all objects in the environment
	static int l_clear_objects(lua_State *L);

	static int l_spawn_tree(lua_State *L);

public:
	EnvRef(ServerEnvironment *env);

	~EnvRef();

	// Creates an EnvRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, ServerEnvironment *env);

	static void set_null(lua_State *L);

	static void Register(lua_State *L);
};

/*****************************************************************************/
/* Minetest interface                                                        */
/*****************************************************************************/
// On environment step
void scriptapi_environment_step(lua_State *L, float dtime);
// After generating a piece of map
void scriptapi_environment_on_generated(lua_State *L, v3s16 minp, v3s16 maxp,
		u32 blockseed);
void scriptapi_add_environment(lua_State *L, ServerEnvironment *env);

#endif /* LUA_ENVIRONMENT_H_ */
