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

#include "lua_api/l_base.h"
#include "environment.h"

class ModApiEnvMod : public ModApiBase {
private:
	// set_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_set_node(lua_State *L);

	static int l_add_node(lua_State *L);

	// remove_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_remove_node(lua_State *L);
	
	// swap_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_swap_node(lua_State *L);

	// get_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node(lua_State *L);

	// get_node_or_nil(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node_or_nil(lua_State *L);

	// get_node_light(pos, timeofday)
	// pos = {x=num, y=num, z=num}
	// timeofday: nil = current time, 0 = night, 0.5 = day
	static int l_get_node_light(lua_State *L);

	// place_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_place_node(lua_State *L);

	// dig_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_dig_node(lua_State *L);

	// punch_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_punch_node(lua_State *L);


	// get_node_max_level(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node_max_level(lua_State *L);

	// get_node_level(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node_level(lua_State *L);

	// set_node_level(pos)
	// pos = {x=num, y=num, z=num}
	static int l_set_node_level(lua_State *L);

	// add_node_level(pos)
	// pos = {x=num, y=num, z=num}
	static int l_add_node_level(lua_State *L);

	// get_meta(pos)
	static int l_get_meta(lua_State *L);

	// get_node_timer(pos)
	static int l_get_node_timer(lua_State *L);

	// add_entity(pos, entityname) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	static int l_add_entity(lua_State *L);

	// add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	static int l_add_item(lua_State *L);

	// get_player_by_name(name)
	static int l_get_player_by_name(lua_State *L);

	// get_objects_inside_radius(pos, radius)
	static int l_get_objects_inside_radius(lua_State *L);

	// set_timeofday(val)
	// val = 0...1
	static int l_set_timeofday(lua_State *L);

	// get_timeofday() -> 0...1
	static int l_get_timeofday(lua_State *L);

	// get_gametime()
	static int l_get_gametime(lua_State *L);

	// find_node_near(pos, radius, nodenames) -> pos or nil
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	static int l_find_node_near(lua_State *L);

	// find_nodes_in_area(minp, maxp, nodenames) -> list of positions
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	static int l_find_nodes_in_area(lua_State *L);

	// get_perlin(seeddiff, octaves, persistence, scale)
	// returns world-specific PerlinNoise
	static int l_get_perlin(lua_State *L);

	// get_perlin_map(noiseparams, size)
	// returns world-specific PerlinNoiseMap
	static int l_get_perlin_map(lua_State *L);
	
	// get_voxel_manip()
	// returns world-specific voxel manipulator
	static int l_get_voxel_manip(lua_State *L);
	
	// clear_objects()
	// clear all objects in the environment
	static int l_clear_objects(lua_State *L);

	// spawn_tree(pos, treedef)
	static int l_spawn_tree(lua_State *L);

	// line_of_sight(pos1, pos2, stepsize) -> true/false
	static int l_line_of_sight(lua_State *L);

	// find_path(pos1, pos2, searchdistance,
	//     max_jump, max_drop, algorithm) -> table containing path
	static int l_find_path(lua_State *L);

	// transforming_liquid_add(pos)
	static int l_transforming_liquid_add(lua_State *L);

	// forceload_block(blockpos)
	// forceloads a block
	static int l_forceload_block(lua_State *L);
	
	// forceload_free_block(blockpos)
	// stops forceloading a position
	static int l_forceload_free_block(lua_State *L);
	
	// get us precision time
	static int l_get_us_time(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
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
