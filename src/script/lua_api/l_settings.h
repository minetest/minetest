/*
Minetest
Copyright (C) 2013 PilzAdam <pilzadam@minetest.net>

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

#ifndef L_SETTINGS_H_
#define L_SETTINGS_H_

#include "lua_api/l_base.h"

class Settings;

class LuaSettings : public ModApiBase {
private:
	static const char className[];
	static const luaL_reg methods[];

	// garbage collector
	static int gc_object(lua_State* L);

	// get(self, key) -> value
	static int l_get(lua_State* L);

	// get_bool(self, key) -> boolean
	static int l_get_bool(lua_State* L);

	// set(self, key, value)
	static int l_set(lua_State* L);

	// remove(self, key) -> success
	static int l_remove(lua_State* L);

	// get_names(self) -> {key1, ...}
	static int l_get_names(lua_State* L);

	// write(self) -> success
	static int l_write(lua_State* L);

	// to_table(self) -> {[key1]=value1,...}
	static int l_to_table(lua_State* L);

	Settings* m_settings;
	std::string m_filename;

public:
	LuaSettings(const char* filename);
	~LuaSettings();

	// LuaSettings(filename)
	// Creates an LuaSettings and leaves it on top of stack
	static int create_object(lua_State* L);

	static LuaSettings* checkobject(lua_State* L, int narg);

	static void Register(lua_State* L);

};

#endif
