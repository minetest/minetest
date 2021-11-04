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

#include "irrlichttypes_extrabloated.h"
#include "lua_api/l_util.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_settings.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "cpp_api/s_async.h"
#include "serialization.h"
#include <json/json.h>
#include "cpp_api/s_security.h"
#include "porting.h"
#include "convert_json.h"
#include "debug.h"
#include "log.h"
#include "tool.h"
#include "filesys.h"
#include "settings.h"
#include "util/auth.h"
#include "util/base64.h"
#include "config.h"
#include "version.h"
#include "util/hex.h"
#include "util/sha1.h"
#include "util/png.h"
#include <algorithm>
#include <cstdio>

// log([level,] text)
// Writes a line to the logger.
// The one-argument version logs to LL_NONE.
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
			log_deprecated(L, text, 2);
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
		std::istringstream stream(jsonstr);

		Json::CharReaderBuilder builder;
		builder.settings_["collectComments"] = false;
		std::string errs;

		if (!Json::parseFromStream(builder, stream, &root, &errs)) {
			errorstream << "Failed to parse json data " << errs << std::endl;
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
		styled = readParam<bool>(L, 2);
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
		out = root.toStyledString();
	} else {
		out = fastWriteJson(root);
	}
	lua_pushlstring(L, out.c_str(), out.size());
	return 1;
}

// get_dig_params(groups, tool_capabilities[, wear])
int ModApiUtil::l_get_dig_params(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ItemGroupList groups;
	read_groups(L, 1, groups);
	ToolCapabilities tp = read_tool_capabilities(L, 2);
	if (lua_isnoneornil(L, 3)) {
		push_dig_params(L, getDigParams(groups, &tp));
	} else {
		u16 wear = readParam<int>(L, 3);
		push_dig_params(L, getDigParams(groups, &tp, wear));
	}
	return 1;
}

// get_hit_params(groups, tool_capabilities[, time_from_last_punch, [, wear]])
int ModApiUtil::l_get_hit_params(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::unordered_map<std::string, int> groups;
	read_groups(L, 1, groups);
	ToolCapabilities tp = read_tool_capabilities(L, 2);
	float time_from_last_punch = readParam<float>(L, 3, 1000000);
	int wear = readParam<int>(L, 4, 0);
	push_hit_params(L, getHitParams(groups, &tp,
		time_from_last_punch, wear));
	return 1;
}

// check_password_entry(name, entry, password)
int ModApiUtil::l_check_password_entry(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	std::string entry = luaL_checkstring(L, 2);
	std::string password = luaL_checkstring(L, 3);

	if (base64_is_valid(entry)) {
		std::string hash = translate_password(name, password);
		lua_pushboolean(L, hash == entry);
		return 1;
	}

	std::string salt;
	std::string verifier;

	if (!decode_srp_verifier_and_salt(entry, &verifier, &salt)) {
		// invalid format
		warningstream << "Invalid password format for " << name << std::endl;
		lua_pushboolean(L, false);
		return 1;
	}
	std::string gen_verifier = generate_srp_verifier(name, password, salt);

	lua_pushboolean(L, gen_verifier == verifier);
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
	std::string str = readParam<std::string>(L, -1); // get result
	lua_pop(L, 1);

	bool yes = is_yes(str);
	lua_pushboolean(L, yes);
	return 1;
}

// get_builtin_path()
int ModApiUtil::l_get_builtin_path(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::string path = porting::path_share + DIR_DELIM + "builtin" + DIR_DELIM;
	lua_pushstring(L, path.c_str());

	return 1;
}

// get_user_path()
int ModApiUtil::l_get_user_path(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::string path = porting::path_user;
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
	if (!lua_isnoneornil(L, 3))
		level = readParam<int>(L, 3);

	std::ostringstream os(std::ios_base::binary);
	compressZlib(reinterpret_cast<const u8 *>(data), size, os, level);

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

	std::istringstream is(std::string(data, size), std::ios_base::binary);
	std::ostringstream os(std::ios_base::binary);
	decompressZlib(is, os);

	std::string out = os.str();

	lua_pushlstring(L, out.data(), out.size());
	return 1;
}

// encode_base64(string)
int ModApiUtil::l_encode_base64(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	size_t size;
	const char *data = luaL_checklstring(L, 1, &size);

	std::string out = base64_encode((const unsigned char *)(data), size);

	lua_pushlstring(L, out.data(), out.size());
	return 1;
}

// decode_base64(string)
int ModApiUtil::l_decode_base64(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	size_t size;
	const char *d = luaL_checklstring(L, 1, &size);
	const std::string data = std::string(d, size);

	if (!base64_is_valid(data))
		return 0;

	std::string out = base64_decode(data);

	lua_pushlstring(L, out.data(), out.size());
	return 1;
}

// mkdir(path)
int ModApiUtil::l_mkdir(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *path = luaL_checkstring(L, 1);
	CHECK_SECURE_PATH(L, path, true);
	lua_pushboolean(L, fs::CreateAllDirs(path));
	return 1;
}

// get_dir_list(path, is_dir)
int ModApiUtil::l_get_dir_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *path = luaL_checkstring(L, 1);
	bool list_all = !lua_isboolean(L, 2); // if its not a boolean list all
	bool list_dirs = readParam<bool>(L, 2); // true: list dirs, false: list files

	CHECK_SECURE_PATH(L, path, false);

	std::vector<fs::DirListNode> list = fs::GetDirListing(path);

	int index = 0;
	lua_newtable(L);

	for (const fs::DirListNode &dln : list) {
		if (list_all || list_dirs == dln.dir) {
			lua_pushstring(L, dln.name.c_str());
			lua_rawseti(L, -2, ++index);
		}
	}

	return 1;
}

// safe_file_write(path, content)
int ModApiUtil::l_safe_file_write(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *path = luaL_checkstring(L, 1);
	size_t size;
	const char *content = luaL_checklstring(L, 2, &size);

	CHECK_SECURE_PATH(L, path, true);

	bool ret = fs::safeWriteToFile(path, std::string(content, size));
	lua_pushboolean(L, ret);

	return 1;
}

// request_insecure_environment()
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
	std::string mod_name = readParam<std::string>(L, -1);
	std::string trusted_mods = g_settings->get("secure.trusted_mods");
	trusted_mods.erase(std::remove_if(trusted_mods.begin(),
			trusted_mods.end(), static_cast<int(*)(int)>(&std::isspace)),
			trusted_mods.end());
	std::vector<std::string> mod_list = str_split(trusted_mods, ',');
	if (std::find(mod_list.begin(), mod_list.end(), mod_name) ==
			mod_list.end()) {
		return 0;
	}

	// Push insecure environment
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_GLOBALS_BACKUP);
	return 1;
}

// get_version()
int ModApiUtil::l_get_version(lua_State *L)
{
	lua_createtable(L, 0, 3);
	int table = lua_gettop(L);

	lua_pushstring(L, PROJECT_NAME_C);
	lua_setfield(L, table, "project");

	lua_pushstring(L, g_version_string);
	lua_setfield(L, table, "string");

	if (strcmp(g_version_string, g_version_hash) != 0) {
		lua_pushstring(L, g_version_hash);
		lua_setfield(L, table, "hash");
	}

	return 1;
}

int ModApiUtil::l_sha1(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	size_t size;
	const char *data = luaL_checklstring(L, 1, &size);
	bool hex = !lua_isboolean(L, 2) || !readParam<bool>(L, 2);

	// Compute actual checksum of data
	std::string data_sha1;
	{
		SHA1 ctx;
		ctx.addBytes(data, size);
		unsigned char *data_tmpdigest = ctx.getDigest();
		data_sha1.assign((char*) data_tmpdigest, 20);
		free(data_tmpdigest);
	}

	if (hex) {
		std::string sha1_hex = hex_encode(data_sha1);
		lua_pushstring(L, sha1_hex.c_str());
	} else {
		lua_pushlstring(L, data_sha1.data(), data_sha1.size());
	}

	return 1;
}

// colorspec_to_colorstring(colorspec)
int ModApiUtil::l_colorspec_to_colorstring(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::SColor color(0);
	if (read_color(L, 1, &color)) {
		char colorstring[10];
		snprintf(colorstring, 10, "#%02X%02X%02X%02X",
			color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha());
		lua_pushstring(L, colorstring);
		return 1;
	}

	return 0;
}

// colorspec_to_bytes(colorspec)
int ModApiUtil::l_colorspec_to_bytes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	video::SColor color(0);
	if (read_color(L, 1, &color)) {
		u8 colorbytes[4] = {
			(u8) color.getRed(),
			(u8) color.getGreen(),
			(u8) color.getBlue(),
			(u8) color.getAlpha(),
		};
		lua_pushlstring(L, (const char*) colorbytes, 4);
		return 1;
	}

	return 0;
}

// encode_png(w, h, data, level)
int ModApiUtil::l_encode_png(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	// The args are already pre-validated on the lua side.
	u32 width = readParam<int>(L, 1);
	u32 height = readParam<int>(L, 2);
	const char *data = luaL_checklstring(L, 3, NULL);
	s32 compression = readParam<int>(L, 4);

	std::string out = encodePNG((const u8*)data, width, height, compression);

	lua_pushlstring(L, out.data(), out.size());
	return 1;
}

// get_last_run_mod()
int ModApiUtil::l_get_last_run_mod(lua_State *L)
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
int ModApiUtil::l_set_last_run_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	const char *mod = luaL_checkstring(L, 1);
	getScriptApiBase(L)->setOriginDirect(mod);
	return 0;
}

void ModApiUtil::Initialize(lua_State *L, int top)
{
	API_FCT(log);

	API_FCT(get_us_time);

	API_FCT(parse_json);
	API_FCT(write_json);

	API_FCT(get_dig_params);
	API_FCT(get_hit_params);

	API_FCT(check_password_entry);
	API_FCT(get_password_hash);

	API_FCT(is_yes);

	API_FCT(get_builtin_path);
	API_FCT(get_user_path);

	API_FCT(compress);
	API_FCT(decompress);

	API_FCT(mkdir);
	API_FCT(get_dir_list);
	API_FCT(safe_file_write);

	API_FCT(request_insecure_environment);

	API_FCT(encode_base64);
	API_FCT(decode_base64);

	API_FCT(get_version);
	API_FCT(sha1);
	API_FCT(colorspec_to_colorstring);
	API_FCT(colorspec_to_bytes);

	API_FCT(encode_png);

	API_FCT(get_last_run_mod);
	API_FCT(set_last_run_mod);

	LuaSettings::create(L, g_settings, g_settings_path);
	lua_setfield(L, top, "settings");
}

void ModApiUtil::InitializeClient(lua_State *L, int top)
{
	API_FCT(log);

	API_FCT(get_us_time);

	API_FCT(parse_json);
	API_FCT(write_json);

	API_FCT(is_yes);

	API_FCT(compress);
	API_FCT(decompress);

	API_FCT(encode_base64);
	API_FCT(decode_base64);

	API_FCT(get_version);
	API_FCT(sha1);
	API_FCT(colorspec_to_colorstring);
	API_FCT(colorspec_to_bytes);
}

void ModApiUtil::InitializeAsync(lua_State *L, int top)
{
	API_FCT(log);

	API_FCT(get_us_time);

	API_FCT(parse_json);
	API_FCT(write_json);

	API_FCT(is_yes);

	API_FCT(get_builtin_path);
	API_FCT(get_user_path);

	API_FCT(compress);
	API_FCT(decompress);

	API_FCT(mkdir);
	API_FCT(get_dir_list);

	API_FCT(encode_base64);
	API_FCT(decode_base64);

	API_FCT(get_version);
	API_FCT(sha1);
	API_FCT(colorspec_to_colorstring);
	API_FCT(colorspec_to_bytes);

	API_FCT(get_last_run_mod);
	API_FCT(set_last_run_mod);

	LuaSettings::create(L, g_settings, g_settings_path);
	lua_setfield(L, top, "settings");
}
