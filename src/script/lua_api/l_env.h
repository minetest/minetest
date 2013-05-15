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

#ifndef L_ENV_H_
#define L_ENV_H_

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "environment.h"
#include "lua_api/l_base.h"

#ifndef SCRIPTAPI_DISABLE_DEPRECATED
#define DEPR_FCT_DEF(name) static int l_depr_##name(lua_State *L)
#define API_FCT_DEPRECATED(name) registerFunction(L,#name,l_depr_##name,env_top)
#define DEPR_FCT(name) int ModApiEnvMod::l_depr_##name(lua_State *L) {         \
						/* warn deprecated */                                  \
						/* TODO enable this warning once old function usage has dropped */ \
						/*infostream << "Warning called deprecated fct: minetest.env:" << #name << std::endl;*/ \
						/* remove first parameter */                           \
						lua_remove(L,1);                                       \
						/* call function */                                    \
						return l_##name(L);                                    \
							}
#else
#define DEPR_FCT_DEF(name)
#define DEPR_FCT(name)
#define API_FCT_DEPRECATED(name)
#endif

class ModApiEnvMod
	:public ModApiBase
{
private:
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

	//get line of sight
	static int l_line_of_sight(lua_State *L);

	//find a path between two positions
	static int l_find_path(lua_State *L);

	DEPR_FCT_DEF(set_node);
	DEPR_FCT_DEF(add_node);
	DEPR_FCT_DEF(add_item);
	DEPR_FCT_DEF(get_node);
	DEPR_FCT_DEF(remove_node);
	DEPR_FCT_DEF(get_node_or_nil);
	DEPR_FCT_DEF(get_node_light);
	DEPR_FCT_DEF(place_node);
	DEPR_FCT_DEF(dig_node);
	DEPR_FCT_DEF(punch_node);
	DEPR_FCT_DEF(add_entity);
	DEPR_FCT_DEF(add_rat);
	DEPR_FCT_DEF(add_firefly);
	DEPR_FCT_DEF(get_meta);
	DEPR_FCT_DEF(get_node_timer);
	DEPR_FCT_DEF(get_player_by_name);
	DEPR_FCT_DEF(get_objects_inside_radius);
	DEPR_FCT_DEF(set_timeofday);
	DEPR_FCT_DEF(get_timeofday);
	DEPR_FCT_DEF(find_node_near);
	DEPR_FCT_DEF(find_nodes_in_area);
	DEPR_FCT_DEF(get_perlin);
	DEPR_FCT_DEF(get_perlin_map);
	DEPR_FCT_DEF(clear_objects);
	DEPR_FCT_DEF(spawn_tree);
	DEPR_FCT_DEF(line_of_sight);
	DEPR_FCT_DEF(find_path);

public:
	bool Initialize(lua_State *L, int top);
};

class LuaABM : public ActiveBlockModifier
{
private:
	int m_id;

	std::set<std::string> m_trigger_contents;
	std::set<std::string> m_required_neighbors;
	float m_trigger_interval;
	u32 m_trigger_chance;
public:
	LuaABM(lua_State *L, int id,
			const std::set<std::string> &trigger_contents,
			const std::set<std::string> &required_neighbors,
			float trigger_interval, u32 trigger_chance):
		m_id(id),
		m_trigger_contents(trigger_contents),
		m_required_neighbors(required_neighbors),
		m_trigger_interval(trigger_interval),
		m_trigger_chance(trigger_chance)
	{
	}
	virtual std::set<std::string> getTriggerContents()
	{
		return m_trigger_contents;
	}
	virtual std::set<std::string> getRequiredNeighbors()
	{
		return m_required_neighbors;
	}
	virtual float getTriggerInterval()
	{
		return m_trigger_interval;
	}
	virtual u32 getTriggerChance()
	{
		return m_trigger_chance;
	}
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider);
};

#endif /* L_ENV_H_ */
