/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "cpp_api/s_csm.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_csm_localplayer.h"
#include "lua_api/l_csm_camera.h"
#include "lua_api/l_csm_minimap.h"

void ScriptApiCSM::on_mods_loaded()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_mods_loaded
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_mods_loaded");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiCSM::on_shutdown()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_shutdown
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiCSM::on_client_ready()
{
	SCRIPTAPI_PRECHECKHEADER

	LuaCSMLocalPlayer::create(L);
}

void ScriptApiCSM::on_camera_ready()
{
	SCRIPTAPI_PRECHECKHEADER

	LuaCSMCamera::create(L);
}

void ScriptApiCSM::on_minimap_ready()
{
	SCRIPTAPI_PRECHECKHEADER

	LuaCSMMinimap::create(L);
}

bool ScriptApiCSM::on_sending_message(const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_sending_chat_message
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_sending_chat_message");
	// Call callbacks
	lua_pushlstring(L, message.data(), message.size());
	runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	return lua_toboolean(L, -1);
}

bool ScriptApiCSM::on_receiving_message(const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_receiving_chat_message
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_receiving_chat_message");
	// Call callbacks
	lua_pushlstring(L, message.data(), message.size());
	runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	return lua_toboolean(L, -1);
}

void ScriptApiCSM::environment_Step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
}
