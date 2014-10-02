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
#include "mapgen.h"
#include "lua_api/l_env.h"
#include "server.h"

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
	script_run_callbacks(L, 3, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiEnv::environment_Step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER
	//infostream<<"scriptapi_environment_step"<<std::endl;

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	try {
		script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getServer()->setAsyncFatalError(e.what());
	}
}

void ScriptApiEnv::player_event(ServerActiveObject* player, std::string type)
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
	try {
		script_run_callbacks(L, 2, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getServer()->setAsyncFatalError(e.what());
	}
}

void ScriptApiEnv::environment_OnMapgenInit(MapgenParams *mgparams)
{
	SCRIPTAPI_PRECHECKHEADER
	
	// Get core.registered_on_mapgen_inits
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_mapgen_inits");

	// Call callbacks
	lua_newtable(L);
	
	lua_pushstring(L, mgparams->mg_name.c_str());
	lua_setfield(L, -2, "mgname");
	
	lua_pushinteger(L, mgparams->seed);
	lua_setfield(L, -2, "seed");
	
	lua_pushinteger(L, mgparams->water_level);
	lua_setfield(L, -2, "water_level");
	
	std::string flagstr = writeFlagString(mgparams->flags,
		flagdesc_mapgen, (u32)-1);
	lua_pushstring(L, flagstr.c_str());
	lua_setfield(L, -2, "flags");
	
	script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiEnv::initializeEnvironment(ServerEnvironment *env)
{
	SCRIPTAPI_PRECHECKHEADER
	verbosestream<<"scriptapi_add_environment"<<std::endl;
	setEnv(env);

	/*
		Add ActiveBlockModifiers to environment
	*/

	// Get core.registered_abms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_abms");
	luaL_checktype(L, -1, LUA_TTABLE);
	int registered_abms = lua_gettop(L);

	if(lua_istable(L, registered_abms)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			int id = lua_tonumber(L, -2);
			int current_abm = lua_gettop(L);

			std::set<std::string> trigger_contents;
			lua_getfield(L, current_abm, "nodenames");
			if(lua_istable(L, -1)){
				int table = lua_gettop(L);
				lua_pushnil(L);
				while(lua_next(L, table) != 0){
					// key at index -2 and value at index -1
					luaL_checktype(L, -1, LUA_TSTRING);
					trigger_contents.insert(lua_tostring(L, -1));
					// removes value, keeps key for next iteration
					lua_pop(L, 1);
				}
			} else if(lua_isstring(L, -1)){
				trigger_contents.insert(lua_tostring(L, -1));
			}
			lua_pop(L, 1);

			std::set<std::string> required_neighbors;
			lua_getfield(L, current_abm, "neighbors");
			if(lua_istable(L, -1)){
				int table = lua_gettop(L);
				lua_pushnil(L);
				while(lua_next(L, table) != 0){
					// key at index -2 and value at index -1
					luaL_checktype(L, -1, LUA_TSTRING);
					required_neighbors.insert(lua_tostring(L, -1));
					// removes value, keeps key for next iteration
					lua_pop(L, 1);
				}
			} else if(lua_isstring(L, -1)){
				required_neighbors.insert(lua_tostring(L, -1));
			}
			lua_pop(L, 1);

			float trigger_interval = 10.0;
			getfloatfield(L, current_abm, "interval", trigger_interval);

			int trigger_chance = 50;
			getintfield(L, current_abm, "chance", trigger_chance);

			LuaABM *abm = new LuaABM(L, id, trigger_contents,
					required_neighbors, trigger_interval, trigger_chance);

			env->addActiveBlockModifier(abm);

			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
}
