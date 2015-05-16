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

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"
#include "cpp_api/s_base.h"
#include <mods.h>
#include <server.h>

ScriptApiBase *ModApiBase::getScriptApiBase(lua_State *L)
{
	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "scriptapi");
	ScriptApiBase *sapi_ptr = (ScriptApiBase*) lua_touserdata(L, -1);
	lua_pop(L, 1);
	return sapi_ptr;
}

Server *ModApiBase::getServer(lua_State *L)
{
	return getScriptApiBase(L)->getServer();
}

Environment *ModApiBase::getEnv(lua_State *L)
{
	return getScriptApiBase(L)->getEnv();
}

GUIEngine *ModApiBase::getGuiEngine(lua_State *L)
{
	return getScriptApiBase(L)->getGuiEngine();
}

std::string ModApiBase::getCurrentModPath(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, SCRIPT_MOD_NAME_FIELD);
	const char *current_mod_name = lua_tostring(L, -1);
	if (!current_mod_name)
		return ".";

	const ModSpec *mod = getServer(L)->getModSpec(current_mod_name);
	if (!mod)
		return ".";

	return mod->path;
}


bool ModApiBase::registerFunction(
	lua_State *L,
	const char *name,
	lua_CFunction fct,
	int top)
{
	//TODO check presence first!

	lua_pushstring(L,name);
	lua_pushcfunction(L,fct);
	lua_settable(L, top);

	return true;
}
