/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "s_client.h"
#include "s_internal.h"
#include "client/client.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "s_item.h"
#include "client/keycode.h"
#include "script/lua_api/l_drawer.h"

void ScriptApiClient::on_mods_loaded()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_mods_loaded");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiClient::on_media_loaded()
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_media_loaded");

	ModApiDrawer::start_callback(true);
	try {
		runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(std::string("Client on_media_loaded: ") + e.what() +
			"\n" + script_get_backtrace(L));
	}
	ModApiDrawer::end_callback(true);
}

void ScriptApiClient::on_shutdown()
{
	SCRIPTAPI_PRECHECKHEADER

	// Shut down drawing
	ModApiDrawer::reset();

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

bool ScriptApiClient::on_sending_message(const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_sending_chat_message");
	// Call callbacks
	lua_pushstring(L, message.c_str());
	runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_receiving_message(const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_receiving_chat_message");
	// Call callbacks
	lua_pushstring(L, message.c_str());
	runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	return readParam<bool>(L, -1);
}

void ScriptApiClient::on_damage_taken(int32_t damage_amount)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_damage_taken");
	// Call callbacks
	lua_pushinteger(L, damage_amount);
	runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
}

void ScriptApiClient::on_hp_modification(int32_t newhp)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_hp_modification");
	// Call callbacks
	lua_pushinteger(L, newhp);
	runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
}

void ScriptApiClient::on_death()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_death");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiClient::environment_step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(std::string("Client environment_step: ") + e.what() + "\n"
				+ script_get_backtrace(L));
	}
}

void ScriptApiClient::on_predraw(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_predraw");

	lua_pushnumber(L, dtime);

	ModApiDrawer::start_callback(false);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(std::string("Client on_predraw: ") + e.what() + "\n" +
			script_get_backtrace(L));
	}
	ModApiDrawer::end_callback(false);
}

void ScriptApiClient::on_draw(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_draw");

	lua_pushnumber(L, dtime);

	ModApiDrawer::start_callback(false);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(std::string("Client on_draw: ") + e.what() + "\n" +
			script_get_backtrace(L));
	}
	ModApiDrawer::end_callback(false);
}

void ScriptApiClient::on_event(const SEvent &event)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_event");

	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		const SEvent::SMouseInput &mouse = event.MouseInput;

		lua_createtable(L, 0, 3);

		// Event type
		lua_pushstring(L, "mouse");
		lua_setfield(L, -2, "event");

		// Mouse event descriptions
		struct EventDesc {
			const char *type;
			const char *button;
			int clicks;
		};
		// Has same order as EMOUSE_INPUT_EVENT
		const EventDesc descs[EMIE_COUNT] = {
			{"click", "left", 1},
			{"click", "right", 1},
			{"click", "middle", 1},
			{"release", "left", 0},
			{"release", "right", 0},
			{"release", "middle", 0},
			{"move", nullptr, 0},
			{"scroll", nullptr, 0},
			{"click", "left", 2},
			{"click", "right", 2},
			{"click", "middle", 2},
			{"click", "left", 3},
			{"click", "right", 3},
			{"click", "middle", 3}
		};
		const EventDesc &desc = descs[mouse.Event];

		// Mouse event type
		lua_pushstring(L, desc.type);
		lua_setfield(L, -2, "type");

		// If applicable, the mouse button that was clicked/released
		if (desc.button != nullptr) {
			lua_pushstring(L, desc.button);
			lua_setfield(L, -2, "button");
		}

		// If applicable, how many times the mouse was clicked (like doubleclick)
		if (desc.clicks != 0) {
			lua_pushinteger(L, desc.clicks);
			lua_setfield(L, -2, "clicks");
		}

		// Mouse position, if applicable
		if (mouse.Event == EMIE_MOUSE_MOVED) {
			push_v2s32(L, v2s32(mouse.X, mouse.Y));
			lua_setfield(L, -2, "pos");
		}

		// Mouse wheel, if applicable
		if (mouse.Event == EMIE_MOUSE_WHEEL) {
			lua_pushnumber(L, mouse.Wheel);
			lua_setfield(L, -2, "scroll");
		}
	} else if (event.EventType == EET_KEY_INPUT_EVENT) {
		const SEvent::SKeyInput &key = event.KeyInput;
		KeyPress key_press(key);

		lua_createtable(L, 0, 3);

		// Event type
		lua_pushstring(L, "key");
		lua_setfield(L, -2, "event");

		// Whether the key was pressed, released, or repeated (held pressed down)
		if (key.PressedDown) {
			if (key.Shift) // Shift means the key is repeating in this case
				lua_pushstring(L, "repeat");
			else
				lua_pushstring(L, "press");
		} else {
			lua_pushstring(L, "release");
		}
		lua_setfield(L, -2, "type");

		// Name of the key
		lua_pushstring(L, key_press.sym());
		lua_setfield(L, -2, "keycode");

		// Keymap setting name, if applicable
		for (const auto keymap_it : g_settings->getKeymapNames()) {
			if (key_press == getKeySetting(keymap_it.c_str())) {
				lua_pushstring(L, keymap_it.substr(7).c_str());
				lua_setfield(L, -2, "keymap");
				break;
			}
		}

		// Character assigned to the key, if applicable
		if (key.Char) {
			std::string char_str = wide_to_utf8(std::wstring(1, key.Char));
			// Sometimes `wide_to_utf8` fails for some characters, so check for this
			if (char_str != "<invalid wstring>") {
				lua_pushlstring(L, char_str.c_str(), char_str.size());
				lua_setfield(L, -2, "char");
			}
		}
	}

	// Call callback; stop event propagation on the first returned `true`.
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	} catch (LuaError &e) {
		getClient()->setFatalError(std::string("Client on_event: ") + e.what() + "\n" +
			script_get_backtrace(L));
	}
}

void ScriptApiClient::on_formspec_input(const std::string &formname,
	const StringMap &fields)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_formspec_input");
	// Call callbacks
	// param 1
	lua_pushstring(L, formname.c_str());
	// param 2
	lua_newtable(L);
	StringMap::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it) {
		const std::string &name = it->first;
		const std::string &value = it->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}
	runCallbacks(2, RUN_CALLBACKS_MODE_OR_SC);
}

bool ScriptApiClient::on_dignode(v3s16 p, MapNode node)
{
	SCRIPTAPI_PRECHECKHEADER

	const NodeDefManager *ndef = getClient()->ndef();

	// Get core.registered_on_dignode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_dignode");

	// Push data
	push_v3s16(L, p);
	pushnode(L, node, ndef);

	// Call functions
	runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	return lua_toboolean(L, -1);
}

bool ScriptApiClient::on_punchnode(v3s16 p, MapNode node)
{
	SCRIPTAPI_PRECHECKHEADER

	const NodeDefManager *ndef = getClient()->ndef();

	// Get core.registered_on_punchgnode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_punchnode");

	// Push data
	push_v3s16(L, p);
	pushnode(L, node, ndef);

	// Call functions
	runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_placenode(const PointedThing &pointed, const ItemDefinition &item)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_placenode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_placenode");

	// Push data
	push_pointed_thing(L, pointed, true);
	push_item_definition(L, item);

	// Call functions
	runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_item_use(const ItemStack &item, const PointedThing &pointed)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_item_use
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_item_use");

	// Push data
	LuaItemStack::create(L, item);
	push_pointed_thing(L, pointed, true);

	// Call functions
	runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_inventory_open(Inventory *inventory)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_inventory_open");

	std::vector<const InventoryList*> lists = inventory->getLists();
	std::vector<const InventoryList*>::iterator iter = lists.begin();
	lua_createtable(L, 0, lists.size());
	for (; iter != lists.end(); iter++) {
		const char* name = (*iter)->getName().c_str();
		lua_pushstring(L, name);
		push_inventory_list(L, inventory, name);
		lua_rawset(L, -3);
	}

	runCallbacks(1, RUN_CALLBACKS_MODE_OR);
	return readParam<bool>(L, -1);
}

void ScriptApiClient::setEnv(ClientEnvironment *env)
{
	ScriptApiBase::setEnv(env);
}
