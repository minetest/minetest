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

#include "lua_api/l_rollback.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "server.h"
#include "rollback.h"


// rollback_get_last_node_actor(p, range, seconds) -> actor, p, seconds
int ModApiRollback::l_rollback_get_last_node_actor(lua_State *L)
{
	v3s16 p = read_v3s16(L, 1);
	int range = luaL_checknumber(L, 2);
	int seconds = luaL_checknumber(L, 3);
	Server *server = getServer(L);
	IRollbackManager *rollback = server->getRollbackManager();
	v3s16 act_p;
	int act_seconds = 0;
	std::string actor = rollback->getLastNodeActor(p, range, seconds, &act_p, &act_seconds);
	lua_pushstring(L, actor.c_str());
	push_v3s16(L, act_p);
	lua_pushnumber(L, act_seconds);
	return 3;
}

// rollback_revert_actions_by(actor, seconds) -> bool, log messages
int ModApiRollback::l_rollback_revert_actions_by(lua_State *L)
{
	std::string actor = luaL_checkstring(L, 1);
	int seconds = luaL_checknumber(L, 2);
	Server *server = getServer(L);
	IRollbackManager *rollback = server->getRollbackManager();
	std::list<RollbackAction> actions = rollback->getRevertActions(actor, seconds);
	std::list<std::string> log;
	bool success = server->rollbackRevertActions(actions, &log);
	// Push boolean result
	lua_pushboolean(L, success);
	// Get the table insert function and push the log table
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	lua_newtable(L);
	int table = lua_gettop(L);
	for(std::list<std::string>::const_iterator i = log.begin();
			i != log.end(); i++)
	{
		lua_pushvalue(L, table_insert);
		lua_pushvalue(L, table);
		lua_pushstring(L, i->c_str());
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
	lua_remove(L, -2); // Remove table
	lua_remove(L, -2); // Remove insert
	return 2;
}

void ModApiRollback::Initialize(lua_State *L, int top)
{
	API_FCT(rollback_get_last_node_actor);
	API_FCT(rollback_revert_actions_by);
}
