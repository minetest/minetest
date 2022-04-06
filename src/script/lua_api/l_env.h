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
#include "serverenvironment.h"
#include "raycast.h"

class ModApiEnvMod : public ModApiBase {
private:
	// set_node(pos, node)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_set_node);

	// bulk_set_node([pos1, pos2, ...], node)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_bulk_set_node);

	ENTRY_POINT_DECL(l_add_node);

	// remove_node(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_remove_node);

	// swap_node(pos, node)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_swap_node);

	// get_node(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_get_node);

	// get_node_or_nil(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_get_node_or_nil);

	// get_node_light(pos, timeofday)
	// pos = {x=num, y=num, z=num}
	// timeofday: nil = current time, 0 = night, 0.5 = day
	ENTRY_POINT_DECL(l_get_node_light);

	// get_natural_light(pos, timeofday)
	// pos = {x=num, y=num, z=num}
	// timeofday: nil = current time, 0 = night, 0.5 = day
	ENTRY_POINT_DECL(l_get_natural_light);

	// place_node(pos, node)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_place_node);

	// dig_node(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_dig_node);

	// punch_node(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_punch_node);

	// get_node_max_level(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_get_node_max_level);

	// get_node_level(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_get_node_level);

	// set_node_level(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_set_node_level);

	// add_node_level(pos)
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_add_node_level);

	// find_nodes_with_meta(pos1, pos2)
	ENTRY_POINT_DECL(l_find_nodes_with_meta);

	// get_meta(pos)
	ENTRY_POINT_DECL(l_get_meta);

	// get_node_timer(pos)
	ENTRY_POINT_DECL(l_get_node_timer);

	// add_entity(pos, entityname) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_add_entity);

	// add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	ENTRY_POINT_DECL(l_add_item);

	// get_connected_players()
	ENTRY_POINT_DECL(l_get_connected_players);

	// get_player_by_name(name)
	ENTRY_POINT_DECL(l_get_player_by_name);

	// get_objects_inside_radius(pos, radius)
	ENTRY_POINT_DECL(l_get_objects_inside_radius);
	
	// get_objects_in_area(pos, minp, maxp)
	ENTRY_POINT_DECL(l_get_objects_in_area);

	// set_timeofday(val)
	// val = 0...1
	ENTRY_POINT_DECL(l_set_timeofday);

	// get_timeofday() -> 0...1
	ENTRY_POINT_DECL(l_get_timeofday);

	// get_gametime()
	ENTRY_POINT_DECL(l_get_gametime);

	// get_day_count() -> int
	ENTRY_POINT_DECL(l_get_day_count);

	// find_node_near(pos, radius, nodenames, search_center) -> pos or nil
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	ENTRY_POINT_DECL(l_find_node_near);

	// find_nodes_in_area(minp, maxp, nodenames) -> list of positions
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	ENTRY_POINT_DECL(l_find_nodes_in_area);

	// find_surface_nodes_in_area(minp, maxp, nodenames) -> list of positions
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	ENTRY_POINT_DECL(l_find_nodes_in_area_under_air);

	// fix_light(p1, p2) -> true/false
	ENTRY_POINT_DECL(l_fix_light);

	// load_area(p1)
	ENTRY_POINT_DECL(l_load_area);

	// emerge_area(p1, p2)
	ENTRY_POINT_DECL(l_emerge_area);

	// delete_area(p1, p2) -> true/false
	ENTRY_POINT_DECL(l_delete_area);

	// get_perlin(seeddiff, octaves, persistence, scale)
	// returns world-specific PerlinNoise
	ENTRY_POINT_DECL(l_get_perlin);

	// get_perlin_map(noiseparams, size)
	// returns world-specific PerlinNoiseMap
	ENTRY_POINT_DECL(l_get_perlin_map);

	// get_voxel_manip()
	// returns world-specific voxel manipulator
	ENTRY_POINT_DECL(l_get_voxel_manip);

	// clear_objects()
	// clear all objects in the environment
	ENTRY_POINT_DECL(l_clear_objects);

	// spawn_tree(pos, treedef)
	ENTRY_POINT_DECL(l_spawn_tree);

	// line_of_sight(pos1, pos2) -> true/false
	ENTRY_POINT_DECL(l_line_of_sight);

	// raycast(pos1, pos2, objects, liquids) -> Raycast
	// ENTRY_POINT_DECL(l_raycast); (Alias of LuaRaycast::create_object);

	// find_path(pos1, pos2, searchdistance,
	//     max_jump, max_drop, algorithm) -> table containing path
	ENTRY_POINT_DECL(l_find_path);

	// transforming_liquid_add(pos)
	ENTRY_POINT_DECL(l_transforming_liquid_add);

	// forceload_block(blockpos)
	// forceloads a block
	ENTRY_POINT_DECL(l_forceload_block);

	// forceload_free_block(blockpos)
	// stops forceloading a position
	ENTRY_POINT_DECL(l_forceload_free_block);

	// compare_block_status(nodepos)
	ENTRY_POINT_DECL(l_compare_block_status);

	// Get a string translated server side
	ENTRY_POINT_DECL(l_get_translated_string);

	/* Helpers */

	static void collectNodeIds(lua_State *L, int idx,
		const NodeDefManager *ndef, std::vector<content_t> &filter);

public:
	static void Initialize(lua_State *L, int top);
	static void InitializeClient(lua_State *L, int top);

	static const EnumString es_ClearObjectsMode[];
	static const EnumString es_BlockStatusType[];
};

class LuaABM : public ActiveBlockModifier {
private:
	int m_id;

	std::vector<std::string> m_trigger_contents;
	std::vector<std::string> m_required_neighbors;
	float m_trigger_interval;
	u32 m_trigger_chance;
	bool m_simple_catch_up;
	s16 m_min_y;
	s16 m_max_y;
public:
	LuaABM(lua_State *L, int id,
			const std::vector<std::string> &trigger_contents,
			const std::vector<std::string> &required_neighbors,
			float trigger_interval, u32 trigger_chance, bool simple_catch_up, s16 min_y, s16 max_y):
		m_id(id),
		m_trigger_contents(trigger_contents),
		m_required_neighbors(required_neighbors),
		m_trigger_interval(trigger_interval),
		m_trigger_chance(trigger_chance),
		m_simple_catch_up(simple_catch_up),
		m_min_y(min_y),
		m_max_y(max_y)
	{
	}
	virtual const std::vector<std::string> &getTriggerContents() const
	{
		return m_trigger_contents;
	}
	virtual const std::vector<std::string> &getRequiredNeighbors() const
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
	virtual bool getSimpleCatchUp()
	{
		return m_simple_catch_up;
	}
	virtual s16 getMinY()
	{
		return m_min_y;
	}
	virtual s16 getMaxY()
	{
		return m_max_y;
	}
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider);
};

class LuaLBM : public LoadingBlockModifierDef
{
private:
	int m_id;
public:
	LuaLBM(lua_State *L, int id,
			const std::set<std::string> &trigger_contents,
			const std::string &name,
			bool run_at_every_load):
		m_id(id)
	{
		this->run_at_every_load = run_at_every_load;
		this->trigger_contents = trigger_contents;
		this->name = name;
	}
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n);
};

//! Lua wrapper for RaycastState objects
class LuaRaycast : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];
	//! Inner state
	RaycastState state;

	// Exported functions

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	/*!
	 * Raycast:next() -> pointed_thing
	 * Returns the next pointed thing on the ray.
	 */
	ENTRY_POINT_DECL(l_next);
public:
	//! Constructor with the same arguments as RaycastState.
	LuaRaycast(
		const core::line3d<f32> &shootline,
		bool objects_pointable,
		bool liquids_pointable) :
		state(shootline, objects_pointable, liquids_pointable)
	{}

	//! Creates a LuaRaycast and leaves it on top of the stack.
	ENTRY_POINT_DECL(create_object);

	/*!
	 * Returns the Raycast from the stack or throws an error.
	 * @param narg location of the RaycastState in the stack
	 */
	static LuaRaycast *checkobject(lua_State *L, int narg);

	//! Registers Raycast as a Lua userdata type.
	static void Register(lua_State *L);
};

struct ScriptCallbackState {
	ServerScripting *script;
	int callback_ref;
	int args_ref;
	unsigned int refcount;
	std::string origin;
};
