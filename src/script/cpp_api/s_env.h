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

#include "cpp_api/s_base.h"
#include "irr_v3d.h"
#include "mapnode.h"
#include <unordered_set>
#include <vector>

class ServerEnvironment;
class MapBlock;
struct ScriptCallbackState;

class ScriptApiEnv : virtual public ScriptApiBase
{
public:
	// Called on environment step
	void environment_Step(float dtime);

	// Called after generating a piece of map
	void environment_OnGenerated(v3s16 minp, v3s16 maxp, u32 blockseed);

	// Called on player event
	void player_event(ServerActiveObject *player, const std::string &type);

	// Called after emerge of a block queued from core.emerge_area()
	void on_emerge_area_completion(v3s16 blockpos, int action,
		ScriptCallbackState *state);

	void check_for_falling(v3s16 p);

	// Called after liquid transform changes
	void on_liquid_transformed(const std::vector<std::pair<v3s16, MapNode>> &list);

	// Called after mapblock changes
	void on_mapblocks_changed(const std::unordered_set<v3s16> &set);

	// Determines whether there are any on_mapblocks_changed callbacks
	bool has_on_mapblocks_changed();

	// Initializes environment and loads some definitions from Lua
	void initializeEnvironment(ServerEnvironment *env);

	void triggerABM(int id, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider);

	void triggerLBM(int id, MapBlock *block,
		const std::unordered_set<v3s16> &positions, float dtime_s);

private:
	void readABMs();

	void readLBMs();

	// Reads a single or a list of node names into a vector
	static bool read_nodenames(lua_State *L, int idx, std::vector<std::string> &to);
};
