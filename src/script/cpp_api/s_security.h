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

#ifndef S_SECURITY_H
#define S_SECURITY_H

#include "cpp_api/s_base.h"


#define CHECK_SECURE_PATH(L, path) \
	if (!ScriptApiSecurity::checkPath(L, path)) { \
		throw LuaError(std::string("Attempt to access external file ") + \
					path + " with mod security on."); \
	}
#define CHECK_SECURE_PATH_OPTIONAL(L, path) \
	if (ScriptApiSecurity::isSecure(L)) { \
		CHECK_SECURE_PATH(L, path); \
	}


class ScriptApiSecurity : virtual public ScriptApiBase
{
public:
	// Sets up security on the ScriptApi's Lua state
	void initializeSecurity();
	// Checks if the Lua state has been secured
	static bool isSecure(lua_State *L);
	// Loads a file as Lua code safely (doesn't allow bytecode).
	static bool safeLoadFile(lua_State *L, const char *path);
	// Checks if mods are allowed to read and write to the path
	static bool checkPath(lua_State *L, const char *path);

private:
	// Syntax: "sl_" <Library name or 'g' (global)> '_' <Function name>
	// (sl stands for Secure Lua)

	static int sl_g_dofile(lua_State *L);
	static int sl_g_load(lua_State *L);
	static int sl_g_loadfile(lua_State *L);
	static int sl_g_loadstring(lua_State *L);
	static int sl_g_require(lua_State *L);

	static int sl_io_open(lua_State *L);
	static int sl_io_input(lua_State *L);
	static int sl_io_output(lua_State *L);
	static int sl_io_lines(lua_State *L);

	static int sl_os_rename(lua_State *L);
	static int sl_os_remove(lua_State *L);
};

#endif

