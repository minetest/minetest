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
#include "content/mods.h"
#include "server.h"
#include <cmath>

ScriptApiBase *ModApiBase::getScriptApiBase(lua_State *L)
{
	// Get server from registry
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_SCRIPTAPI);
	ScriptApiBase *sapi_ptr = (ScriptApiBase*) lua_touserdata(L, -1);
	lua_pop(L, 1);
	return sapi_ptr;
}

Server *ModApiBase::getServer(lua_State *L)
{
	return getScriptApiBase(L)->getServer();
}

#ifndef SERVER
Client *ModApiBase::getClient(lua_State *L)
{
	return getScriptApiBase(L)->getClient();
}
#endif

IGameDef *ModApiBase::getGameDef(lua_State *L)
{
	return getScriptApiBase(L)->getGameDef();
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
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	const char *current_mod_name = lua_tostring(L, -1);
	if (!current_mod_name)
		return ".";

	const ModSpec *mod = getServer(L)->getModSpec(current_mod_name);
	if (!mod)
		return ".";

	return mod->path;
}


bool ModApiBase::registerFunction(lua_State *L, const char *name,
		lua_CFunction func, int top)
{
	// TODO: Check presence first!

	lua_pushcfunction(L, func);
	lua_setfield(L, top, name);

	return true;
}

bool ModApiBase::isNaN(lua_State *L, int idx)
{
	return lua_type(L, idx) == LUA_TNUMBER && std::isnan(lua_tonumber(L, idx));
}

/*
 * Read template functions
 */
template<>
float ModApiBase::readParam(lua_State *L, int index)
{
	if (isNaN(L, index))
		throw LuaError("NaN value is not allowed.");

	return (float) luaL_checknumber(L, index);
}
