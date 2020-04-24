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
#include "profiler.h"
#include "server.h"
#include <algorithm>
#include <cmath>

ScriptApiBase *ModApiBase::getScriptApiBase(lua_State *L)
{
	// Get server from registry
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_SCRIPTAPI);
	ScriptApiBase *sapi_ptr;
#if INDIRECT_SCRIPTAPI_RIDX
	sapi_ptr = (ScriptApiBase*) *(void**)(lua_touserdata(L, -1));
#else
	sapi_ptr = (ScriptApiBase*) lua_touserdata(L, -1);
#endif
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
	std::string current_mod_name = readParam<std::string>(L, -1, "");
	if (current_mod_name.empty())
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

std::unordered_map<std::string, luaL_Reg> ModApiBase::m_deprecated_wrappers;
bool ModApiBase::m_error_deprecated_calls = false;

int ModApiBase::l_deprecated_function(lua_State *L)
{
	thread_local std::vector<u64> deprecated_logged;

	u64 start_time = porting::getTimeUs();
	lua_Debug ar;

	// Get function name for lookup
	FATAL_ERROR_IF(!lua_getstack(L, 0, &ar), "lua_getstack() failed");
	FATAL_ERROR_IF(!lua_getinfo(L, "n", &ar), "lua_getinfo() failed");

	// Combine name with line and script backtrace
	FATAL_ERROR_IF(!lua_getstack(L, 1, &ar), "lua_getstack() failed");
	FATAL_ERROR_IF(!lua_getinfo(L, "Sl", &ar), "lua_getinfo() failed");

	// Get parent class to get the wrappers map
	luaL_checktype(L, 1, LUA_TUSERDATA);
	void *ud = lua_touserdata(L, 1);
	ModApiBase *o = *(ModApiBase**)ud;

	// New function and new function name
	auto it = o->m_deprecated_wrappers.find(ar.name);

	// Get backtrace and hash it to reduce the warning flood
	std::string backtrace = ar.short_src;
	backtrace.append(":").append(std::to_string(ar.currentline));
	u64 hash = murmur_hash_64_ua(backtrace.data(), backtrace.length(), 0xBADBABE);

	if (std::find(deprecated_logged.begin(), deprecated_logged.end(), hash)
			== deprecated_logged.end()) {

		deprecated_logged.emplace_back(hash);
		warningstream << "Call to deprecated function '"  << ar.name << "', please use '"
			<< it->second.name << "' at " << backtrace << std::endl;

		if (m_error_deprecated_calls)
			script_error(L, LUA_ERRRUN, NULL, NULL);
	}

	u64 end_time = porting::getTimeUs();
	g_profiler->avg("l_deprecated_function", end_time - start_time);

	return it->second.func(L);
}

void ModApiBase::markAliasDeprecated(luaL_Reg *reg)
{
	std::string value = g_settings->get("deprecated_lua_api_handling");
	m_error_deprecated_calls = value == "error";

	if (!m_error_deprecated_calls && value != "log")
		return;

	const char *last_name = nullptr;
	lua_CFunction last_func = nullptr;

	// ! Null termination !
	while (reg->func) {
		if (last_func == reg->func) {
			// Duplicate found
			luaL_Reg original_reg;
			// Do not inline struct. Breaks MSVC or is error-prone
			original_reg.name = last_name;
			original_reg.func = reg->func;
			m_deprecated_wrappers.emplace(
				std::pair<std::string, luaL_Reg>(reg->name, original_reg));
			reg->func = l_deprecated_function;
		}

		last_func = reg->func;
		last_name = reg->name;
		++reg;
	}
}
