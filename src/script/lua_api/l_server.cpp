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

#include "lua_api/l_server.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"
#include "environment.h"
#include "player.h"
#include "log.h"

// request_shutdown()
int ModApiServer::l_request_shutdown(lua_State *L)
{
	getServer(L)->requestShutdown();
	return 0;
}

// get_server_status()
int ModApiServer::l_get_server_status(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, wide_to_narrow(getServer(L)->getStatusString()).c_str());
	return 1;
}

// chat_send_all(text)
int ModApiServer::l_chat_send_all(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *text = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = getServer(L);
	// Send
	server->notifyPlayers(narrow_to_wide(text));
	return 0;
}

// chat_send_player(name, text)
int ModApiServer::l_chat_send_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	const char *text = luaL_checkstring(L, 2);

	// Get server from registry
	Server *server = getServer(L);
	// Send
	server->notifyPlayer(name, narrow_to_wide(text));
	return 0;
}

// get_player_privs(name, text)
int ModApiServer::l_get_player_privs(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = getServer(L);
	// Do it
	lua_newtable(L);
	int table = lua_gettop(L);
	std::set<std::string> privs_s = server->getPlayerEffectivePrivs(name);
	for(std::set<std::string>::const_iterator
			i = privs_s.begin(); i != privs_s.end(); i++){
		lua_pushboolean(L, true);
		lua_setfield(L, table, i->c_str());
	}
	lua_pushvalue(L, table);
	return 1;
}

// get_player_ip()
int ModApiServer::l_get_player_ip(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * name = luaL_checkstring(L, 1);
	Player *player = getEnv(L)->getPlayer(name);
	if(player == NULL)
	{
		lua_pushnil(L); // no such player
		return 1;
	}
	try
	{
		Address addr = getServer(L)->getPeerAddress(player->peer_id);
		std::string ip_str = addr.serializeString();
		lua_pushstring(L, ip_str.c_str());
		return 1;
	}
	catch(con::PeerNotFoundException) // unlikely
	{
		dstream << __FUNCTION_NAME << ": peer was not found" << std::endl;
		lua_pushnil(L); // error
		return 1;
	}
}

// get_player_information()
int ModApiServer::l_get_player_information(lua_State *L)
{

	NO_MAP_LOCK_REQUIRED;
	const char * name = luaL_checkstring(L, 1);
	Player *player = getEnv(L)->getPlayer(name);
	if(player == NULL)
	{
		lua_pushnil(L); // no such player
		return 1;
	}

	Address addr;
	try
	{
		addr = getServer(L)->getPeerAddress(player->peer_id);
	}
	catch(con::PeerNotFoundException) // unlikely
	{
		dstream << __FUNCTION_NAME << ": peer was not found" << std::endl;
		lua_pushnil(L); // error
		return 1;
	}

	float min_rtt,max_rtt,avg_rtt,min_jitter,max_jitter,avg_jitter;
	ClientState state;
	u32 uptime;
	u16 prot_vers;
	u8 ser_vers,major,minor,patch;
	std::string vers_string;

#define ERET(code)                                                             \
	if (!(code)) {                                                             \
		dstream << __FUNCTION_NAME << ": peer was not found" << std::endl;     \
		lua_pushnil(L); /* error */                                            \
		return 1;                                                              \
	}

	ERET(getServer(L)->getClientConInfo(player->peer_id,con::MIN_RTT,&min_rtt))
	ERET(getServer(L)->getClientConInfo(player->peer_id,con::MAX_RTT,&max_rtt))
	ERET(getServer(L)->getClientConInfo(player->peer_id,con::AVG_RTT,&avg_rtt))
	ERET(getServer(L)->getClientConInfo(player->peer_id,con::MIN_JITTER,&min_jitter))
	ERET(getServer(L)->getClientConInfo(player->peer_id,con::MAX_JITTER,&max_jitter))
	ERET(getServer(L)->getClientConInfo(player->peer_id,con::AVG_JITTER,&avg_jitter))

	ERET(getServer(L)->getClientInfo(player->peer_id,
										&state, &uptime, &ser_vers, &prot_vers,
										&major, &minor, &patch, &vers_string))

	lua_newtable(L);
	int table = lua_gettop(L);

	lua_pushstring(L,"address");
	lua_pushstring(L, addr.serializeString().c_str());
	lua_settable(L, table);

	lua_pushstring(L,"ip_version");
	if (addr.getFamily() == AF_INET) {
		lua_pushnumber(L, 4);
	} else if (addr.getFamily() == AF_INET6) {
		lua_pushnumber(L, 6);
	} else {
		lua_pushnumber(L, 0);
	}
	lua_settable(L, table);

	lua_pushstring(L,"min_rtt");
	lua_pushnumber(L, min_rtt);
	lua_settable(L, table);

	lua_pushstring(L,"max_rtt");
	lua_pushnumber(L, max_rtt);
	lua_settable(L, table);

	lua_pushstring(L,"avg_rtt");
	lua_pushnumber(L, avg_rtt);
	lua_settable(L, table);

	lua_pushstring(L,"min_jitter");
	lua_pushnumber(L, min_jitter);
	lua_settable(L, table);

	lua_pushstring(L,"max_jitter");
	lua_pushnumber(L, max_jitter);
	lua_settable(L, table);

	lua_pushstring(L,"avg_jitter");
	lua_pushnumber(L, avg_jitter);
	lua_settable(L, table);

	lua_pushstring(L,"connection_uptime");
	lua_pushnumber(L, uptime);
	lua_settable(L, table);

#ifndef NDEBUG
	lua_pushstring(L,"serialization_version");
	lua_pushnumber(L, ser_vers);
	lua_settable(L, table);

	lua_pushstring(L,"protocol_version");
	lua_pushnumber(L, prot_vers);
	lua_settable(L, table);

	lua_pushstring(L,"major");
	lua_pushnumber(L, major);
	lua_settable(L, table);

	lua_pushstring(L,"minor");
	lua_pushnumber(L, minor);
	lua_settable(L, table);

	lua_pushstring(L,"patch");
	lua_pushnumber(L, patch);
	lua_settable(L, table);

	lua_pushstring(L,"version_string");
	lua_pushstring(L, vers_string.c_str());
	lua_settable(L, table);

	lua_pushstring(L,"state");
	lua_pushstring(L,ClientInterface::state2Name(state).c_str());
	lua_settable(L, table);
#endif

#undef ERET
	return 1;
}

// get_ban_list()
int ModApiServer::l_get_ban_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, getServer(L)->getBanDescription("").c_str());
	return 1;
}

// get_ban_description()
int ModApiServer::l_get_ban_description(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * ip_or_name = luaL_checkstring(L, 1);
	lua_pushstring(L, getServer(L)->getBanDescription(std::string(ip_or_name)).c_str());
	return 1;
}

// ban_player()
int ModApiServer::l_ban_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * name = luaL_checkstring(L, 1);
	Player *player = getEnv(L)->getPlayer(name);
	if(player == NULL)
	{
		lua_pushboolean(L, false); // no such player
		return 1;
	}
	try
	{
		Address addr = getServer(L)->getPeerAddress(getEnv(L)->getPlayer(name)->peer_id);
		std::string ip_str = addr.serializeString();
		getServer(L)->setIpBanned(ip_str, name);
	}
	catch(con::PeerNotFoundException) // unlikely
	{
		dstream << __FUNCTION_NAME << ": peer was not found" << std::endl;
		lua_pushboolean(L, false); // error
		return 1;
	}
	lua_pushboolean(L, true);
	return 1;
}

// kick_player(name, [reason]) -> success
int ModApiServer::l_kick_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	std::string message;
	if (lua_isstring(L, 2))
	{
		message = std::string("Kicked: ") + lua_tostring(L, 2);
	}
	else
	{
		message = "Kicked.";
	}
	Player *player = getEnv(L)->getPlayer(name);
	if (player == NULL)
	{
		lua_pushboolean(L, false); // No such player
		return 1;
	}
	getServer(L)->DenyAccess(player->peer_id, narrow_to_wide(message));
	lua_pushboolean(L, true);
	return 1;
}

// unban_player_or_ip()
int ModApiServer::l_unban_player_or_ip(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * ip_or_name = luaL_checkstring(L, 1);
	getServer(L)->unsetIpBanned(ip_or_name);
	lua_pushboolean(L, true);
	return 1;
}

// show_formspec(playername,formname,formspec)
int ModApiServer::l_show_formspec(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *playername = luaL_checkstring(L, 1);
	const char *formname = luaL_checkstring(L, 2);
	const char *formspec = luaL_checkstring(L, 3);

	if(getServer(L)->showFormspec(playername,formspec,formname))
	{
		lua_pushboolean(L, true);
	}else{
		lua_pushboolean(L, false);
	}
	return 1;
}

// get_current_modname()
int ModApiServer::l_get_current_modname(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_getfield(L, LUA_REGISTRYINDEX, "current_modname");
	return 1;
}

// get_modpath(modname)
int ModApiServer::l_get_modpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string modname = luaL_checkstring(L, 1);
	const ModSpec *mod = getServer(L)->getModSpec(modname);
	if (!mod) {
		lua_pushnil(L);
		return 1;
	}
	lua_pushstring(L, mod->path.c_str());
	return 1;
}

// get_modnames()
// the returned list is sorted alphabetically for you
int ModApiServer::l_get_modnames(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	// Get a list of mods
	std::list<std::string> mods_unsorted, mods_sorted;
	getServer(L)->getModNames(mods_unsorted);

	// Take unsorted items from mods_unsorted and sort them into
	// mods_sorted; not great performance but the number of mods on a
	// server will likely be small.
	for(std::list<std::string>::iterator i = mods_unsorted.begin();
			i != mods_unsorted.end(); ++i) {
		bool added = false;
		for(std::list<std::string>::iterator x = mods_sorted.begin();
				x != mods_sorted.end(); ++x) {
			// I doubt anybody using Minetest will be using
			// anything not ASCII based :)
			if(i->compare(*x) <= 0) {
				mods_sorted.insert(x, *i);
				added = true;
				break;
			}
		}
		if(!added)
			mods_sorted.push_back(*i);
	}

	// Package them up for Lua
	lua_createtable(L, mods_sorted.size(), 0);
	std::list<std::string>::iterator iter = mods_sorted.begin();
	for (u16 i = 0; iter != mods_sorted.end(); iter++) {
		lua_pushstring(L, iter->c_str());
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// get_worldpath()
int ModApiServer::l_get_worldpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string worldpath = getServer(L)->getWorldPath();
	lua_pushstring(L, worldpath.c_str());
	return 1;
}

// sound_play(spec, parameters)
int ModApiServer::l_sound_play(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	ServerSoundParams params;
	read_server_sound_params(L, 2, params);
	s32 handle = getServer(L)->playSound(spec, params);
	lua_pushinteger(L, handle);
	return 1;
}

// sound_stop(handle)
int ModApiServer::l_sound_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	int handle = luaL_checkinteger(L, 1);
	getServer(L)->stopSound(handle);
	return 0;
}

// is_singleplayer()
int ModApiServer::l_is_singleplayer(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, getServer(L)->isSingleplayer());
	return 1;
}

// notify_authentication_modified(name)
int ModApiServer::l_notify_authentication_modified(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = "";
	if(lua_isstring(L, 1))
		name = lua_tostring(L, 1);
	getServer(L)->reportPrivsModified(name);
	return 0;
}

#ifndef NDEBUG
// cause_error(type_of_error)
int ModApiServer::l_cause_error(lua_State *L)
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

void ModApiServer::Initialize(lua_State *L, int top)
{
	API_FCT(request_shutdown);
	API_FCT(get_server_status);
	API_FCT(get_worldpath);
	API_FCT(is_singleplayer);

	API_FCT(get_current_modname);
	API_FCT(get_modpath);
	API_FCT(get_modnames);

	API_FCT(chat_send_all);
	API_FCT(chat_send_player);
	API_FCT(show_formspec);
	API_FCT(sound_play);
	API_FCT(sound_stop);

	API_FCT(get_player_information);
	API_FCT(get_player_privs);
	API_FCT(get_player_ip);
	API_FCT(get_ban_list);
	API_FCT(get_ban_description);
	API_FCT(ban_player);
	API_FCT(kick_player);
	API_FCT(unban_player_or_ip);
	API_FCT(notify_authentication_modified);

#ifndef NDEBUG
	API_FCT(cause_error);
#endif
}
