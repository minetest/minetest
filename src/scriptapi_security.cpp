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
#include <map>

#include "scriptapi_security.h"
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

//global variables
/******************************************************************************/
std::map<std::string,lua_CFunction> glb_security_backup;
std::vector<FILE*>                  glb_security_current_files;

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
	replaceLibFunc(L,"io","lines",      l_safe_io_lines);
	replaceLibFunc(L,"io","open",       l_safe_io_open);
	replaceLibFunc(L,"io","popen",      l_forbidden);
	replaceLibFunc(L,"io","read",       l_forbidden);
	replaceLibFunc(L,"io","tmpfile",    l_forbidden);
	replaceLibFunc(L,"io","write",      l_forbidden);
	replaceLibFunc(L,"io","close",      l_safe_io_close);

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
	lua_getglobal(L, libname.c_str());
	if (lua_istable(L,-1)) {
		lua_pushstring(L,funcname.c_str());
		lua_gettable(L,-2);
		if (lua_iscfunction(L,-1)) {
			infostream << "Security: replacing: " << libname << "." << funcname << std::endl;
			glb_security_backup[libname + "." + funcname] = lua_tocfunction(L,-1);
			lua_pushcfunction(L, function);
			lua_settable(L,-3);
		}
		else {
			infostream << "Security: adding: " << libname << "." << funcname << std::endl;
			luaL_Reg toadd[2] = {
					{ funcname.c_str(), function },
					{0,0}
				};

			luaL_register(L, libname.c_str(), toadd);
		}
	}
	else {
		errorstream << "Security: unable to replace/add " << funcname
				<< " for nonexistant lib" << libname << std::endl;
	}
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

/******************************************************************************/
int l_safe_io_lines(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);

	if (inAllowedFolder(L,filename)) {
		return glb_security_backup["io.lines"](L);
	}
	lua_pushnil(L);
	return 1;
}

/******************************************************************************/
int l_safe_io_open(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);

	if (inAllowedFolder(L,filename)) {
		int retval = glb_security_backup["io.open"](L);

		FILE* toadd = *((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE));
		if (toadd != 0)
			glb_security_current_files.push_back(toadd);

		return retval;
	}
	lua_pushnil(L);
	return 1;
}

/******************************************************************************/
int l_safe_io_close(lua_State *L) {
	FILE* toclose = *((FILE **)luaL_checkudata(L, 1, LUA_FILEHANDLE));

	std::vector<FILE*>::iterator pos;

	for (pos = glb_security_current_files.begin();
						pos != glb_security_current_files.end(); pos++) {
		if (*pos == toclose)
			break;
	}

	if (pos != glb_security_current_files.end()) {
		glb_security_current_files.erase(pos);
		return glb_security_backup["io.close"](L);
	}

	return 0;
}
