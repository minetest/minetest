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

#include "lua_api/l_client.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "cpp_api/s_base.h"
#include "client.h"
#include "environment.h"
#include "player.h"
#include "log.h"

// get_client_status()
int ModApiClient::l_get_client_status(lua_State *L)
{
//	NO_MAP_LOCK_REQUIRED;
//	lua_pushstring(L, wide_to_narrow(getClient(L)->getStatusString()).c_str());
	return 1;
}

// print(text)
int ModApiClient::l_print(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string text;
	text = luaL_checkstring(L, 1);
	std::cout << text << std::endl;
	return 0;
}

// get_player_privs(name, text)
int ModApiClient::l_get_player_privs(lua_State *L)
{
//	NO_MAP_LOCK_REQUIRED;
//	const char *name = luaL_checkstring(L, 1);
//	// Get client from registry
//	Client *client = getClient(L);
//	// Do it
//	lua_newtable(L);
//	int table = lua_gettop(L);
//	std::set<std::string> privs_s = client->getPlayerEffectivePrivs(name);
//	for(std::set<std::string>::const_iterator
//			i = privs_s.begin(); i != privs_s.end(); i++){
//		lua_pushboolean(L, true);
//		lua_setfield(L, table, i->c_str());
//	}
//	lua_pushvalue(L, table);
	return 1;
}

// get_player_information()
int ModApiClient::l_get_player_information(lua_State *L)
{
//
//	NO_MAP_LOCK_REQUIRED;
//	const char * name = luaL_checkstring(L, 1);
//	Player *player = getEnv(L)->getPlayer(name);
//	if(player == NULL)
//	{
//		lua_pushnil(L); // no such player
//		return 1;
//	}
//
//	Address addr;
//	try
//	{
//		addr = getClient(L)->getPeerAddress(player->peer_id);
//	}
//	catch(con::PeerNotFoundException) // unlikely
//	{
//		dstream << FUNCTION_NAME << ": peer was not found" << std::endl;
//		lua_pushnil(L); // error
//		return 1;
//	}
//
//	float min_rtt,max_rtt,avg_rtt,min_jitter,max_jitter,avg_jitter;
//	ClientState state;
//	u32 uptime;
//	u16 prot_vers;
//	u8 ser_vers,major,minor,patch;
//	std::string vers_string;
//
//#define ERET(code)                                                             \
//	if (!(code)) {                                                             \
//		dstream << FUNCTION_NAME << ": peer was not found" << std::endl;     \
//		lua_pushnil(L); /* error */                                            \
//		return 1;                                                              \
//	}
//
//	ERET(getClient(L)->getClientConInfo(player->peer_id,con::MIN_RTT,&min_rtt))
//	ERET(getClient(L)->getClientConInfo(player->peer_id,con::MAX_RTT,&max_rtt))
//	ERET(getClient(L)->getClientConInfo(player->peer_id,con::AVG_RTT,&avg_rtt))
//	ERET(getClient(L)->getClientConInfo(player->peer_id,con::MIN_JITTER,&min_jitter))
//	ERET(getClient(L)->getClientConInfo(player->peer_id,con::MAX_JITTER,&max_jitter))
//	ERET(getClient(L)->getClientConInfo(player->peer_id,con::AVG_JITTER,&avg_jitter))
//
//	ERET(getClient(L)->getClientInfo(player->peer_id,
//										&state, &uptime, &ser_vers, &prot_vers,
//										&major, &minor, &patch, &vers_string))
//
//	lua_newtable(L);
//	int table = lua_gettop(L);
//
//	lua_pushstring(L,"address");
//	lua_pushstring(L, addr.serializeString().c_str());
//	lua_settable(L, table);
//
//	lua_pushstring(L,"ip_version");
//	if (addr.getFamily() == AF_INET) {
//		lua_pushnumber(L, 4);
//	} else if (addr.getFamily() == AF_INET6) {
//		lua_pushnumber(L, 6);
//	} else {
//		lua_pushnumber(L, 0);
//	}
//	lua_settable(L, table);
//
//	lua_pushstring(L,"min_rtt");
//	lua_pushnumber(L, min_rtt);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"max_rtt");
//	lua_pushnumber(L, max_rtt);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"avg_rtt");
//	lua_pushnumber(L, avg_rtt);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"min_jitter");
//	lua_pushnumber(L, min_jitter);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"max_jitter");
//	lua_pushnumber(L, max_jitter);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"avg_jitter");
//	lua_pushnumber(L, avg_jitter);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"connection_uptime");
//	lua_pushnumber(L, uptime);
//	lua_settable(L, table);
//
//#ifndef NDEBUG
//	lua_pushstring(L,"serialization_version");
//	lua_pushnumber(L, ser_vers);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"protocol_version");
//	lua_pushnumber(L, prot_vers);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"major");
//	lua_pushnumber(L, major);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"minor");
//	lua_pushnumber(L, minor);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"patch");
//	lua_pushnumber(L, patch);
//	lua_settable(L, table);
//
//	lua_pushstring(L,"version_string");
//	lua_pushstring(L, vers_string.c_str());
//	lua_settable(L, table);
//
//	lua_pushstring(L,"state");
//	lua_pushstring(L,ClientInterface::state2Name(state).c_str());
//	lua_settable(L, table);
//#endif
//
//#undef ERET
	return 1;
}

// show_formspec(playername,formname,formspec)
int ModApiClient::l_show_formspec(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *playername = luaL_checkstring(L, 1);
	const char *formname = luaL_checkstring(L, 2);
	const char *formspec = luaL_checkstring(L, 3);
	Client *client = getClient(L);
	bool ok = false;
	
	if (client->getPlayerName() == playername) {
		//TODO: Show the formspec and set ok
	}
	lua_pushboolean(L, ok);
	return 1;
}

// get_current_modname()
int ModApiClient::l_get_current_modname(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	return 1;
}

// get_modpath(modname)
int ModApiClient::l_get_modpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string modname = luaL_checkstring(L, 1);
	lua_pushstring(L, modname.c_str());
	return 1;
}

// get_modnames()
// the returned list is sorted alphabetically for you
int ModApiClient::l_get_modnames(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	// Get a list of mods
	std::vector<std::string> modlist;
	getClient(L)->getScripting()->getModNames(modlist);

	// Take unsorted items from mods_unsorted and sort them into
	// mods_sorted; not great performance but the number of mods on a
	// client will likely be small.
	std::sort(modlist.begin(), modlist.end());

	// Package them up for Lua
	lua_createtable(L, modlist.size(), 0);
	std::vector<std::string>::iterator iter = modlist.begin();
	for (u16 i = 0; iter != modlist.end(); iter++) {
		lua_pushstring(L, iter->c_str());
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// sound_play(spec, parameters)
int ModApiClient::l_sound_play(lua_State *L)
{
//	NO_MAP_LOCK_REQUIRED;
//	SimpleSoundSpec spec;
//	read_soundspec(L, 1, spec);
//	ClientSoundParams params;
//	read_client_sound_params(L, 2, params);
//	s32 handle = getClient(L)->playSound(spec, params);
//	lua_pushinteger(L, handle);
	return 1;
}

// sound_stop(handle)
int ModApiClient::l_sound_stop(lua_State *L)
{
//	NO_MAP_LOCK_REQUIRED;
//	int handle = luaL_checkinteger(L, 1);
//	getClient(L)->stopSound(handle);
	return 0;
}

// is_singleplayer()
int ModApiClient::l_is_singleplayer(lua_State *L)
{
//	NO_MAP_LOCK_REQUIRED;
//	lua_pushboolean(L, getClient(L)->isSingleplayer());
	return 1;
}

#ifndef NDEBUG
// cause_error(type_of_error)
int ModApiClient::l_cause_error(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string type_of_error = "none";
	if(lua_isstring(L, 1))
		type_of_error = lua_tostring(L, 1);

	errorstream << "Error handler test called, errortype=" << type_of_error << std::endl;

	if(type_of_error == "segv") {
		volatile int* some_pointer = 0;
		errorstream << "Cause a sigsegv now: " << (*some_pointer) << std::endl;

	} else if (type_of_error == "zerodivision") {

		unsigned int some_number = porting::getTimeS();
		unsigned int zerovalue = 0;
		unsigned int result = some_number / zerovalue;
		errorstream << "Well this shouldn't ever be shown: " << result << std::endl;

	} else if (type_of_error == "exception") {
		throw BaseException("Errorhandler test fct called");
	}

	return 0;
}
#endif

void ModApiClient::Initialize(lua_State *L, int top)
{
	API_FCT(get_client_status);
	API_FCT(is_singleplayer);

	API_FCT(get_current_modname);
	API_FCT(get_modpath);
	API_FCT(get_modnames);

	API_FCT(print);

	API_FCT(show_formspec);
	API_FCT(sound_play);
	API_FCT(sound_stop);

	API_FCT(get_player_information);
	API_FCT(get_player_privs);
#ifndef NDEBUG
	API_FCT(cause_error);
#endif
}
