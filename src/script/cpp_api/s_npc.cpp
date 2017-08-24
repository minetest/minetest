/*
Minetest
Copyright (C) 2017 Vincent Glize, Dumbeldor <vincent.glize@live.fr>

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

#include "cpp_api/s_npc.h"
#include "cpp_api/s_internal.h"
#include "log.h"
#include "object_properties.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"

bool ScriptApiNpc::luanpc_Add(u16 id, const char *name)
{
	SCRIPTAPI_PRECHECKHEADER

	verbosestream<<"scriptapi_luanpc_add: id="<<id<<" name=\""
				 <<name<<"\""<<std::endl;

	// Get core.registered_entities[name]
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_npc");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	// Should be a table, which we will use as a prototype
	//luaL_checktype(L, -1, LUA_TTABLE);
	if (lua_type(L, -1) != LUA_TTABLE){
		errorstream<<"luanpc name \""<<name<<"\" not defined"<<std::endl;
		return false;
	}
	int prototype_table = lua_gettop(L);
	//dump2(L, "prototype_table");

	// Create npc object
	lua_newtable(L);
	int object = lua_gettop(L);

	// Set object metatable
	lua_pushvalue(L, prototype_table);
	lua_setmetatable(L, -2);

	// Add object reference
	// This should be userdata with metatable ObjectRef
	push_objectRef(L, id);
	luaL_checktype(L, -1, LUA_TUSERDATA);
	if (!luaL_checkudata(L, -1, "ObjectRef"))
		luaL_typerror(L, -1, "ObjectRef");
	lua_setfield(L, -2, "object");

	// core.luaentities[id] = object
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "luanpc");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id); // Push id
	lua_pushvalue(L, object); // Copy object to top of stack
	lua_settable(L, -3);

	return true;
}

