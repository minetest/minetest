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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "lua_api/l_base.h"
#include "common/c_internal.h"
#include "server.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "lua_api/luaapi.h"
#include "settings.h"
#include "tool.h"
#include "rollback.h"
#include "log.h"
#include "emerge.h"
#include "main.h"  //required for g_settings

struct EnumString ModApiBasic::es_OreType[] =
{
	{ORE_SCATTER,  "scatter"},
	{ORE_SHEET,    "sheet"},
	{ORE_CLAYLIKE, "claylike"},
	{0, NULL},
};

struct EnumString ModApiBasic::es_DecorationType[] =
{
	{DECO_SIMPLE,    "simple"},
	{DECO_SCHEMATIC, "schematic"},
	{DECO_LSYSTEM,   "lsystem"},
	{0, NULL},
};

struct EnumString ModApiBasic::es_Rotation[] =
{
	{ROTATE_0,    "0"},
	{ROTATE_90,   "90"},
	{ROTATE_180,  "180"},
	{ROTATE_270,  "270"},
	{ROTATE_RAND, "random"},
	{0, NULL},
};


ModApiBasic::ModApiBasic() : ModApiBase() {
}

bool ModApiBasic::Initialize(lua_State* L,int top) {

	bool retval = true;

	retval &= API_FCT(debug);
	retval &= API_FCT(log);
	retval &= API_FCT(request_shutdown);
	retval &= API_FCT(get_server_status);

	retval &= API_FCT(register_biome);

	retval &= API_FCT(setting_set);
	retval &= API_FCT(setting_get);
	retval &= API_FCT(setting_getbool);
	retval &= API_FCT(setting_save);

	retval &= API_FCT(chat_send_all);
	retval &= API_FCT(chat_send_player);
	retval &= API_FCT(show_formspec);

	retval &= API_FCT(get_player_privs);
	retval &= API_FCT(get_player_ip);
	retval &= API_FCT(get_ban_list);
	retval &= API_FCT(get_ban_description);
	retval &= API_FCT(ban_player);
	retval &= API_FCT(unban_player_or_ip);
	retval &= API_FCT(get_password_hash);
	retval &= API_FCT(notify_authentication_modified);

	retval &= API_FCT(get_dig_params);
	retval &= API_FCT(get_hit_params);

	retval &= API_FCT(get_current_modname);
	retval &= API_FCT(get_modpath);
	retval &= API_FCT(get_modnames);

	retval &= API_FCT(get_worldpath);
	retval &= API_FCT(is_singleplayer);
	retval &= API_FCT(sound_play);
	retval &= API_FCT(sound_stop);

	retval &= API_FCT(rollback_get_last_node_actor);
	retval &= API_FCT(rollback_revert_actions_by);

	retval &= API_FCT(register_ore);
	retval &= API_FCT(register_decoration);
	retval &= API_FCT(create_schematic);
	retval &= API_FCT(place_schematic);

	return retval;
}

// debug(...)
// Writes a line to dstream
int ModApiBasic::l_debug(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	// Handle multiple parameters to behave like standard lua print()
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for(int i = 1; i <= n; i++){
		/*
			Call tostring(i-th argument).
			This is what print() does, and it behaves a bit
			differently from directly calling lua_tostring.
		*/
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);
		if(i>1)
			dstream << "\t";
		if(s)
			dstream << s;
		lua_pop(L, 1);
	}
	dstream << std::endl;
	return 0;
}

// log([level,] text)
// Writes a line to the logger.
// The one-argument version logs to infostream.
// The two-argument version accept a log level: error, action, info, or verbose.
int ModApiBasic::l_log(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string text;
	LogMessageLevel level = LMT_INFO;
	if(lua_isnone(L, 2))
	{
		text = lua_tostring(L, 1);
	}
	else
	{
		std::string levelname = luaL_checkstring(L, 1);
		text = luaL_checkstring(L, 2);
		if(levelname == "error")
			level = LMT_ERROR;
		else if(levelname == "action")
			level = LMT_ACTION;
		else if(levelname == "verbose")
			level = LMT_VERBOSE;
	}
	log_printline(level, text);
	return 0;
}

// request_shutdown()
int ModApiBasic::l_request_shutdown(lua_State *L)
{
	getServer(L)->requestShutdown();
	return 0;
}

// get_server_status()
int ModApiBasic::l_get_server_status(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, wide_to_narrow(getServer(L)->getStatusString()).c_str());
	return 1;
}

// register_biome({lots of stuff})
int ModApiBasic::l_register_biome(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	BiomeDefManager *bmgr = getServer(L)->getEmergeManager()->biomedef;
	if (!bmgr) {
		verbosestream << "register_biome: BiomeDefManager not active" << std::endl;
		return 0;
	}

	enum BiomeTerrainType terrain = (BiomeTerrainType)getenumfield(L, index,
				"terrain_type", es_BiomeTerrainType, BIOME_TERRAIN_NORMAL);
	Biome *b = bmgr->createBiome(terrain);

	b->name         = getstringfield_default(L, index, "name",
												"<no name>");
	b->nname_top    = getstringfield_default(L, index, "node_top",
												"mapgen_dirt_with_grass");
	b->nname_filler = getstringfield_default(L, index, "node_filler",
												"mapgen_dirt");
	b->nname_water  = getstringfield_default(L, index, "node_water",
												"mapgen_water_source");
	b->nname_dust   = getstringfield_default(L, index, "node_dust",
												"air");
	b->nname_dust_water = getstringfield_default(L, index, "node_dust_water",
												"mapgen_water_source");
	
	b->depth_top      = getintfield_default(L, index, "depth_top",    1);
	b->depth_filler   = getintfield_default(L, index, "depth_filler", 3);
	b->height_min     = getintfield_default(L, index, "height_min",   0);
	b->height_max     = getintfield_default(L, index, "height_max",   0);
	b->heat_point     = getfloatfield_default(L, index, "heat_point",     0.);
	b->humidity_point = getfloatfield_default(L, index, "humidity_point", 0.);

	b->flags        = 0; //reserved
	b->c_top        = CONTENT_IGNORE;
	b->c_filler     = CONTENT_IGNORE;
	b->c_water      = CONTENT_IGNORE;
	b->c_dust       = CONTENT_IGNORE;
	b->c_dust_water = CONTENT_IGNORE;
	
	verbosestream << "register_biome: " << b->name << std::endl;
	bmgr->addBiome(b);

	return 0;
}

// setting_set(name, value)
int ModApiBasic::l_setting_set(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	const char *value = luaL_checkstring(L, 2);
	g_settings->set(name, value);
	return 0;
}

// setting_get(name)
int ModApiBasic::l_setting_get(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	try{
		std::string value = g_settings->get(name);
		lua_pushstring(L, value.c_str());
	} catch(SettingNotFoundException &e){
		lua_pushnil(L);
	}
	return 1;
}

// setting_getbool(name)
int ModApiBasic::l_setting_getbool(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	try{
		bool value = g_settings->getBool(name);
		lua_pushboolean(L, value);
	} catch(SettingNotFoundException &e){
		lua_pushnil(L);
	}
	return 1;
}

// setting_save()
int ModApiBasic::l_setting_save(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	getServer(L)->saveConfig();
	return 0;
}

// chat_send_all(text)
int ModApiBasic::l_chat_send_all(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *text = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = getServer(L);
	// Send
	server->notifyPlayers(narrow_to_wide(text));
	return 0;
}

// chat_send_player(name, text, prepend)
int ModApiBasic::l_chat_send_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	const char *text = luaL_checkstring(L, 2);
	bool prepend = true;

	if (lua_isboolean(L, 3))
		prepend = lua_toboolean(L, 3);

	// Get server from registry
	Server *server = getServer(L);
	// Send
	server->notifyPlayer(name, narrow_to_wide(text), prepend);
	return 0;
}

// get_player_privs(name, text)
int ModApiBasic::l_get_player_privs(lua_State *L)
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
int ModApiBasic::l_get_player_ip(lua_State *L)
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
		Address addr = getServer(L)->getPeerAddress(getEnv(L)->getPlayer(name)->peer_id);
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

// get_ban_list()
int ModApiBasic::l_get_ban_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, getServer(L)->getBanDescription("").c_str());
	return 1;
}

// get_ban_description()
int ModApiBasic::l_get_ban_description(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * ip_or_name = luaL_checkstring(L, 1);
	lua_pushstring(L, getServer(L)->getBanDescription(std::string(ip_or_name)).c_str());
	return 1;
}

// ban_player()
int ModApiBasic::l_ban_player(lua_State *L)
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

// unban_player_or_ip()
int ModApiBasic::l_unban_player_or_ip(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char * ip_or_name = luaL_checkstring(L, 1);
	getServer(L)->unsetIpBanned(ip_or_name);
	lua_pushboolean(L, true);
	return 1;
}

// show_formspec(playername,formname,formspec)
int ModApiBasic::l_show_formspec(lua_State *L)
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

// get_dig_params(groups, tool_capabilities[, time_from_last_punch])
int ModApiBasic::l_get_dig_params(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::map<std::string, int> groups;
	read_groups(L, 1, groups);
	ToolCapabilities tp = read_tool_capabilities(L, 2);
	if(lua_isnoneornil(L, 3))
		push_dig_params(L, getDigParams(groups, &tp));
	else
		push_dig_params(L, getDigParams(groups, &tp,
					luaL_checknumber(L, 3)));
	return 1;
}

// get_hit_params(groups, tool_capabilities[, time_from_last_punch])
int ModApiBasic::l_get_hit_params(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::map<std::string, int> groups;
	read_groups(L, 1, groups);
	ToolCapabilities tp = read_tool_capabilities(L, 2);
	if(lua_isnoneornil(L, 3))
		push_hit_params(L, getHitParams(groups, &tp));
	else
		push_hit_params(L, getHitParams(groups, &tp,
					luaL_checknumber(L, 3)));
	return 1;
}

// get_current_modname()
int ModApiBasic::l_get_current_modname(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	return 1;
}

// get_modpath(modname)
int ModApiBasic::l_get_modpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string modname = luaL_checkstring(L, 1);
	// Do it
	if(modname == "__builtin"){
		std::string path = getServer(L)->getBuiltinLuaPath();
		lua_pushstring(L, path.c_str());
		return 1;
	}
	const ModSpec *mod = getServer(L)->getModSpec(modname);
	if(!mod){
		lua_pushnil(L);
		return 1;
	}
	lua_pushstring(L, mod->path.c_str());
	return 1;
}

// get_modnames()
// the returned list is sorted alphabetically for you
int ModApiBasic::l_get_modnames(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	// Get a list of mods
	std::list<std::string> mods_unsorted, mods_sorted;
	getServer(L)->getModNames(mods_unsorted);

	// Take unsorted items from mods_unsorted and sort them into
	// mods_sorted; not great performance but the number of mods on a
	// server will likely be small.
	for(std::list<std::string>::iterator i = mods_unsorted.begin();
		i != mods_unsorted.end(); ++i)
	{
		bool added = false;
		for(std::list<std::string>::iterator x = mods_sorted.begin();
			x != mods_sorted.end(); ++x)
		{
			// I doubt anybody using Minetest will be using
			// anything not ASCII based :)
			if((*i).compare(*x) <= 0)
			{
				mods_sorted.insert(x, *i);
				added = true;
				break;
			}
		}
		if(!added)
			mods_sorted.push_back(*i);
	}

	// Get the table insertion function from Lua.
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int insertion_func = lua_gettop(L);

	// Package them up for Lua
	lua_newtable(L);
	int new_table = lua_gettop(L);
	std::list<std::string>::iterator i = mods_sorted.begin();
	while(i != mods_sorted.end())
	{
		lua_pushvalue(L, insertion_func);
		lua_pushvalue(L, new_table);
		lua_pushstring(L, (*i).c_str());
		if(lua_pcall(L, 2, 0, 0) != 0)
		{
			script_error(L, "error: %s", lua_tostring(L, -1));
		}
		++i;
	}
	return 1;
}

// get_worldpath()
int ModApiBasic::l_get_worldpath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string worldpath = getServer(L)->getWorldPath();
	lua_pushstring(L, worldpath.c_str());
	return 1;
}

// sound_play(spec, parameters)
int ModApiBasic::l_sound_play(lua_State *L)
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
int ModApiBasic::l_sound_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	int handle = luaL_checkinteger(L, 1);
	getServer(L)->stopSound(handle);
	return 0;
}

// is_singleplayer()
int ModApiBasic::l_is_singleplayer(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, getServer(L)->isSingleplayer());
	return 1;
}

// get_password_hash(name, raw_password)
int ModApiBasic::l_get_password_hash(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	std::string raw_password = luaL_checkstring(L, 2);
	std::string hash = translatePassword(name,
			narrow_to_wide(raw_password));
	lua_pushstring(L, hash.c_str());
	return 1;
}

// notify_authentication_modified(name)
int ModApiBasic::l_notify_authentication_modified(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = "";
	if(lua_isstring(L, 1))
		name = lua_tostring(L, 1);
	getServer(L)->reportPrivsModified(name);
	return 0;
}

// rollback_get_last_node_actor(p, range, seconds) -> actor, p, seconds
int ModApiBasic::l_rollback_get_last_node_actor(lua_State *L)
{
	v3s16 p = read_v3s16(L, 1);
	int range = luaL_checknumber(L, 2);
	int seconds = luaL_checknumber(L, 3);
	Server *server = getServer(L);
	IRollbackManager *rollback = server->getRollbackManager();
	v3s16 act_p;
	int act_seconds = 0;
	std::string actor = rollback->getLastNodeActor(p, range, seconds, &act_p, &act_seconds);
	lua_pushstring(L, actor.c_str());
	push_v3s16(L, act_p);
	lua_pushnumber(L, act_seconds);
	return 3;
}

// rollback_revert_actions_by(actor, seconds) -> bool, log messages
int ModApiBasic::l_rollback_revert_actions_by(lua_State *L)
{
	std::string actor = luaL_checkstring(L, 1);
	int seconds = luaL_checknumber(L, 2);
	Server *server = getServer(L);
	IRollbackManager *rollback = server->getRollbackManager();
	std::list<RollbackAction> actions = rollback->getRevertActions(actor, seconds);
	std::list<std::string> log;
	bool success = server->rollbackRevertActions(actions, &log);
	// Push boolean result
	lua_pushboolean(L, success);
	// Get the table insert function and push the log table
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	lua_newtable(L);
	int table = lua_gettop(L);
	for(std::list<std::string>::const_iterator i = log.begin();
			i != log.end(); i++)
	{
		lua_pushvalue(L, table_insert);
		lua_pushvalue(L, table);
		lua_pushstring(L, i->c_str());
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
	lua_remove(L, -2); // Remove table
	lua_remove(L, -2); // Remove insert
	return 2;
}

int ModApiBasic::l_register_ore(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	EmergeManager *emerge = getServer(L)->getEmergeManager();

	enum OreType oretype = (OreType)getenumfield(L, index,
				"ore_type", es_OreType, ORE_SCATTER);
	Ore *ore = createOre(oretype);
	if (!ore) {
		errorstream << "register_ore: ore_type "
			<< oretype << " not implemented";
		return 0;
	}

	ore->ore_name       = getstringfield_default(L, index, "ore", "");
	ore->ore_param2     = (u8)getintfield_default(L, index, "ore_param2", 0);
	ore->clust_scarcity = getintfield_default(L, index, "clust_scarcity", 1);
	ore->clust_num_ores = getintfield_default(L, index, "clust_num_ores", 1);
	ore->clust_size     = getintfield_default(L, index, "clust_size", 0);
	ore->height_min     = getintfield_default(L, index, "height_min", 0);
	ore->height_max     = getintfield_default(L, index, "height_max", 0);
	ore->flags          = getflagsfield(L, index, "flags", flagdesc_ore);
	ore->nthresh        = getfloatfield_default(L, index, "noise_threshhold", 0.);

	lua_getfield(L, index, "wherein");
	if (lua_istable(L, -1)) {
		int  i = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, i) != 0) {
			ore->wherein_names.push_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	} else if (lua_isstring(L, -1)) {
		ore->wherein_names.push_back(lua_tostring(L, -1));
	} else {
		ore->wherein_names.push_back("");
	}
	lua_pop(L, 1);

	lua_getfield(L, index, "noise_params");
	ore->np = read_noiseparams(L, -1);
	lua_pop(L, 1);

	ore->noise = NULL;

	if (ore->clust_scarcity <= 0 || ore->clust_num_ores <= 0) {
		errorstream << "register_ore: clust_scarcity and clust_num_ores"
			"must be greater than 0" << std::endl;
		delete ore;
		return 0;
	}

	emerge->ores.push_back(ore);

	verbosestream << "register_ore: ore '" << ore->ore_name
		<< "' registered" << std::endl;
	return 0;
}

// register_decoration({lots of stuff})
int ModApiBasic::l_register_decoration(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	EmergeManager *emerge = getServer(L)->getEmergeManager();
	BiomeDefManager *bdef = emerge->biomedef;

	enum DecorationType decotype = (DecorationType)getenumfield(L, index,
				"deco_type", es_DecorationType, -1);
	if (decotype == -1) {
		errorstream << "register_decoration: unrecognized "
			"decoration placement type";
		return 0;
	}
	
	Decoration *deco = createDecoration(decotype);
	if (!deco) {
		errorstream << "register_decoration: decoration placement type "
			<< decotype << " not implemented";
		return 0;
	}

	deco->c_place_on    = CONTENT_IGNORE;
	deco->place_on_name = getstringfield_default(L, index, "place_on", "ignore");
	deco->fill_ratio    = getfloatfield_default(L, index, "fill_ratio", 0.02);
	deco->sidelen       = getintfield_default(L, index, "sidelen", 8);
	if (deco->sidelen <= 0) {
		errorstream << "register_decoration: sidelen must be "
			"greater than 0" << std::endl;
		delete deco;
		return 0;
	}
	
	lua_getfield(L, index, "noise_params");
	deco->np = read_noiseparams(L, -1);
	lua_pop(L, 1);
	
	lua_getfield(L, index, "biomes");
	if (lua_istable(L, -1)) {
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			const char *s = lua_tostring(L, -1);
			u8 biomeid = bdef->getBiomeIdByName(s);
			if (biomeid)
				deco->biomes.insert(biomeid);

			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	
	switch (decotype) {
		case DECO_SIMPLE: {
			DecoSimple *dsimple = (DecoSimple *)deco;
			dsimple->c_deco     = CONTENT_IGNORE;
			dsimple->c_spawnby  = CONTENT_IGNORE;
			dsimple->spawnby_name    = getstringfield_default(L, index, "spawn_by", "air");
			dsimple->deco_height     = getintfield_default(L, index, "height", 1);
			dsimple->deco_height_max = getintfield_default(L, index, "height_max", 0);
			dsimple->nspawnby        = getintfield_default(L, index, "num_spawn_by", -1);
			
			lua_getfield(L, index, "decoration");
			if (lua_istable(L, -1)) {
				lua_pushnil(L);
				while (lua_next(L, -2)) {
					const char *s = lua_tostring(L, -1);
					std::string str(s);
					dsimple->decolist_names.push_back(str);

					lua_pop(L, 1);
				}
			} else if (lua_isstring(L, -1)) {
				dsimple->deco_name = std::string(lua_tostring(L, -1));
			} else {
				dsimple->deco_name = std::string("air");
			}
			lua_pop(L, 1);
			
			if (dsimple->deco_height <= 0) {
				errorstream << "register_decoration: simple decoration height"
					" must be greater than 0" << std::endl;
				delete dsimple;
				return 0;
			}

			break; }
		case DECO_SCHEMATIC: {
			DecoSchematic *dschem = (DecoSchematic *)deco;
			dschem->flags    = getflagsfield(L, index, "flags", flagdesc_deco_schematic);
			dschem->rotation = (Rotation)getenumfield(L, index,
								"rotation", es_Rotation, ROTATE_0);
			
			lua_getfield(L, index, "schematic");
			if (!read_schematic(L, -1, dschem, getServer(L))) {
				delete dschem;
				return 0;
			}
			lua_pop(L, -1);
			
			if (!dschem->filename.empty() && !dschem->loadSchematicFile()) {
				errorstream << "register_decoration: failed to load schematic file '"
					<< dschem->filename << "'" << std::endl;
				delete dschem;
				return 0;
			}
			break; }
		case DECO_LSYSTEM: {
			//DecoLSystem *decolsystem = (DecoLSystem *)deco;
		
			break; }
	}

	emerge->decorations.push_back(deco);

	verbosestream << "register_decoration: decoration '" << deco->getName()
		<< "' registered" << std::endl;
	return 0;
}

// create_schematic(p1, p2, probability_list, filename)
int ModApiBasic::l_create_schematic(lua_State *L)
{
	DecoSchematic dschem;

	Map *map = &(getEnv(L)->getMap());
	INodeDefManager *ndef = getServer(L)->getNodeDefManager();

	v3s16 p1 = read_v3s16(L, 1);
	v3s16 p2 = read_v3s16(L, 2);
	sortBoxVerticies(p1, p2);
	
	std::vector<std::pair<v3s16, s16> > probability_list;
	if (lua_istable(L, 3)) {
		lua_pushnil(L);
		while (lua_next(L, 3)) {
			if (lua_istable(L, -1)) {
				lua_getfield(L, -1, "pos");
				v3s16 pos = read_v3s16(L, -1);
				lua_pop(L, 1);
				
				s16 prob = getintfield_default(L, -1, "prob", 0);
				if (prob < -1 || prob >= UCHAR_MAX) {
					errorstream << "create_schematic: probability value of "
						<< prob << " at " << PP(pos) << " out of range" << std::endl;
				} else {
					probability_list.push_back(std::make_pair(pos, prob));
				}
			}

			lua_pop(L, 1);
		}
	}
	
	dschem.filename = std::string(lua_tostring(L, 4));

	if (!dschem.getSchematicFromMap(map, p1, p2)) {
		errorstream << "create_schematic: failed to get schematic "
			"from map" << std::endl;
		return 0;
	}
	
	dschem.applyProbabilities(&probability_list, p1);
	
	dschem.saveSchematicFile(ndef);
	actionstream << "create_schematic: saved schematic file '"
		<< dschem.filename << "'." << std::endl;

	return 1;
}


// place_schematic(p, schematic, rotation)
int ModApiBasic::l_place_schematic(lua_State *L)
{
	DecoSchematic dschem;

	Map *map = &(getEnv(L)->getMap());
	INodeDefManager *ndef = getServer(L)->getNodeDefManager();

	v3s16 p = read_v3s16(L, 1);
	if (!read_schematic(L, 2, &dschem, getServer(L)))
		return 0;
		
	Rotation rot = ROTATE_0;
	if (lua_isstring(L, 3))
		string_to_enum(es_Rotation, (int &)rot, std::string(lua_tostring(L, 3)));
		
	dschem.rotation = rot;

	if (!dschem.filename.empty()) {
		if (!dschem.loadSchematicFile()) {
			errorstream << "place_schematic: failed to load schematic file '"
				<< dschem.filename << "'" << std::endl;
			return 0;
		}
		dschem.resolveNodeNames(ndef);
	}
	
	dschem.placeStructure(map, p);

	return 1;
}


ModApiBasic modapibasic_prototype;
