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

#include "cpp_api/s_mainmenu.h"
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"

void ScriptApiMainMenu::setMainMenuErrorMessage(std::string errormessage)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "gamedata");
	int gamedata_idx = lua_gettop(L);
	lua_pushstring(L, "errormessage");
	lua_pushstring(L, errormessage.c_str());
	lua_settable(L, gamedata_idx);
	lua_pop(L, 1);
}

void ScriptApiMainMenu::handleMainMenuEvent(std::string text)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get handler function
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "event_handler");
	lua_remove(L, -2); // Remove core
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1); // Pop event_handler
		return;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);

	// Call it
	lua_pushstring(L, text.c_str());
	if (lua_pcall(L, 1, 0, m_errorhandler))
		scriptError();
}

void ScriptApiMainMenu::handleMainMenuButtons(std::map<std::string, std::string> fields)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get handler function
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "button_handler");
	lua_remove(L, -2); // Remove core
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1); // Pop button handler
		return;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);

	// Convert fields to a Lua table
	lua_newtable(L);
	std::map<std::string, std::string>::const_iterator it;
	for (it = fields.begin(); it != fields.end(); it++){
		const std::string &name = it->first;
		const std::string &value = it->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}

	// Call it
	if (lua_pcall(L, 1, 0, m_errorhandler))
		scriptError();
}

