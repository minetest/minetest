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

#include "l_client.h"
#include "clientenvironment.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "cpp_api/s_base.h"
#include "gettext.h"
#include "l_internal.h"
#include "lua_api/l_item.h"
#include "lua_api/l_nodemeta.h"
#include "mainmenumanager.h"
#include "map.h"
#include "util/string.h"

extern MainGameCallback *g_gamecallback;

int ModApiClient::l_get_current_modname(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	return 1;
}

// get_last_run_mod()
int ModApiClient::l_get_last_run_mod(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	const char *current_mod = lua_tostring(L, -1);
	if (current_mod == NULL || current_mod[0] == '\0') {
		lua_pop(L, 1);
		lua_pushstring(L, getScriptApiBase(L)->getOrigin().c_str());
	}
	return 1;
}

// set_last_run_mod(modname)
int ModApiClient::l_set_last_run_mod(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	const char *mod = lua_tostring(L, 1);
	getScriptApiBase(L)->setOriginDirect(mod);
	lua_pushboolean(L, true);
	return 1;
}

// display_chat_message(message)
int ModApiClient::l_display_chat_message(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	std::string message = luaL_checkstring(L, 1);
	getClient(L)->pushToChatQueue(utf8_to_wide(message));
	lua_pushboolean(L, true);
	return 1;
}

// get_player_names()
int ModApiClient::l_get_player_names(lua_State *L)
{
	const std::list<std::string> &plist = getClient(L)->getConnectedPlayerNames();
	lua_createtable(L, plist.size(), 0);
	int newTable = lua_gettop(L);
	int index = 1;
	std::list<std::string>::const_iterator iter;
	for (iter = plist.begin(); iter != plist.end(); iter++) {
		lua_pushstring(L, (*iter).c_str());
		lua_rawseti(L, newTable, index);
		index++;
	}
	return 1;
}

// show_formspec(formspec)
int ModApiClient::l_show_formspec(lua_State *L)
{
	if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
		return 0;

	ClientEvent event;
	event.type = CE_SHOW_LOCAL_FORMSPEC;
	event.show_formspec.formname = new std::string(luaL_checkstring(L, 1));
	event.show_formspec.formspec = new std::string(luaL_checkstring(L, 2));
	getClient(L)->pushToEventQueue(event);
	lua_pushboolean(L, true);
	return 1;
}

// send_respawn()
int ModApiClient::l_send_respawn(lua_State *L)
{
	getClient(L)->sendRespawn();
	return 0;
}

// disconnect()
int ModApiClient::l_disconnect(lua_State *L)
{
	// Stops badly written Lua code form causing boot loops
	if (getClient(L)->isShutdown()) {
		lua_pushboolean(L, false);
		return 1;
	}

	g_gamecallback->disconnect();
	lua_pushboolean(L, true);
	return 1;
}

// gettext(text)
int ModApiClient::l_gettext(lua_State *L)
{
	std::string text = strgettext(std::string(luaL_checkstring(L, 1)));
	lua_pushstring(L, text.c_str());

	return 1;
}

// get_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiClient::l_get_node(lua_State *L)
{
	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool pos_ok;
	MapNode n = getClient(L)->getNode(pos, &pos_ok);
	// Return node
	pushnode(L, n, getClient(L)->ndef());
	return 1;
}

// get_node_or_nil(pos)
// pos = {x=num, y=num, z=num}
int ModApiClient::l_get_node_or_nil(lua_State *L)
{
	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool pos_ok;
	MapNode n = getClient(L)->getNode(pos, &pos_ok);
	if (pos_ok) {
		// Return node
		pushnode(L, n, getClient(L)->ndef());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int ModApiClient::l_get_wielded_item(lua_State *L)
{
	Client *client = getClient(L);

	Inventory local_inventory(client->idef());
	client->getLocalInventory(local_inventory);

	InventoryList *mlist = local_inventory.getList("main");

	if (mlist && client->getPlayerItem() < mlist->getSize()) {
		LuaItemStack::create(L, mlist->getItem(client->getPlayerItem()));
	} else {
		LuaItemStack::create(L, ItemStack());
	}
	return 1;
}

// get_meta(pos)
int ModApiClient::l_get_meta(lua_State *L)
{
	v3s16 p = read_v3s16(L, 1);
	NodeMetadata *meta = getClient(L)->getEnv().getMap().getNodeMetadata(p);
	NodeMetaRef::createClient(L, meta);
	return 1;
}

int ModApiClient::l_sound_play(lua_State *L)
{
	ISoundManager *sound = getClient(L)->getSoundManager();

	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	float gain = 1.0;
	bool looped = false;
	s32 handle;

	if (lua_istable(L, 2)) {
		getfloatfield(L, 2, "gain", gain);
		getboolfield(L, 2, "loop", looped);

		lua_getfield(L, 2, "pos");
		if (!lua_isnil(L, -1)) {
			v3f pos = read_v3f(L, -1) * BS;
			lua_pop(L, 1);
			handle = sound->playSoundAt(
					spec.name, looped, gain * spec.gain, pos);
			lua_pushinteger(L, handle);
			return 1;
		}
	}

	handle = sound->playSound(spec.name, looped, gain * spec.gain);
	lua_pushinteger(L, handle);

	return 1;
}

int ModApiClient::l_sound_stop(lua_State *L)
{
	u32 handle = luaL_checkinteger(L, 1);

	getClient(L)->getSoundManager()->stopSound(handle);

	return 0;
}

// get_protocol_version()
int ModApiClient::l_get_protocol_version(lua_State *L)
{
	lua_pushinteger(L, getClient(L)->getProtoVersion());
	return 1;
}

void ModApiClient::Initialize(lua_State *L, int top)
{
	API_FCT(get_current_modname);
	API_FCT(display_chat_message);
	API_FCT(get_player_names);
	API_FCT(set_last_run_mod);
	API_FCT(get_last_run_mod);
	API_FCT(show_formspec);
	API_FCT(send_respawn);
	API_FCT(gettext);
	API_FCT(get_node);
	API_FCT(get_node_or_nil);
	API_FCT(get_wielded_item);
	API_FCT(disconnect);
	API_FCT(get_meta);
	API_FCT(sound_play);
	API_FCT(sound_stop);
	API_FCT(get_protocol_version);
}
