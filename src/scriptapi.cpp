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

#include "scriptapi.h"

#include <iostream>
#include <list>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "settings.h" // For accessing g_settings
#include "main.h" // For g_settings
#include "biome.h"
#include "emerge.h"
#include "script.h"
#include "rollback.h"

#include "scriptapi_types.h"
#include "scriptapi_env.h"
#include "scriptapi_nodetimer.h"
#include "scriptapi_inventory.h"
#include "scriptapi_nodemeta.h"
#include "scriptapi_object.h"
#include "scriptapi_noise.h"
#include "scriptapi_common.h"
#include "scriptapi_item.h"
#include "scriptapi_content.h"
#include "scriptapi_craft.h"
#include "scriptapi_particles.h"

/*****************************************************************************/
/* Mod related                                                               */
/*****************************************************************************/

class ModNameStorer
{
private:
	lua_State *L;
public:
	ModNameStorer(lua_State *L_, const std::string modname):
		L(L_)
	{
		// Store current modname in registry
		lua_pushstring(L, modname.c_str());
		lua_setfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	}
	~ModNameStorer()
	{
		// Clear current modname in registry
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	}
};

bool scriptapi_loadmod(lua_State *L, const std::string &scriptpath,
		const std::string &modname)
{
	ModNameStorer modnamestorer(L, modname);

	if(!string_allowed(modname, "abcdefghijklmnopqrstuvwxyz"
			"0123456789_")){
		errorstream<<"Error loading mod \""<<modname
				<<"\": modname does not follow naming conventions: "
				<<"Only chararacters [a-z0-9_] are allowed."<<std::endl;
		return false;
	}

	bool success = false;

	try{
		success = script_load(L, scriptpath.c_str());
	}
	catch(LuaError &e){
		errorstream<<"Error loading mod \""<<modname
				<<"\": "<<e.what()<<std::endl;
	}

	return success;
}


/*****************************************************************************/
/* Auth                                                                      */
/*****************************************************************************/

/*
	Privileges
*/
static void read_privileges(lua_State *L, int index,
		std::set<std::string> &result)
{
	result.clear();
	lua_pushnil(L);
	if(index < 0)
		index -= 1;
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		std::string key = luaL_checkstring(L, -2);
		bool value = lua_toboolean(L, -1);
		if(value)
			result.insert(key);
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

static void get_auth_handler(lua_State *L)
{
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_auth_handler");
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		lua_getfield(L, -1, "builtin_auth_handler");
	}
	if(lua_type(L, -1) != LUA_TTABLE)
		throw LuaError(L, "Authentication handler table not valid");
}

bool scriptapi_get_auth(lua_State *L, const std::string &playername,
		std::string *dst_password, std::set<std::string> *dst_privs)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	get_auth_handler(L);
	lua_getfield(L, -1, "get_auth");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing get_auth");
	lua_pushstring(L, playername.c_str());
	if(lua_pcall(L, 1, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));

	// nil = login not allowed
	if(lua_isnil(L, -1))
		return false;
	luaL_checktype(L, -1, LUA_TTABLE);

	std::string password;
	bool found = getstringfield(L, -1, "password", password);
	if(!found)
		throw LuaError(L, "Authentication handler didn't return password");
	if(dst_password)
		*dst_password = password;

	lua_getfield(L, -1, "privileges");
	if(!lua_istable(L, -1))
		throw LuaError(L,
				"Authentication handler didn't return privilege table");
	if(dst_privs)
		read_privileges(L, -1, *dst_privs);
	lua_pop(L, 1);

	return true;
}

void scriptapi_create_auth(lua_State *L, const std::string &playername,
		const std::string &password)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	get_auth_handler(L);
	lua_getfield(L, -1, "create_auth");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing create_auth");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

bool scriptapi_set_password(lua_State *L, const std::string &playername,
		const std::string &password)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	get_auth_handler(L);
	lua_getfield(L, -1, "set_password");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing set_password");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	if(lua_pcall(L, 2, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	return lua_toboolean(L, -1);
}

/*****************************************************************************/
/* Misc                                                                      */
/*****************************************************************************/
/*
	Groups
*/
void read_groups(lua_State *L, int index,
		std::map<std::string, int> &result)
{
	if (!lua_istable(L,index))
		return;
	result.clear();
	lua_pushnil(L);
	if(index < 0)
		index -= 1;
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		std::string name = luaL_checkstring(L, -2);
		int rating = luaL_checkinteger(L, -1);
		result[name] = rating;
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

struct EnumString es_BiomeTerrainType[] =
{
	{BIOME_TERRAIN_NORMAL, "normal"},
	{BIOME_TERRAIN_LIQUID, "liquid"},
	{BIOME_TERRAIN_NETHER, "nether"},
	{BIOME_TERRAIN_AETHER, "aether"},
	{BIOME_TERRAIN_FLAT,   "flat"},
	{0, NULL},
};

struct EnumString es_OreType[] =
{
	{ORE_SCATTER,  "scatter"},
	{ORE_SHEET,    "sheet"},
	{ORE_CLAYLIKE, "claylike"},
	{0, NULL},
};

/*****************************************************************************/
/* Parameters                                                                */
/*****************************************************************************/
/*
	DigParams
*/

static void set_dig_params(lua_State *L, int table,
		const DigParams &params)
{
	setboolfield(L, table, "diggable", params.diggable);
	setfloatfield(L, table, "time", params.time);
	setintfield(L, table, "wear", params.wear);
}

static void push_dig_params(lua_State *L,
		const DigParams &params)
{
	lua_newtable(L);
	set_dig_params(L, -1, params);
}

/*
	HitParams
*/

static void set_hit_params(lua_State *L, int table,
		const HitParams &params)
{
	setintfield(L, table, "hp", params.hp);
	setintfield(L, table, "wear", params.wear);
}

static void push_hit_params(lua_State *L,
		const HitParams &params)
{
	lua_newtable(L);
	set_hit_params(L, -1, params);
}

/*
	ServerSoundParams
*/

static void read_server_sound_params(lua_State *L, int index,
		ServerSoundParams &params)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	// Clear
	params = ServerSoundParams();
	if(lua_istable(L, index)){
		getfloatfield(L, index, "gain", params.gain);
		getstringfield(L, index, "to_player", params.to_player);
		lua_getfield(L, index, "pos");
		if(!lua_isnil(L, -1)){
			v3f p = read_v3f(L, -1)*BS;
			params.pos = p;
			params.type = ServerSoundParams::SSP_POSITIONAL;
		}
		lua_pop(L, 1);
		lua_getfield(L, index, "object");
		if(!lua_isnil(L, -1)){
			ObjectRef *ref = ObjectRef::checkobject(L, -1);
			ServerActiveObject *sao = ObjectRef::getobject(ref);
			if(sao){
				params.object = sao->getId();
				params.type = ServerSoundParams::SSP_OBJECT;
			}
		}
		lua_pop(L, 1);
		params.max_hear_distance = BS*getfloatfield_default(L, index,
				"max_hear_distance", params.max_hear_distance/BS);
		getboolfield(L, index, "loop", params.loop);
	}
}

/*****************************************************************************/
/* callbacks                                                                 */
/*****************************************************************************/

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - removes the table and arguments from the lua stack
// - pushes the return value, computed depending on mode
void scriptapi_run_callbacks(lua_State *L, int nargs,
		RunCallbacksMode mode)
{
	// Insert the return value into the lua stack, below the table
	assert(lua_gettop(L) >= nargs + 1);
	lua_pushnil(L);
	lua_insert(L, -(nargs + 1) - 1);
	// Stack now looks like this:
	// ... <return value = nil> <table> <arg#1> <arg#2> ... <arg#n>

	int rv = lua_gettop(L) - nargs - 1;
	int table = rv + 1;
	int arg = table + 1;

	luaL_checktype(L, table, LUA_TTABLE);

	// Foreach
	lua_pushnil(L);
	bool first_loop = true;
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		for(int i = 0; i < nargs; i++)
			lua_pushvalue(L, arg+i);
		if(lua_pcall(L, nargs, 1, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));

		// Move return value to designated space in stack
		// Or pop it
		if(first_loop){
			// Result of first callback is always moved
			lua_replace(L, rv);
			first_loop = false;
		} else {
			// Otherwise, what happens depends on the mode
			if(mode == RUN_CALLBACKS_MODE_FIRST)
				lua_pop(L, 1);
			else if(mode == RUN_CALLBACKS_MODE_LAST)
				lua_replace(L, rv);
			else if(mode == RUN_CALLBACKS_MODE_AND ||
					mode == RUN_CALLBACKS_MODE_AND_SC){
				if((bool)lua_toboolean(L, rv) == true &&
						(bool)lua_toboolean(L, -1) == false)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else if(mode == RUN_CALLBACKS_MODE_OR ||
					mode == RUN_CALLBACKS_MODE_OR_SC){
				if((bool)lua_toboolean(L, rv) == false &&
						(bool)lua_toboolean(L, -1) == true)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else
				assert(0);
		}

		// Handle short circuit modes
		if(mode == RUN_CALLBACKS_MODE_AND_SC &&
				(bool)lua_toboolean(L, rv) == false)
			break;
		else if(mode == RUN_CALLBACKS_MODE_OR_SC &&
				(bool)lua_toboolean(L, rv) == true)
			break;

		// value removed, keep key for next iteration
	}

	// Remove stuff from stack, leaving only the return value
	lua_settop(L, rv);

	// Fix return value in case no callbacks were called
	if(first_loop){
		if(mode == RUN_CALLBACKS_MODE_AND ||
				mode == RUN_CALLBACKS_MODE_AND_SC){
			lua_pop(L, 1);
			lua_pushboolean(L, true);
		}
		else if(mode == RUN_CALLBACKS_MODE_OR ||
				mode == RUN_CALLBACKS_MODE_OR_SC){
			lua_pop(L, 1);
			lua_pushboolean(L, false);
		}
	}
}

bool scriptapi_on_chat_message(lua_State *L, const std::string &name,
		const std::string &message)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_chat_messages
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_chat_messages");
	// Call callbacks
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, message.c_str());
	scriptapi_run_callbacks(L, 2, RUN_CALLBACKS_MODE_OR_SC);
	bool ate = lua_toboolean(L, -1);
	return ate;
}

void scriptapi_on_shutdown(lua_State *L)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get registered shutdown hooks
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	scriptapi_run_callbacks(L, 0, RUN_CALLBACKS_MODE_FIRST);
}

void scriptapi_on_newplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_newplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_newplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void scriptapi_on_dieplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_dieplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_dieplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

bool scriptapi_on_respawnplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_respawnplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_respawnplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_OR);
	bool positioning_handled_by_some = lua_toboolean(L, -1);
	return positioning_handled_by_some;
}

void scriptapi_on_joinplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_joinplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_joinplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void scriptapi_on_leaveplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_leaveplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_leaveplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

/*
	player
*/
void scriptapi_on_player_receive_fields(lua_State *L,
		ServerActiveObject *player,
		const std::string &formname,
		const std::map<std::string, std::string> &fields)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_chat_messages
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_player_receive_fields");
	// Call callbacks
	// param 1
	objectref_get_or_create(L, player);
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
	scriptapi_run_callbacks(L, 3, RUN_CALLBACKS_MODE_OR_SC);
}
/*****************************************************************************/
/* Api functions                                                             */
/*****************************************************************************/

// debug(text)
// Writes a line to dstream
static int l_debug(lua_State *L)
{
	std::string text = lua_tostring(L, 1);
	dstream << text << std::endl;
	return 0;
}

// log([level,] text)
// Writes a line to the logger.
// The one-argument version logs to infostream.
// The two-argument version accept a log level: error, action, info, or verbose.
static int l_log(lua_State *L)
{
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
static int l_request_shutdown(lua_State *L)
{
	get_server(L)->requestShutdown();
	return 0;
}

// get_server_status()
static int l_get_server_status(lua_State *L)
{
	lua_pushstring(L, wide_to_narrow(get_server(L)->getStatusString()).c_str());
	return 1;
}


// register_biome({lots of stuff})
static int l_register_biome(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	BiomeDefManager *bmgr = get_server(L)->getEmergeManager()->biomedef;
	if (!bmgr) {
		verbosestream << "register_biome: BiomeDefManager not active" << std::endl;
		return 0;
	}
	
	enum BiomeTerrainType terrain = (BiomeTerrainType)getenumfield(L, index,
					"terrain_type", es_BiomeTerrainType, BIOME_TERRAIN_NORMAL);
	Biome *b = bmgr->createBiome(terrain);

	b->name            = getstringfield_default(L, index, "name", "");
	b->top_nodename    = getstringfield_default(L, index, "top_node", "");
	b->top_depth       = getintfield_default(L, index, "top_depth", 0);
	b->filler_nodename = getstringfield_default(L, index, "filler_node", "");
	b->filler_height   = getintfield_default(L, index, "filler_height", 0);
	b->height_min      = getintfield_default(L, index, "height_min", 0);
	b->height_max      = getintfield_default(L, index, "height_max", 0);
	b->heat_point      = getfloatfield_default(L, index, "heat_point", 0.);
	b->humidity_point  = getfloatfield_default(L, index, "humidity_point", 0.);

	b->flags    = 0; //reserved
	b->c_top    = CONTENT_IGNORE;
	b->c_filler = CONTENT_IGNORE;
	bmgr->addBiome(b);

	verbosestream << "register_biome: " << b->name << std::endl;
	return 0;
}


static int l_register_ore(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);
	
	EmergeManager *emerge = get_server(L)->getEmergeManager();
	
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
	ore->wherein_name   = getstringfield_default(L, index, "wherein", "");
	ore->clust_scarcity = getintfield_default(L, index, "clust_scarcity", 1);
	ore->clust_num_ores = getintfield_default(L, index, "clust_num_ores", 1);
	ore->clust_size     = getintfield_default(L, index, "clust_size", 0);
	ore->height_min     = getintfield_default(L, index, "height_min", 0);
	ore->height_max     = getintfield_default(L, index, "height_max", 0);
	ore->flags          = getflagsfield(L, index, "flags", flagdesc_ore);
	ore->nthresh        = getfloatfield_default(L, index, "noise_threshhold", 0.);

	lua_getfield(L, index, "noise_params");
	ore->np = read_noiseparams(L, -1);
	lua_pop(L, 1);
	
	ore->noise = NULL;
	
	if (ore->clust_scarcity <= 0 || ore->clust_num_ores <= 0) {
		errorstream << "register_ore: clust_scarcity and clust_num_ores"
			" must be greater than 0" << std::endl;
		delete ore;
		return 0;
	}
	
	emerge->ores.push_back(ore);
	
	verbosestream << "register_ore: ore '" << ore->ore_name
		<< "' registered" << std::endl;
	return 0;
}


// setting_set(name, value)
static int l_setting_set(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	const char *value = luaL_checkstring(L, 2);
	g_settings->set(name, value);
	return 0;
}

// setting_get(name)
static int l_setting_get(lua_State *L)
{
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
static int l_setting_getbool(lua_State *L)
{
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
static int l_setting_save(lua_State *L)
{
	get_server(L)->saveConfig();
	return 0;
}

// chat_send_all(text)
static int l_chat_send_all(lua_State *L)
{
	const char *text = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = get_server(L);
	// Send
	server->notifyPlayers(narrow_to_wide(text));
	return 0;
}

// chat_send_player(name, text, prepend)
static int l_chat_send_player(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	const char *text = luaL_checkstring(L, 2);
	bool prepend = true;
	if (lua_isboolean(L, 3))
		prepend = lua_toboolean(L, 3);
	// Get server from registry
	Server *server = get_server(L);
	// Send
	server->notifyPlayer(name, narrow_to_wide(text), prepend);
	return 0;
}

// get_player_privs(name, text)
static int l_get_player_privs(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = get_server(L);
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

// get_ban_list()
static int l_get_ban_list(lua_State *L)
{
	lua_pushstring(L, get_server(L)->getBanDescription("").c_str());
	return 1;
}

// get_ban_description()
static int l_get_ban_description(lua_State *L)
{
	const char * ip_or_name = luaL_checkstring(L, 1);
	lua_pushstring(L, get_server(L)->getBanDescription(std::string(ip_or_name)).c_str());
	return 1;
}

// ban_player()
static int l_ban_player(lua_State *L)
{
	const char * name = luaL_checkstring(L, 1);
	Player *player = get_env(L)->getPlayer(name);
	if(player == NULL)
	{
		lua_pushboolean(L, false); // no such player
		return 1;
	}
	try
	{
		Address addr = get_server(L)->getPeerAddress(get_env(L)->getPlayer(name)->peer_id);
		std::string ip_str = addr.serializeString();
		get_server(L)->setIpBanned(ip_str, name);
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
static int l_unban_player_of_ip(lua_State *L)
{
	const char * ip_or_name = luaL_checkstring(L, 1);
	get_server(L)->unsetIpBanned(ip_or_name);
	lua_pushboolean(L, true);
	return 1;
}

// show_formspec(playername,formname,formspec)
static int l_show_formspec(lua_State *L)
{
	const char *playername = luaL_checkstring(L, 1);
	const char *formname = luaL_checkstring(L, 2);
	const char *formspec = luaL_checkstring(L, 3);

	if(get_server(L)->showFormspec(playername,formspec,formname))
	{
		lua_pushboolean(L, true);
	}else{
		lua_pushboolean(L, false);
	}
	return 1;
}

// get_dig_params(groups, tool_capabilities[, time_from_last_punch])
static int l_get_dig_params(lua_State *L)
{
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
static int l_get_hit_params(lua_State *L)
{
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
static int l_get_current_modname(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	return 1;
}

// get_modpath(modname)
static int l_get_modpath(lua_State *L)
{
	std::string modname = luaL_checkstring(L, 1);
	// Do it
	if(modname == "__builtin"){
		std::string path = get_server(L)->getBuiltinLuaPath();
		lua_pushstring(L, path.c_str());
		return 1;
	}
	const ModSpec *mod = get_server(L)->getModSpec(modname);
	if(!mod){
		lua_pushnil(L);
		return 1;
	}
	lua_pushstring(L, mod->path.c_str());
	return 1;
}

// get_modnames()
// the returned list is sorted alphabetically for you
static int l_get_modnames(lua_State *L)
{
	// Get a list of mods
	std::list<std::string> mods_unsorted, mods_sorted;
	get_server(L)->getModNames(mods_unsorted);

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
static int l_get_worldpath(lua_State *L)
{
	std::string worldpath = get_server(L)->getWorldPath();
	lua_pushstring(L, worldpath.c_str());
	return 1;
}

// sound_play(spec, parameters)
static int l_sound_play(lua_State *L)
{
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	ServerSoundParams params;
	read_server_sound_params(L, 2, params);
	s32 handle = get_server(L)->playSound(spec, params);
	lua_pushinteger(L, handle);
	return 1;
}

// sound_stop(handle)
static int l_sound_stop(lua_State *L)
{
	int handle = luaL_checkinteger(L, 1);
	get_server(L)->stopSound(handle);
	return 0;
}

// is_singleplayer()
static int l_is_singleplayer(lua_State *L)
{
	lua_pushboolean(L, get_server(L)->isSingleplayer());
	return 1;
}

// get_password_hash(name, raw_password)
static int l_get_password_hash(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	std::string raw_password = luaL_checkstring(L, 2);
	std::string hash = translatePassword(name,
			narrow_to_wide(raw_password));
	lua_pushstring(L, hash.c_str());
	return 1;
}

// notify_authentication_modified(name)
static int l_notify_authentication_modified(lua_State *L)
{
	std::string name = "";
	if(lua_isstring(L, 1))
		name = lua_tostring(L, 1);
	get_server(L)->reportPrivsModified(name);
	return 0;
}

// rollback_get_last_node_actor(p, range, seconds) -> actor, p, seconds
static int l_rollback_get_last_node_actor(lua_State *L)
{
	v3s16 p = read_v3s16(L, 1);
	int range = luaL_checknumber(L, 2);
	int seconds = luaL_checknumber(L, 3);
	Server *server = get_server(L);
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
static int l_rollback_revert_actions_by(lua_State *L)
{
	std::string actor = luaL_checkstring(L, 1);
	int seconds = luaL_checknumber(L, 2);
	Server *server = get_server(L);
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

static const struct luaL_Reg minetest_f [] = {
	{"debug", l_debug},
	{"log", l_log},
	{"request_shutdown", l_request_shutdown},
	{"get_server_status", l_get_server_status},
	{"register_item_raw", l_register_item_raw},
	{"register_alias_raw", l_register_alias_raw},
	{"register_craft", l_register_craft},
	{"register_biome", l_register_biome},
	{"register_ore", l_register_ore},
	{"setting_set", l_setting_set},
	{"setting_get", l_setting_get},
	{"setting_getbool", l_setting_getbool},
	{"setting_save",l_setting_save},
	{"chat_send_all", l_chat_send_all},
	{"chat_send_player", l_chat_send_player},
	{"get_player_privs", l_get_player_privs},
	{"get_ban_list", l_get_ban_list},
	{"get_ban_description", l_get_ban_description},
	{"ban_player", l_ban_player},
	{"unban_player_or_ip", l_unban_player_of_ip},
	{"get_inventory", l_get_inventory},
	{"create_detached_inventory_raw", l_create_detached_inventory_raw},
	{"show_formspec", l_show_formspec},
	{"get_dig_params", l_get_dig_params},
	{"get_hit_params", l_get_hit_params},
	{"get_current_modname", l_get_current_modname},
	{"get_modpath", l_get_modpath},
	{"get_modnames", l_get_modnames},
	{"get_worldpath", l_get_worldpath},
	{"sound_play", l_sound_play},
	{"sound_stop", l_sound_stop},
	{"is_singleplayer", l_is_singleplayer},
	{"get_password_hash", l_get_password_hash},
	{"notify_authentication_modified", l_notify_authentication_modified},
	{"get_craft_result", l_get_craft_result},
	{"get_craft_recipe", l_get_craft_recipe},
	{"get_all_craft_recipes", l_get_all_craft_recipes},
	{"rollback_get_last_node_actor", l_rollback_get_last_node_actor},
	{"rollback_revert_actions_by", l_rollback_revert_actions_by},
	{"add_particle", l_add_particle},
	{"add_particlespawner", l_add_particlespawner},
	{"delete_particlespawner", l_delete_particlespawner},
	{NULL, NULL}
};


/*
	Main export function
*/

void scriptapi_export(lua_State *L, Server *server)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	verbosestream<<"scriptapi_export()"<<std::endl;
	StackUnroller stack_unroller(L);

	// Store server as light userdata in registry
	lua_pushlightuserdata(L, server);
	lua_setfield(L, LUA_REGISTRYINDEX, "minetest_server");

	// Register global functions in table minetest
	lua_newtable(L);
	luaL_register(L, NULL, minetest_f);
	lua_setglobal(L, "minetest");

	// Get the main minetest table
	lua_getglobal(L, "minetest");

	// Add tables to minetest
	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");
	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");

	// Register wrappers
	LuaItemStack::Register(L);
	InvRef::Register(L);
	NodeMetaRef::Register(L);
	NodeTimerRef::Register(L);
	ObjectRef::Register(L);
	EnvRef::Register(L);
	LuaPseudoRandom::Register(L);
	LuaPerlinNoise::Register(L);
	LuaPerlinNoiseMap::Register(L);
}
