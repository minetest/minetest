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

#include "cpp_api/s_security.h"
#include "filesys.h"
#include "porting.h"
#include "server.h"
#include "main.h"
#include <cerrno>
#include <string>

#define SECURE_LIB_API(lib, name) \
	lua_pushcfunction(L, sl_##lib##_##name);\
	lua_setfield(L, -2, #name);

#define SECURE_API(name) SECURE_LIB_API(g, name)

// Copies from second-top-most to top-most table
#define COPY_SAFE(list) \
	for (unsigned i = 0; i < (sizeof(list) / sizeof(list[0])); i++) {\
		lua_getfield(L, -2, list[i]);\
		lua_setfield(L, -2, list[i]);\
	}

#define GET_ORIGINAL(lib, func) \
	lua_getfield(L, LUA_REGISTRYINDEX, "globals_backup");\
	lua_getfield(L, -1, lib);\
	lua_remove(L, -2);  /* Remove globals_backup */\
	lua_getfield(L, -1, func);\
	lua_remove(L, -2);  /* Remove lib */


void ScriptApiSecurity::initializeSecurity()
{
	static const char * whitelist[] = {
		"assert",
		"core",
		"collectgarbage",
		"DIR_DELIM",
		"error",
		"getfenv",
		"getmetatable",
		"ipairs",
		"next",
		"pairs",
		"pcall",
		"print",
		"rawequal",
		"rawget",
		"rawset",
		"select",
		"setfenv",
		"setmetatable",
		"tonumber",
		"tostring",
		"type",
		"unpack",
		"_VERSION",
		"xpcall",
		"coroutine",
		"string",
		"table",
		"math",
	};
	static const char * io_whitelist[] = {
		"close",
		"flush",
		"read",
		"type",
		"write",
	};
	static const char * os_whitelist[] = {
		"clock",
		"date",
		"difftime",
		"exit",
		"getenv",
		"setlocale",
		"time",
		"tmpname",
	};
	static const char * debug_whitelist[] = {
		"gethook",
		"traceback",
		"getinfo",
		"getmetatable",
		"setupvalue",
		"setmetatable",
		"upvalueid",
		"upvaluejoin",
		"sethook",
		"debug",
		"getupvalue",
		"setlocal",
	};
	static const char * package_whitelist[] = {
		"config",
		"searchpath",
		"cpath",
		"path",
	};
	static const char * jit_whitelist[] = {
		"arch",
		"flush",
		"off",
		"on",
		"opt",
		"os",
		"status",
		"version",
		"version_num",
	};

	m_secure = true;

	lua_State * L = getStack();

	// Backup globals to the registry
	lua_getglobal(L, "_G");
	lua_setfield(L, LUA_REGISTRYINDEX, "globals_backup");

	// Replace the global environment with an empty one
#if LUA_VERSION_NUM <= 501
	int is_main = lua_pushthread(L);  // Push the main thread
	assert(is_main);
#endif
	lua_newtable(L);  // Create new environment
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "_G");  // Set _G of new environment
#if LUA_VERSION_NUM >= 502  // Lua >= 5.2
	// Set the global environment
	lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
#else  // Lua <= 5.1
	// Set the environment of the main thread
	int ok = lua_setfenv(L, -2);
	assert(ok);
	lua_pop(L, 1);  // Pop thread
#endif

	// Copy over safe things
	lua_getfield(L, LUA_REGISTRYINDEX, "globals_backup");

	// Copy safe globals
	lua_getglobal(L, "_G");
	COPY_SAFE(whitelist);
	// And replace unsafe ones
	SECURE_API(dofile);
	SECURE_API(load);
	SECURE_API(loadfile);
	SECURE_API(loadstring);
	SECURE_API(require);
	lua_pop(L, 1);

	// Copy safe IO functions
	lua_getfield(L, -1, "io");
	lua_newtable(L);
	COPY_SAFE(io_whitelist);
	// And replace unsafe ones
	SECURE_LIB_API(io, open);
	SECURE_LIB_API(io, input);
	SECURE_LIB_API(io, output);
	SECURE_LIB_API(io, lines);
	lua_setglobal(L, "io");
	lua_pop(L, 1);  // Pop backup IO

	// Copy safe OS functions
	lua_getfield(L, -1, "os");
	lua_newtable(L);
	COPY_SAFE(os_whitelist);
	// And replace unsafe ones
	SECURE_LIB_API(os, remove);
	SECURE_LIB_API(os, rename);
	lua_setglobal(L, "os");
	lua_pop(L, 1);  // Pop backup OS

	// Copy safe debug functions
	lua_getfield(L, -1, "debug");
	lua_newtable(L);
	COPY_SAFE(debug_whitelist);
	lua_setglobal(L, "debug");
	lua_pop(L, 1);  // Pop backup debug

	// Copy safe package fields
	lua_getfield(L, -1, "package");
	lua_newtable(L);
	COPY_SAFE(package_whitelist);
	lua_setglobal(L, "package");
	lua_pop(L, 1);  // Pop backup package

	// Copy safe jit functions, if they exist
	lua_getfield(L, -1, "jit");
	if (!lua_isnil(L, -1)) {
		lua_newtable(L);
		COPY_SAFE(jit_whitelist);
		lua_setglobal(L, "jit");
	}
	lua_pop(L, 1);  // Pop backup jit

	lua_pop(L, 1); // Pop globals_backup
}


bool ScriptApiSecurity::isSecure(lua_State * L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "globals_backup");
	bool secure = !lua_isnil(L, -1);
	lua_pop(L, 1);
	return secure;
}


#define CHECK_FILE_ERR(ret, fp) \
	if (ret) {\
		if (fp) std::fclose(fp);\
		lua_pushfstring(L, "%s: %s", path, strerror(errno));\
		return false;\
	}


bool ScriptApiSecurity::safeLoadFile(lua_State * L, const char * path)
{
	FILE * fp;
	char * chunk_name;
	if (path == NULL) {
		fp = stdin;
		chunk_name = const_cast<char *>("=stdin");
	} else {
		fp = fopen(path, "r");
		CHECK_FILE_ERR(!fp, fp)
		chunk_name = new char[strlen(path) + 2];
		chunk_name[0] = '@';
		chunk_name[1] = '\0';
		strcat(chunk_name, path);
	}

	size_t start = 0;
	int c = std::getc(fp);
	if (c == '#') {
		// Skip the first line
		while ((c = std::getc(fp)) != EOF && c != '\n');
		if (c == '\n') c = std::getc(fp);
		start = std::ftell(fp);
	}

	if (c == LUA_SIGNATURE[0]) {
		lua_pushliteral(L, "Bytecode prohibited when mod security is enabled.");
		return false;
	}

	// Read the file
	int ret = std::fseek(fp, 0, SEEK_END);
	CHECK_FILE_ERR(ret, fp);
	size_t size = std::ftell(fp) - start;
	char * code = new char[size];
	ret = std::fseek(fp, start, SEEK_SET);
	CHECK_FILE_ERR(ret, fp);
	size_t num_read = std::fread(code, 1, size, fp);
	if (path) {
		std::fclose(fp);
	}
	if (num_read != size) {
		lua_pushliteral(L, "Error reading file to load.");
		return false;
	}

	if (luaL_loadbuffer(L, code, size, chunk_name)) {
		return false;
	}

	if (path) {
		delete [] chunk_name;
	}
	return true;
}


#define ALLOW_IN_PATH(allow_path) \
	str = fs::AbsolutePath(allow_path);\
	if (!str.empty() &&\
			fs::PathStartsWith(abs_path, str)) {\
		return true;\
	}

#define DISALLOW_IN_PATH(disallow_path) \
	str = fs::AbsolutePath(disallow_path);\
	if (str.empty() ||\
			fs::PathStartsWith(abs_path, str)) {\
		return false;\
	}

bool ScriptApiSecurity::checkPath(lua_State * L, const char * path)
{
	std::string str;  // Transient

	// Get absolute path
	std::string abs_path = fs::AbsolutePath(path);
	if (!abs_path.empty()) {
		// Don't allow accessing the settings file
		str = fs::AbsolutePath(g_settings_path);
		if (str == abs_path) {
			return false;
		}
	}

	fs::RemoveLastPathComponent(path, &str);
	// Don't allow accessing minetest.conf (main or of games)
	if (str == "minetest.conf") {
		return false;
	}
	// Remove last components if they don't exist to allow opening
	// non-existing files/folders.
	std::string cur_path = path;
	while (abs_path.empty() && !cur_path.empty()) {
		cur_path = fs::RemoveLastPathComponent(cur_path);
		abs_path = fs::AbsolutePath(cur_path);
	}

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "scriptapi");
	ScriptApiBase *script = (ScriptApiBase*) lua_touserdata(L, -1);
	lua_pop(L, 1);
	const Server *server = script->getServer();

	if (server) {
		// Allow paths in world path
		ALLOW_IN_PATH(server->getWorldPath());

		// Get modname and allow paths in modpath if possible
		lua_getfield(L, LUA_REGISTRYINDEX, "current_modname");
		if (lua_isstring(L, -1)) {
			std::string modname = lua_tostring(L, -1);
			const ModSpec *mod = server->getModSpec(modname);
			if (mod) {
				ALLOW_IN_PATH(mod->path);
			}
		}
		lua_pop(L, 1);
	}

	// Don't allow accessing binaries
	DISALLOW_IN_PATH(porting::path_share + DIR_DELIM "bin");

	// Don't allow accessing utility scripts
	DISALLOW_IN_PATH(porting::path_share + DIR_DELIM "util");

	// Allow paths in user path
	ALLOW_IN_PATH(porting::path_user);

	// Allow paths in share path
	ALLOW_IN_PATH(porting::path_share);

	return false;
}


int ScriptApiSecurity::sl_g_dofile(lua_State * L)
{
	int nret = sl_g_loadfile(L);
	if (nret != 1) {
		return nret;
	}
	int top_precall = lua_gettop(L);
	lua_call(L, 0, LUA_MULTRET);
	// Return number of arguments returned by the function,
	// adjusting for the function being poped.
	return lua_gettop(L) - (top_precall - 1);
}


int ScriptApiSecurity::sl_g_load(lua_State * L)
{
	size_t len;
	const char * buf;
	std::string code;
	const char * chunk_name = "=(load)";

	luaL_checktype(L, 1, LUA_TFUNCTION);
	if (!lua_isnone(L, 2)) {
		luaL_checktype(L, 2, LUA_TSTRING);
		chunk_name = lua_tostring(L, 2);
	}

	while (true) {
		lua_pushvalue(L, 1);
		lua_call(L, 0, 1);
		int t = lua_type(L, -1);
		if (t == LUA_TNIL) {
			break;
		} else if (t != LUA_TSTRING) {
			lua_pushnil(L);
			lua_pushliteral(L, "Loader didn't return a string");
			return 2;
		}
		buf = lua_tolstring(L, -1, &len);
		code += std::string(buf, len);
		lua_pop(L, 1); // Pop return value
	}
	if (code[0] == LUA_SIGNATURE[0]) {
		lua_pushnil(L);
		lua_pushliteral(L, "Bytecode prohibited when mod security is enabled.");
		return 2;
	}
	if (luaL_loadbuffer(L, code.data(), code.size(), chunk_name)) {
		lua_pushnil(L);
		lua_insert(L, lua_gettop(L) - 1);
		return 2;
	}
	return 1;
}


int ScriptApiSecurity::sl_g_loadfile(lua_State * L)
{
	const char * path = NULL;

	if (lua_isstring(L, 1)) {
		path = lua_tostring(L, 1);
		CHECK_SECURE_PATH(L, path);
	}

	if (!safeLoadFile(L, path)) {
		lua_pushnil(L);
		lua_insert(L, -2);
		return 2;
	}

	return 1;
}


int ScriptApiSecurity::sl_g_loadstring(lua_State * L)
{
	const char * chunk_name = "=(load)";

	luaL_checktype(L, 1, LUA_TSTRING);
	if (!lua_isnone(L, 2)) {
		luaL_checktype(L, 2, LUA_TSTRING);
		chunk_name = lua_tostring(L, 2);
	}

	size_t size;
	const char * code = lua_tolstring(L, 1, &size);

	if (size > 0 && code[0] == LUA_SIGNATURE[0]) {
		lua_pushnil(L);
		lua_pushliteral(L, "Bytecode prohibited when mod security is enabled.");
		return 2;
	}
	if (luaL_loadbuffer(L, code, size, chunk_name)) {
		lua_pushnil(L);
		lua_insert(L, lua_gettop(L) - 1);
		return 2;
	}
	return 1;
}


int ScriptApiSecurity::sl_g_require(lua_State * L)
{
	lua_pushliteral(L, "require() is disabled when mod security is on.");
	return lua_error(L);
}


int ScriptApiSecurity::sl_io_open(lua_State * L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char * path = lua_tostring(L, 1);
	CHECK_SECURE_PATH(L, path);

	GET_ORIGINAL("io", "open");
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_call(L, 2, 2);
	return 2;
}


int ScriptApiSecurity::sl_io_input(lua_State * L)
{
	if (lua_isstring(L, 1)) {
		const char * path = lua_tostring(L, 1);
		CHECK_SECURE_PATH(L, path);
	}

	GET_ORIGINAL("io", "input");
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}


int ScriptApiSecurity::sl_io_output(lua_State * L)
{
	if (lua_isstring(L, 1)) {
		const char * path = lua_tostring(L, 1);
		CHECK_SECURE_PATH(L, path);
	}

	GET_ORIGINAL("io", "output");
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}


int ScriptApiSecurity::sl_io_lines(lua_State * L)
{
	if (lua_isstring(L, 1)) {
		const char * path = lua_tostring(L, 1);
		CHECK_SECURE_PATH(L, path);
	}

	GET_ORIGINAL("io", "lines");
	lua_pushvalue(L, 1);
	int top_precall = lua_gettop(L);
	lua_call(L, 1, LUA_MULTRET);
	// Return number of arguments returned by the function,
	// adjusting for the function being poped.
	return lua_gettop(L) - (top_precall - 1);
}


int ScriptApiSecurity::sl_os_rename(lua_State * L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char * path1 = lua_tostring(L, 1);
	CHECK_SECURE_PATH(L, path1);

	luaL_checktype(L, 2, LUA_TSTRING);
	const char * path2 = lua_tostring(L, 2);
	CHECK_SECURE_PATH(L, path2);

	GET_ORIGINAL("os", "rename");
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_call(L, 2, 2);
	return 2;
}


int ScriptApiSecurity::sl_os_remove(lua_State * L)
{
	luaL_checktype(L, 1, LUA_TSTRING);
	const char * path = lua_tostring(L, 1);
	CHECK_SECURE_PATH(L, path);

	GET_ORIGINAL("os", "remove");
	lua_pushvalue(L, 1);
	lua_call(L, 1, 2);
	return 2;
}

