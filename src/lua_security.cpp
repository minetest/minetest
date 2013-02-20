/*
Minetest-c55
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 sapier, sapier at gmx dot net

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
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <errno.h>

#include <iostream>

#include "lua_security.h"
#include "filesys.h"
#include "mods.h"
#include "server.h"
#include "log.h"
#include "settings.h"
#include "main.h"

//functions in scriptapi.cpp
/******************************************************************************/
extern bool getstringfield(lua_State *L, int table,
		const char *fieldname, std::string &result);

extern Server* get_server(lua_State *L);
extern void script_error(lua_State *L, const char *fmt, ...);

//constant declarations
/******************************************************************************/
const luaL_Reg security_funcs[] =
{
	{"dofile"       , l_safe_dofile },
	{"loadfile"     , l_forbidden },
	{"load"         , l_forbidden },
//	{"rawequal"     , l_safe_rawequal },     unsafe but required for register*
//	{"rawget"       , l_safe_rawget },       unsafe but required for register*
//	{"rawset"       , l_safe_rawset },       unsafe but required for register*
//	{"setfenv"      , l_safe_fenv },         unsafe but required for serialize/deserialize
//	{"setmetatable" , l_safe_setmetatable }, unsafe but required for register*
	{"module"       , l_forbidden },
	{"require"      , l_forbidden },
	{"newproxy"     , l_forbidden },
	{0,0}
};


//functions
/******************************************************************************/
void InitSecurity(lua_State* L) {
	if (!g_settings->getBool("security_safe_mod_api")) {
		infostream << "================================================================================" << std::endl;
		infostream << "=                                                                              =" << std::endl;
		infostream << "= WARNING: Secure mod api disabled, ensure only trusted mods are enabled!!!    =" << std::endl;
		infostream << "=                                                                              =" << std::endl;
		infostream << "================================================================================" << std::endl;
		return;
	}

	luaL_register(L, "_G", security_funcs);
	lua_pop(L, 1);

	//forbid or replace all unsafe functions
	replaceLibFunc(L,"os","execute",    l_forbidden);
	replaceLibFunc(L,"os","exit",       l_forbidden);
	replaceLibFunc(L,"os","getenv",     l_forbidden);
	replaceLibFunc(L,"os","remove",     l_safe_remove);
	replaceLibFunc(L,"os","rmdir",      l_safe_remove);
	replaceLibFunc(L,"os","mkdir",      l_safe_mkdir);
	replaceLibFunc(L,"os","rename",     l_forbidden);
	replaceLibFunc(L,"os","setlocale",  l_forbidden);
	replaceLibFunc(L,"os","tmpname",    l_forbidden); //maybe replace by own fct


	replaceLibFunc(L,"io","input",      l_forbidden);
	replaceLibFunc(L,"io","output",     l_forbidden);
	replaceLibFunc(L,"io","lines",      l_forbidden);
	replaceLibFunc(L,"io","open",       l_safe_open);
	replaceLibFunc(L,"io","popen",      l_forbidden);
	replaceLibFunc(L,"io","read",       l_forbidden);
	replaceLibFunc(L,"io","tmpfile",    l_forbidden);
	replaceLibFunc(L,"io","write",      l_forbidden);
	replaceLibFunc(L,"io","close",      l_forbidden);

	replaceMetatableFunc(L,LUA_FILEHANDLE,"close", l_safe_fclose);

}

/******************************************************************************/
#define ENDSWITH(ending) \
	if (filename.compare(filename.length() - std::string(ending).length(), \
						std::string(ending).length(), \
						ending) == 0) return true

bool isSpecialFile(std::string filename) {

	ENDSWITH("auth.txt");
	ENDSWITH("env_meta.txt");
	ENDSWITH("ipban.txt");
	ENDSWITH("map_meta.txt");
	ENDSWITH("map.sqlite");
	ENDSWITH("rollback.txt");
	ENDSWITH("world.mt");
	ENDSWITH("minetest.conf");

	return false;
}

/******************************************************************************/
bool inAllowedFolder(lua_State* L,std::string filename) {
	std::string absolute_file_path = fs::AbsolutePath(filename);
	std::string absolute_world_path = fs::AbsolutePath(get_server(L)->getWorldPath());
	std::string absolute_user_path = fs::AbsolutePath(porting::path_user);
	std::string absolute_player_path = absolute_world_path + DIR_DELIM + "players";

	if (isSpecialFile(filename))
		return false;

	if (absolute_file_path.compare(0,
			absolute_player_path.length(),absolute_player_path) == 0)
		return false;

	if (absolute_file_path.compare(0,
			absolute_world_path.length(),absolute_world_path) == 0)
		return true;

	if (absolute_file_path.compare(0,
			absolute_user_path.length(),absolute_user_path) == 0)
		return true;

	return false;
}

/******************************************************************************/
bool validate_filename(lua_State *L,std::string filename) {
	std::string modname;
	getstringfield(L, LUA_REGISTRYINDEX, "minetest_current_modname", modname);

	if (modname == "__builtin") {
		return true;
	}

	const ModSpec *mod = get_server(L)->getModSpec(modname);
	if(!mod){
		return false;
	}
	std::string absolute_file_path   = fs::AbsolutePath(filename);
	std::string absolute_mod_path = fs::AbsolutePath(mod->path);

	return (absolute_file_path.compare(0,absolute_mod_path.length(),absolute_mod_path) == 0);
}

/******************************************************************************/
void replaceLibFunc(lua_State *L,
						std::string libname,
						std::string funcname,
						int (function)(lua_State*)) {

	luaL_Reg toreplace[2] = {
			{ funcname.c_str(), function },
			{0,0}
		};

	luaL_register(L, libname.c_str(), toreplace);
	lua_pop(L, 1);
}

/******************************************************************************/
void replaceMetatableFunc(lua_State* L,
							std::string metatablename,
							std::string funcname,
							int (function)(lua_State*)) {

	//get metatable onto stack
	luaL_getmetatable (L,metatablename.c_str());

	luaL_Reg toreplace[2] = {
			{ funcname.c_str(), function },
			{0,0}
		};

	luaL_register(L, NULL, toreplace);

	//switch back to environment
	lua_remove(L, 1);
}
/******************************************************************************/
int l_safe_dofile(lua_State *L) {
	std::string filename = luaL_checkstring(L, 1);

	if (validate_filename(L,filename)) {

		int retval = luaL_loadfile(L, filename.c_str());

		if (retval != 0) {
			std::string errorstring = lua_tostring(L, -1);
			luaL_error(L,errorstring.c_str());
			lua_pushboolean(L,false);
			return 1;
		}

		retval = lua_pcall(L, 0, LUA_MULTRET, 0);

		if (retval == 0) {
			lua_pushboolean(L,true);
		}
		else {
			std::string errorstring = "Unable to run: " + filename + " Detail: " + lua_tostring(L, -1);
			luaL_error(L,errorstring.c_str());
			lua_pushboolean(L,false);
		}
	}
	else {
		script_error(L, "Security: Error running dofile: %s not allowed\n", filename.c_str());
		lua_pushboolean(L,false);
	}
	return 1;
}

/******************************************************************************/
int l_forbidden(lua_State *L) {
	script_error(L, "Security: Function not allowed!\n");
	return 0;
}

/******************************************************************************/
bool isBuiltin(lua_State *L,std::string funcname) {
	std::string modname;
	getstringfield(L, LUA_REGISTRYINDEX, "minetest_current_modname", modname);

	if (modname != "__builtin") {
		script_error(L, "Security: Function \"%s\" not allowed!\n",funcname.c_str());
		return false;
	}

	return true;
}

/******************************************************************************/
int l_safe_rawequal(lua_State *L) {
	//TODO
	return 0;
}

/******************************************************************************/
int l_safe_rawget(lua_State *L) {
	//TODO
	return 0;
}

/******************************************************************************/
int l_safe_rawset(lua_State *L) {
	//TODO
	return 0;
}

/******************************************************************************/
int l_safe_fenv(lua_State *L) {
	//TODO
	return 0;
}

/******************************************************************************/
int l_safe_setmetatable(lua_State *L) {
	//TODO
	return 0;
}

/******************************************************************************/
int l_safe_open(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);

	if (inAllowedFolder(L,filename)) {
		const char *mode = luaL_optstring(L, 2, "r");
		FILE **pf = (FILE **)lua_newuserdata(L, sizeof(FILE *));
		*pf = NULL;  /* file handle is currently `closed' */
		luaL_getmetatable(L, LUA_FILEHANDLE);
		lua_setmetatable(L, -2);
		*pf = fopen(filename, mode);

		if (*pf != NULL) return 1;

		lua_pushboolean(L, false);
		return 1;
	}
	else {
		script_error(L, "Security: some mod tried to access \"%s\"!\n",filename);
	}
	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/
#define tofilep(L)	((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE))
int l_safe_fclose(lua_State *L) {

	FILE **p = tofilep(L);
	int ok = (fclose(*p) == 0);
	*p = NULL;

	int en = errno;  /* calls to Lua API may change this value */
	if (ok) {
		lua_pushboolean(L, true);
		return 1;
		}
	else {
		lua_pushnil(L);
		lua_pushfstring(L, "%s", strerror(en));
		lua_pushinteger(L, en);
		return 3;
	}
}

/******************************************************************************/
int l_safe_remove(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);

	if (inAllowedFolder(L,filename)) {
		lua_pushboolean(L, fs::DeleteSingleFileOrEmptyDirectory(filename));
		return 1;
	}
	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int l_safe_mkdir(lua_State *L) {
	const char *foldername = luaL_checkstring(L, 1);

	if (inAllowedFolder(L,foldername)) {
		lua_pushboolean(L, fs::CreateAllDirs(foldername));
		return 1;
	}
	lua_pushboolean(L,false);
	return 1;
}
