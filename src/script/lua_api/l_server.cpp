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
#include "cpp_api/s_base.h"
#include "server.h"
#include "environment.h"
#include "remoteplayer.h"
#include "log.h"
#include <algorithm>

// request_shutdown()
int ModApiServer::l_request_shutdown(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *msg = lua_tolstring(L, 1, NULL);
	bool reconnect = readParam<bool>(L, 2);
	float seconds_before_shutdown = lua_tonumber(L, 3);
	getServer(L)->requestShutdown(msg ? msg : "", reconnect, seconds_before_shutdown);
	return 0;
}

// get_server_status()
int ModApiServer::l_get_server_status(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, wide_to_narrow(getServer(L)->getStatusString()).c_str());
	return 1;
}

// get_server_uptime()
int ModApiServer::l_get_server_uptime(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushnumber(L, getServer(L)->getUptime());
	return 1;
}


// print(text)
int ModApiServer::l_print(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string text;
	text = luaL_checkstring(L, 1);
	getServer(L)->printToConsoleOnly(text);
	return 0;
}

// chat_send_all(text)
int ModApiServer::l_chat_send_all(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *text = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = getServer(L);
	// Send
	server->notifyPlayers(utf8_to_wide(text));
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
	server->notifyPlayer(name, utf8_to_wide(text));
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
	for (const std::string &privs_ : privs_s) {
		lua_pushboolean(L, true);
		lua_setfield(L, table, privs_.c_str());
	}
	lua_pushvalue(L, table);
	return 1;
}

// get_player_ip()
int ModApiServer::l_get_player_ip(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * name = luaL_checkstring(L, 1);
	RemotePlayer *player = dynamic_cast<ServerEnvironment *>(getEnv(L))->getPlayer(name);
	if(player == NULL)
	{
		lua_pushnil(L); // no such player
		return 1;
	}
	try
	{
		Address addr = getServer(L)->getPeerAddress(player->getPeerId());
		std::string ip_str = addr.serializeString();
		lua_pushstring(L, ip_str.c_str());
		return 1;
	} catch (const con::PeerNotFoundException &) {
		dstream << FUNCTION_NAME << ": peer was not found" << std::endl;
		lua_pushnil(L); // error
		return 1;
	}
}

// get_player_information(name)
int ModApiServer::l_get_player_information(lua_State *L)
{

	NO_MAP_LOCK_REQUIRED;
	const char * name = luaL_checkstring(L, 1);
	RemotePlayer *player = dynamic_cast<ServerEnvironment *>(getEnv(L))->getPlayer(name);
	if (player == NULL) {
		lua_pushnil(L); // no such player
		return 1;
	}

	Address addr;
	try
	{
		addr = getServer(L)->getPeerAddress(player->getPeerId());
	} catch(const con::PeerNotFoundException &) {
		dstream << FUNCTION_NAME << ": peer was not found" << std::endl;
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
		dstream << FUNCTION_NAME << ": peer was not found" << std::endl;     \
		lua_pushnil(L); /* error */                                            \
		return 1;                                                              \
	}

	ERET(getServer(L)->getClientConInfo(player->getPeerId(), con::MIN_RTT, &min_rtt))
	ERET(getServer(L)->getClientConInfo(player->getPeerId(), con::MAX_RTT, &max_rtt))
	ERET(getServer(L)->getClientConInfo(player->getPeerId(), con::AVG_RTT, &avg_rtt))
	ERET(getServer(L)->getClientConInfo(player->getPeerId(), con::MIN_JITTER,
		&min_jitter))
	ERET(getServer(L)->getClientConInfo(player->getPeerId(), con::MAX_JITTER,
		&max_jitter))
	ERET(getServer(L)->getClientConInfo(player->getPeerId(), con::AVG_JITTER,
		&avg_jitter))

	ERET(getServer(L)->getClientInfo(player->getPeerId(), &state, &uptime, &ser_vers,
		&prot_vers, &major, &minor, &patch, &vers_string))

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

	lua_pushstring(L,"protocol_version");
	lua_pushnumber(L, prot_vers);
	lua_settable(L, table);

	lua_pushstring(L, "formspec_version");
	lua_pushnumber(L, player->formspec_version);
	lua_settable(L, table);

#ifndef NDEBUG
	lua_pushstring(L,"serialization_version");
	lua_pushnumber(L, ser_vers);
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
	RemotePlayer *player = dynamic_cast<ServerEnvironment *>(getEnv(L))->getPlayer(name);
	if (player == NULL) {
		lua_pushboolean(L, false); // no such player
		return 1;
	}
	try
	{
		Address addr = getServer(L)->getPeerAddress(
			dynamic_cast<ServerEnvironment *>(getEnv(L))->getPlayer(name)->getPeerId());
		std::string ip_str = addr.serializeString();
		getServer(L)->setIpBanned(ip_str, name);
	} catch(const con::PeerNotFoundException &) {
		dstream << FUNCTION_NAME << ": peer was not found" << std::endl;
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
	std::string message("Kicked");
	if (lua_isstring(L, 2))
		message.append(": ").append(readParam<std::string>(L, 2));
	else
		message.append(".");

	RemotePlayer *player = dynamic_cast<ServerEnvironment *>(getEnv(L))->getPlayer(name);
	if (player == NULL) {
		lua_pushboolean(L, false); // No such player
		return 1;
	}
	getServer(L)->DenyAccess_Legacy(player->getPeerId(), utf8_to_wide(message));
	lua_pushboolean(L, true);
	return 1;
}

int ModApiServer::l_remove_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	ServerEnvironment *s_env = dynamic_cast<ServerEnvironment *>(getEnv(L));
	assert(s_env);

	RemotePlayer *player = s_env->getPlayer(name.c_str());
	if (!player)
		lua_pushinteger(L, s_env->removePlayerFromDatabase(name) ? 0 : 1);
	else
		lua_pushinteger(L, 2);

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
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
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
	std::vector<std::string> modlist;
	getServer(L)->getModNames(modlist);

	// Take unsorted items from mods_unsorted and sort them into
	// mods_sorted; not great performance but the number of mods on a
	// server will likely be small.
	std::sort(modlist.begin(), modlist.end());

	// Package them up for Lua
	lua_createtable(L, modlist.size(), 0);
	std::vector<std::string>::iterator iter = modlist.begin();
	for (u16 i = 0; iter != modlist.end(); ++iter) {
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

// sound_play(spec, parameters, [ephemeral])
int ModApiServer::l_sound_play(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	ServerSoundParams params;
	read_server_sound_params(L, 2, params);
	bool ephemeral = lua_gettop(L) > 2 && readParam<bool>(L, 3);
	if (ephemeral) {
		getServer(L)->playSound(spec, params, true);
		lua_pushnil(L);
	} else {
		s32 handle = getServer(L)->playSound(spec, params);
		lua_pushinteger(L, handle);
	}
	return 1;
}

// sound_stop(handle)
int ModApiServer::l_sound_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	s32 handle = luaL_checkinteger(L, 1);
	getServer(L)->stopSound(handle);
	return 0;
}

int ModApiServer::l_sound_fade(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	s32 handle = luaL_checkinteger(L, 1);
	float step = readParam<float>(L, 2);
	float gain = readParam<float>(L, 3);
	getServer(L)->fadeSound(handle, step, gain);
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
	std::string name;
	if(lua_isstring(L, 1))
		name = readParam<std::string>(L, 1);
	getServer(L)->reportPrivsModified(name);
	return 0;
}

// get_last_run_mod()
int ModApiServer::l_get_last_run_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	std::string current_mod = readParam<std::string>(L, -1, "");
	if (current_mod.empty()) {
		lua_pop(L, 1);
		lua_pushstring(L, getScriptApiBase(L)->getOrigin().c_str());
	}
	return 1;
}

// set_last_run_mod(modname)
int ModApiServer::l_set_last_run_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
#ifdef SCRIPTAPI_DEBUG
	const char *mod = lua_tostring(L, 1);
	getScriptApiBase(L)->setOriginDirect(mod);
	//printf(">>>> last mod set from Lua: %s\n", mod);
#endif
	return 0;
}

void ModApiServer::Initialize(lua_State *L, int top)
{
	API_FCT(request_shutdown);
	API_FCT(get_server_status);
	API_FCT(get_server_uptime);
	API_FCT(get_worldpath);
	API_FCT(is_singleplayer);

	API_FCT(get_current_modname);
	API_FCT(get_modpath);
	API_FCT(get_modnames);

	API_FCT(print);

	API_FCT(chat_send_all);
	API_FCT(chat_send_player);
	API_FCT(show_formspec);
	API_FCT(sound_play);
	API_FCT(sound_stop);
	API_FCT(sound_fade);

	API_FCT(get_player_information);
	API_FCT(get_player_privs);
	API_FCT(get_player_ip);
	API_FCT(get_ban_list);
	API_FCT(get_ban_description);
	API_FCT(ban_player);
	API_FCT(kick_player);
	API_FCT(remove_player);
	API_FCT(unban_player_or_ip);
	API_FCT(notify_authentication_modified);

	API_FCT(get_last_run_mod);
	API_FCT(set_last_run_mod);
}
