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
#include "common/c_content.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_csm_localplayer.h"
#include "lua_api/l_csm_camera.h"
#include "lua_api/l_csm_minimap.h"
#include "lua_api/l_item.h"

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

bool ScriptApiCSM::on_formspec_input(const std::string &formname, const StringMap &fields)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_formspec_input
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_formspec_input");
	// Call callbacks
	lua_pushstring(L, formname.c_str());
	lua_createtable(L, 0, fields.size());
	for (const auto &pair : fields) {
		lua_pushlstring(L, pair.first.data(), pair.first.size());
		lua_pushlstring(L, pair.second.data(), pair.second.size());
		lua_settable(L, -3);
	}
	runCallbacks(2, RUN_CALLBACKS_MODE_OR_SC);
	return lua_toboolean(L, -1);
}

bool ScriptApiCSM::on_inventory_open(const Inventory *inventory)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_inventory_open
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_inventory_open");
	// Call callbacks
	push_inventory_lists(L, *inventory);
	runCallbacks(1, RUN_CALLBACKS_MODE_OR);
	return lua_toboolean(L, -1);
}

bool ScriptApiCSM::on_item_use(const ItemStack &selected_item, const PointedThing &pointed)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_item_use
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_item_use");
	// Call callbacks
	LuaItemStack::create(L, selected_item);
	push_pointed_thing(L, pointed, true);
	runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	return lua_toboolean(L, -1);
}

void ScriptApiCSM::on_placenode(const PointedThing &pointed, const ItemDefinition &item)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_placenode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_placenode");
	// Call callbacks
	push_pointed_thing(L, pointed, true);
	push_item_definition(L, item);
	runCallbacks(2, RUN_CALLBACKS_MODE_FIRST);
}

bool ScriptApiCSM::on_hp_modification(u16 hp)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_hp_modification
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_hp_modification");
	// Call callbacks
	lua_pushinteger(L, hp);
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
