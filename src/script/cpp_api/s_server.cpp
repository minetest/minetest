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

#include "cpp_api/s_server.h"
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"

bool ScriptApiServer::getAuth(const std::string &playername,
		std::string *dst_password,
		std::set<std::string> *dst_privs)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);
	getAuthHandler();
	lua_getfield(L, -1, "get_auth");
	if (lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError("Authentication handler missing get_auth");
	lua_pushstring(L, playername.c_str());
	PCALL_RES(lua_pcall(L, 1, 1, error_handler));
	lua_remove(L, -2); // Remove auth handler
	lua_remove(L, error_handler);

	// nil = login not allowed
	if (lua_isnil(L, -1))
		return false;
	luaL_checktype(L, -1, LUA_TTABLE);

	std::string password;
	bool found = getstringfield(L, -1, "password", password);
	if (!found)
		throw LuaError("Authentication handler didn't return password");
	if (dst_password)
		*dst_password = password;

	lua_getfield(L, -1, "privileges");
	if (!lua_istable(L, -1))
		throw LuaError("Authentication handler didn't return privilege table");
	if (dst_privs)
		readPrivileges(-1, *dst_privs);
	lua_pop(L, 1);

	return true;
}

void ScriptApiServer::getAuthHandler()
{
	lua_State *L = getStack();

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_auth_handler");
	if (lua_isnil(L, -1)){
		lua_pop(L, 1);
		lua_getfield(L, -1, "builtin_auth_handler");
	}

	setOriginFromTable(-1);

	lua_remove(L, -2); // Remove core
	if (lua_type(L, -1) != LUA_TTABLE)
		throw LuaError("Authentication handler table not valid");
}

void ScriptApiServer::readPrivileges(int index, std::set<std::string> &result)
{
	lua_State *L = getStack();

	result.clear();
	lua_pushnil(L);
	if (index < 0)
		index -= 1;
	while (lua_next(L, index) != 0) {
		// key at index -2 and value at index -1
		std::string key = luaL_checkstring(L, -2);
		bool value = readParam<bool>(L, -1);
		if (value)
			result.insert(key);
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

void ScriptApiServer::createAuth(const std::string &playername,
		const std::string &password)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);
	getAuthHandler();
	lua_getfield(L, -1, "create_auth");
	lua_remove(L, -2); // Remove auth handler
	if (lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError("Authentication handler missing create_auth");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	PCALL_RES(lua_pcall(L, 2, 0, error_handler));
	lua_pop(L, 1); // Pop error handler
}

bool ScriptApiServer::setPassword(const std::string &playername,
		const std::string &password)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);
	getAuthHandler();
	lua_getfield(L, -1, "set_password");
	lua_remove(L, -2); // Remove auth handler
	if (lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError("Authentication handler missing set_password");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	PCALL_RES(lua_pcall(L, 2, 1, error_handler));
	lua_remove(L, error_handler);
	return lua_toboolean(L, -1);
}

bool ScriptApiServer::on_chat_message(const std::string &name,
		const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_chat_messages");
	// Call callbacks
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, message.c_str());
	runCallbacks(2, RUN_CALLBACKS_MODE_OR_SC);
	return readParam<bool>(L, -1);
}

void ScriptApiServer::on_mods_loaded()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_mods_loaded");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiServer::on_shutdown()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

std::string ScriptApiServer::formatChatMessage(const std::string &name,
	const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push function onto stack
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "format_chat_message");

	// Push arguments onto stack
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, message.c_str());

	// Actually call the function
	lua_call(L, 2, 1);

	// Fetch return value
	std::string ret = lua_tostring(L, -1);
	lua_pop(L, 1);

	return ret;
}
