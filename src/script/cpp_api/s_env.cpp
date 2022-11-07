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

#include "cpp_api/s_env.h"
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"
#include "log.h"
#include "environment.h"
#include "mapgen/mapgen.h"
#include "lua_api/l_env.h"
#include "server.h"
#include "script/common/c_content.h"


void ScriptApiEnv::environment_OnGenerated(v3s16 minp, v3s16 maxp,
	u32 blockseed)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_generateds
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_generateds");
	// Call callbacks
	push_v3s16(L, minp);
	push_v3s16(L, maxp);
	lua_pushnumber(L, blockseed);
	runCallbacks(3, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiEnv::environment_Step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER
	//infostream << "scriptapi_environment_step" << std::endl;

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiEnv::player_event(ServerActiveObject *player, const std::string &type)
{
	SCRIPTAPI_PRECHECKHEADER

	if (player == NULL)
		return;

	// Get minetest.registered_playerevents
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_playerevents");

	// Call callbacks
	objectrefGetOrCreate(L, player);   // player
	lua_pushstring(L,type.c_str()); // event type
	runCallbacks(2, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiEnv::initializeEnvironment(ServerEnvironment *env)
{
	SCRIPTAPI_PRECHECKHEADER
	verbosestream << "ScriptApiEnv: Environment initialized" << std::endl;
	setEnv(env);

	/*
		Add {Loading,Active}BlockModifiers to environment
	*/

	// Get core.registered_abms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_abms");
	int registered_abms = lua_gettop(L);

	if (!lua_istable(L, registered_abms)) {
		lua_pop(L, 1);
		throw LuaError("core.registered_abms was not a lua table, as expected.");
	}
	lua_pushnil(L);
	while (lua_next(L, registered_abms)) {
		// key at index -2 and value at index -1
		int id = lua_tonumber(L, -2);
		int current_abm = lua_gettop(L);

		std::vector<std::string> trigger_contents;
		lua_getfield(L, current_abm, "nodenames");
		if (lua_istable(L, -1)) {
			int table = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, table)) {
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				trigger_contents.emplace_back(readParam<std::string>(L, -1));
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if (lua_isstring(L, -1)) {
			trigger_contents.emplace_back(readParam<std::string>(L, -1));
		}
		lua_pop(L, 1);

		std::vector<std::string> required_neighbors;
		lua_getfield(L, current_abm, "neighbors");
		if (lua_istable(L, -1)) {
			int table = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, table)) {
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				required_neighbors.emplace_back(readParam<std::string>(L, -1));
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if (lua_isstring(L, -1)) {
			required_neighbors.emplace_back(readParam<std::string>(L, -1));
		}
		lua_pop(L, 1);

		float trigger_interval = 10.0;
		getfloatfield(L, current_abm, "interval", trigger_interval);

		int trigger_chance = 50;
		getintfield(L, current_abm, "chance", trigger_chance);

		bool simple_catch_up = true;
		getboolfield(L, current_abm, "catch_up", simple_catch_up);

		s16 min_y = INT16_MIN;
		getintfield(L, current_abm, "min_y", min_y);

		s16 max_y = INT16_MAX;
		getintfield(L, current_abm, "max_y", max_y);

		lua_getfield(L, current_abm, "action");
		luaL_checktype(L, current_abm + 1, LUA_TFUNCTION);
		lua_pop(L, 1);

		LuaABM *abm = new LuaABM(L, id, trigger_contents, required_neighbors,
			trigger_interval, trigger_chance, simple_catch_up, min_y, max_y);

		env->addActiveBlockModifier(abm);

		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// Get core.registered_lbms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_lbms");
	int registered_lbms = lua_gettop(L);

	if (!lua_istable(L, registered_lbms)) {
		lua_pop(L, 1);
		throw LuaError("core.registered_lbms was not a lua table, as expected.");
	}

	lua_pushnil(L);
	while (lua_next(L, registered_lbms)) {
		// key at index -2 and value at index -1
		int id = lua_tonumber(L, -2);
		int current_lbm = lua_gettop(L);

		std::set<std::string> trigger_contents;
		lua_getfield(L, current_lbm, "nodenames");
		if (lua_istable(L, -1)) {
			int table = lua_gettop(L);
			lua_pushnil(L);
			while (lua_next(L, table)) {
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				trigger_contents.insert(readParam<std::string>(L, -1));
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if (lua_isstring(L, -1)) {
			trigger_contents.insert(readParam<std::string>(L, -1));
		}
		lua_pop(L, 1);

		std::string name;
		getstringfield(L, current_lbm, "name", name);

		bool run_at_every_load = getboolfield_default(L, current_lbm,
			"run_at_every_load", false);

		lua_getfield(L, current_lbm, "action");
		luaL_checktype(L, current_lbm + 1, LUA_TFUNCTION);
		lua_pop(L, 1);

		LuaLBM *lbm = new LuaLBM(L, id, trigger_contents, name,
			run_at_every_load);

		env->addLoadingBlockModifierDef(lbm);

		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

void ScriptApiEnv::on_emerge_area_completion(
	v3s16 blockpos, int action, ScriptCallbackState *state)
{
	Server *server = getServer();

	// This function should be executed with envlock held.
	// The caller (LuaEmergeAreaCallback in src/script/lua_api/l_env.cpp)
	// should have obtained the lock.
	// Note that the order of these locks is important!  Envlock must *ALWAYS*
	// be acquired before attempting to acquire scriptlock, or else ServerThread
	// will try to acquire scriptlock after it already owns envlock, thus
	// deadlocking EmergeThread and ServerThread

	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, state->callback_ref);
	luaL_checktype(L, -1, LUA_TFUNCTION);

	push_v3s16(L, blockpos);
	lua_pushinteger(L, action);
	lua_pushinteger(L, state->refcount);
	lua_rawgeti(L, LUA_REGISTRYINDEX, state->args_ref);

	setOriginDirect(state->origin.c_str());

	try {
		PCALL_RES(lua_pcall(L, 4, 0, error_handler));
	} catch (LuaError &e) {
		// Note: don't throw here, we still need to run the cleanup code below
		server->setAsyncFatalError(e);
	}

	lua_pop(L, 1); // Pop error handler

	if (state->refcount == 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, state->callback_ref);
		luaL_unref(L, LUA_REGISTRYINDEX, state->args_ref);
	}
}

void ScriptApiEnv::check_for_falling(v3s16 p)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "check_for_falling");
	luaL_checktype(L, -1, LUA_TFUNCTION);
	push_v3s16(L, p);
	PCALL_RES(lua_pcall(L, 1, 0, error_handler));
}

void ScriptApiEnv::on_liquid_transformed(
	const std::vector<std::pair<v3s16, MapNode>> &list)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_liquid_transformed
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_liquid_transformed");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_remove(L, -2);

	// Skip converting list and calling hook if there are
	// no registered callbacks.
	if(lua_objlen(L, -1) < 1) return;

	// Convert the list to a pos array and a node array for lua
	int index = 1;
	lua_createtable(L, list.size(), 0);
	lua_createtable(L, list.size(), 0);
	for(std::pair<v3s16, MapNode> p : list) {
		lua_pushnumber(L, index);
		push_v3s16(L, p.first);
		lua_rawset(L, -4);
		lua_pushnumber(L, index++);
		pushnode(L, p.second);
		lua_rawset(L, -3);
	}

	runCallbacks(2, RUN_CALLBACKS_MODE_FIRST);
}
