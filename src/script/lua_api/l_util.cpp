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

#include "lua_api/l_util.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "cpp_api/s_async.h"
#include "serialization.h"
#include "json/json.h"
#include "cpp_api/s_security.h"
#include "porting.h"
#include "debug.h"
#include "log.h"
#include "tool.h"
#include "filesys.h"
#include "settings.h"
#include "util/auth.h"
#include <algorithm>

// log([level,] text)
// Writes a line to the logger.
// The one-argument version logs to infostream.
// The two-argument version accepts a log level.
// Either the special case "deprecated" for deprecation notices, or any specified in
// Logger::stringToLevel(name).
int ModApiUtil::l_log(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string text;
	LogLevel level = LL_NONE;
	if (lua_isnone(L, 2)) {
		text = luaL_checkstring(L, 1);
	} else {
		std::string name = luaL_checkstring(L, 1);
		text = luaL_checkstring(L, 2);
		if (name == "deprecated") {
			log_deprecated(L, text);
			return 0;
		}
		level = Logger::stringToLevel(name);
		if (level == LL_MAX) {
			warningstream << "Tried to log at unknown level '" << name
				<< "'.  Defaulting to \"none\"." << std::endl;
			level = LL_NONE;
		}
	}
	g_logger.log(level, text);
	return 0;
}

// get_us_time()
int ModApiUtil::l_get_us_time(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushnumber(L, porting::getTimeUs());
	return 1;
}

#define CHECK_SECURE_SETTING(L, name) \
	if (ScriptApiSecurity::isSecure(L) && \
			name.compare(0, 7, "secure.") == 0) { \
		throw LuaError("Attempt to set secure setting."); \
	}

// setting_set(name, value)
int ModApiUtil::l_setting_set(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	std::string value = luaL_checkstring(L, 2);
	CHECK_SECURE_SETTING(L, name);
	g_settings->set(name, value);
	return 0;
}

// setting_get(name)
int ModApiUtil::l_setting_get(lua_State *L)
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

// setting_setbool(name)
int ModApiUtil::l_setting_setbool(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	bool value = lua_toboolean(L, 2);
	CHECK_SECURE_SETTING(L, name);
	g_settings->setBool(name, value);
	return 0;
}

// setting_getbool(name)
int ModApiUtil::l_setting_getbool(lua_State *L)
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
int ModApiUtil::l_setting_save(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	if(g_settings_path != "")
		g_settings->updateConfigFile(g_settings_path.c_str());
	return 0;
}

// parse_json(str[, nullvalue])
int ModApiUtil::l_parse_json(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	const char *jsonstr = luaL_checkstring(L, 1);

	// Use passed nullvalue or default to nil
	int nullindex = 2;
	if (lua_isnone(L, nullindex)) {
		lua_pushnil(L);
		nullindex = lua_gettop(L);
	}

	Json::Value root;

	{
		Json::Reader reader;
		std::istringstream stream(jsonstr);

		if (!reader.parse(stream, root)) {
			errorstream << "Failed to parse json data "
				<< reader.getFormattedErrorMessages();
			size_t jlen = strlen(jsonstr);
			if (jlen > 100) {
				errorstream << "Data (" << jlen
					<< " bytes) printed to warningstream." << std::endl;
				warningstream << "data: \"" << jsonstr << "\"" << std::endl;
			} else {
				errorstream << "data: \"" << jsonstr << "\"" << std::endl;
			}
			lua_pushnil(L);
			return 1;
		}
	}

	if (!push_json_value(L, root, nullindex)) {
		errorstream << "Failed to parse json data, "
			<< "depth exceeds lua stack limit" << std::endl;
		errorstream << "data: \"" << jsonstr << "\"" << std::endl;
		lua_pushnil(L);
	}
	return 1;
}

// write_json(data[, styled]) -> string or nil and error message
int ModApiUtil::l_write_json(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	bool styled = false;
	if (!lua_isnone(L, 2)) {
		styled = lua_toboolean(L, 2);
		lua_pop(L, 1);
	}

	Json::Value root;
	try {
		read_json_value(L, root, 1);
	} catch (SerializationError &e) {
		lua_pushnil(L);
		lua_pushstring(L, e.what());
		return 2;
	}

	std::string out;
	if (styled) {
		Json::StyledWriter writer;
		out = writer.write(root);
	} else {
		Json::FastWriter writer;
		out = writer.write(root);
	}
	lua_pushlstring(L, out.c_str(), out.size());
	return 1;
}

// get_dig_params(groups, tool_capabilities[, time_from_last_punch])
int ModApiUtil::l_get_dig_params(lua_State *L)
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
int ModApiUtil::l_get_hit_params(lua_State *L)
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

// get_password_hash(name, raw_password)
int ModApiUtil::l_get_password_hash(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	std::string raw_password = luaL_checkstring(L, 2);
	std::string hash = translate_password(name, raw_password);
	lua_pushstring(L, hash.c_str());
	return 1;
}

// is_yes(arg)
int ModApiUtil::l_is_yes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	lua_getglobal(L, "tostring"); // function to be called
	lua_pushvalue(L, 1); // 1st argument
	lua_call(L, 1, 1); // execute function
	std::string str(lua_tostring(L, -1)); // get result
	lua_pop(L, 1);

	bool yes = is_yes(str);
	lua_pushboolean(L, yes);
	return 1;
}

int ModApiUtil::l_get_builtin_path(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::string path = porting::path_share + DIR_DELIM + "builtin";
	lua_pushstring(L, path.c_str());
	return 1;
}

// compress(data, method, level)
int ModApiUtil::l_compress(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	size_t size;
	const char *data = luaL_checklstring(L, 1, &size);

	int level = -1;
	if (!lua_isnone(L, 3) && !lua_isnil(L, 3))
		level = luaL_checknumber(L, 3);

	std::ostringstream os;
	compressZlib(std::string(data, size), os, level);

	std::string out = os.str();

	lua_pushlstring(L, out.data(), out.size());
	return 1;
}

// decompress(data, method)
int ModApiUtil::l_decompress(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	size_t size;
	const char *data = luaL_checklstring(L, 1, &size);

	std::istringstream is(std::string(data, size));
	std::ostringstream os;
	decompressZlib(is, os);

	std::string out = os.str();

	lua_pushlstring(L, out.data(), out.size());
	return 1;
}

// mkdir(path)
int ModApiUtil::l_mkdir(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *path = luaL_checkstring(L, 1);
	CHECK_SECURE_PATH_OPTIONAL(L, path);
	lua_pushboolean(L, fs::CreateAllDirs(path));
	return 1;
}

// get_dir_list(path, is_dir)
int ModApiUtil::l_get_dir_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *path = luaL_checkstring(L, 1);
	short is_dir = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : -1;

	CHECK_SECURE_PATH_OPTIONAL(L, path);

	std::vector<fs::DirListNode> list = fs::GetDirListing(path);

	int index = 0;
	lua_newtable(L);

	for (size_t i = 0; i < list.size(); i++) {
		if (is_dir == -1 || is_dir == list[i].dir) {
			lua_pushstring(L, list[i].name.c_str());
			lua_rawseti(L, -2, ++index);
		}
	}

	return 1;
}

int ModApiUtil::l_request_insecure_environment(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	// Just return _G if security is disabled
	if (!ScriptApiSecurity::isSecure(L)) {
		lua_getglobal(L, "_G");
		return 1;
	}

	// We have to make sure that this function is being called directly by
	// a mod, otherwise a malicious mod could override this function and
	// steal its return value.
	lua_Debug info;
	// Make sure there's only one item below this function on the stack...
	if (lua_getstack(L, 2, &info)) {
		return 0;
	}
	FATAL_ERROR_IF(!lua_getstack(L, 1, &info), "lua_getstack() failed");
	FATAL_ERROR_IF(!lua_getinfo(L, "S", &info), "lua_getinfo() failed");
	// ...and that that item is the main file scope.
	if (strcmp(info.what, "main") != 0) {
		return 0;
	}

	// Get mod name
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	if (!lua_isstring(L, -1)) {
		return 0;
	}

	// Check secure.trusted_mods
	const char *mod_name = lua_tostring(L, -1);
	std::string trusted_mods = g_settings->get("secure.trusted_mods");
	trusted_mods.erase(std::remove(trusted_mods.begin(),
			trusted_mods.end(), ' '), trusted_mods.end());
	std::vector<std::string> mod_list = str_split(trusted_mods, ',');
	if (std::find(mod_list.begin(), mod_list.end(), mod_name) ==
			mod_list.end()) {
		return 0;
	}

	// Push insecure environment
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_GLOBALS_BACKUP);
	return 1;
}


void ModApiUtil::Initialize(lua_State *L, int top)
{
	API_FCT(log);

	API_FCT(get_us_time);

	API_FCT(setting_set);
	API_FCT(setting_get);
	API_FCT(setting_setbool);
	API_FCT(setting_getbool);
	API_FCT(setting_save);

	API_FCT(parse_json);
	API_FCT(write_json);

	API_FCT(get_dig_params);
	API_FCT(get_hit_params);

	API_FCT(get_password_hash);

	API_FCT(is_yes);

	API_FCT(get_builtin_path);

	API_FCT(compress);
	API_FCT(decompress);

	API_FCT(mkdir);
	API_FCT(get_dir_list);

	API_FCT(request_insecure_environment);
}

void ModApiUtil::InitializeAsync(AsyncEngine& engine)
{
	ASYNC_API_FCT(log);

	ASYNC_API_FCT(get_us_time);

	//ASYNC_API_FCT(setting_set);
	ASYNC_API_FCT(setting_get);
	//ASYNC_API_FCT(setting_setbool);
	ASYNC_API_FCT(setting_getbool);
	//ASYNC_API_FCT(setting_save);

	ASYNC_API_FCT(parse_json);
	ASYNC_API_FCT(write_json);

	ASYNC_API_FCT(is_yes);

	ASYNC_API_FCT(get_builtin_path);

	ASYNC_API_FCT(compress);
	ASYNC_API_FCT(decompress);

	ASYNC_API_FCT(mkdir);
	ASYNC_API_FCT(get_dir_list);
}

