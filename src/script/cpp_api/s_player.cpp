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

#include "cpp_api/s_player.h"
#include "cpp_api/s_internal.h"
#include "util/string.h"

void ScriptApiPlayer::on_newplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_newplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_newplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiPlayer::on_dieplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_dieplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_dieplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

bool ScriptApiPlayer::on_respawnplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_respawnplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_respawnplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_OR);
	bool positioning_handled_by_some = lua_toboolean(L, -1);
	return positioning_handled_by_some;
}

bool ScriptApiPlayer::on_prejoinplayer(std::string name, std::string ip, std::string &reason)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_prejoinplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_prejoinplayers");
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, ip.c_str());
	script_run_callbacks(L, 2, RUN_CALLBACKS_MODE_OR);
	if (lua_isstring(L, -1)) {
		reason.assign(lua_tostring(L, -1));
		return true;
	}
	return false;
}

void ScriptApiPlayer::on_joinplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_joinplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_joinplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiPlayer::on_leaveplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_leaveplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_leaveplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	script_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiPlayer::on_cheat(ServerActiveObject *player,
		const std::string &cheat_type)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_cheats
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_cheats");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	lua_newtable(L);
	lua_pushlstring(L, cheat_type.c_str(), cheat_type.size());
	lua_setfield(L, -2, "type");
	script_run_callbacks(L, 2, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiPlayer::on_playerReceiveFields(ServerActiveObject *player,
		const std::string &formname,
		const std::map<std::string, std::string> &fields)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_player_receive_fields");
	// Call callbacks
	// param 1
	objectrefGetOrCreate(L, player);
	// param 2
	lua_pushstring(L, formname.c_str());
	// param 3
	lua_newtable(L);
	for(std::map<std::string, std::string>::const_iterator
			i = fields.begin(); i != fields.end(); i++){
		const std::string &name = i->first;
		const std::string &value = i->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}
	script_run_callbacks(L, 3, RUN_CALLBACKS_MODE_OR_SC);
}
ScriptApiPlayer::~ScriptApiPlayer() {
}


