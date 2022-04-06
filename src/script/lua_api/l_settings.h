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

#pragma once

#include "common/c_content.h"
#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class Settings;

class LuaSettings : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	// get(self, key) -> value
	ENTRY_POINT_DECL(l_get);

	// get_bool(self, key) -> boolean
	ENTRY_POINT_DECL(l_get_bool);

	// get_np_group(self, key) -> noiseparam
	ENTRY_POINT_DECL(l_get_np_group);

	// get_flags(self, key) -> key/value table
	ENTRY_POINT_DECL(l_get_flags);

	// set(self, key, value)
	ENTRY_POINT_DECL(l_set);

	// set_bool(self, key, value)
	ENTRY_POINT_DECL(l_set_bool);

	// set_np_group(self, key, value)
	ENTRY_POINT_DECL(l_set_np_group);

	// remove(self, key) -> success
	ENTRY_POINT_DECL(l_remove);

	// get_names(self) -> {key1, ...}
	ENTRY_POINT_DECL(l_get_names);

	// write(self) -> success
	ENTRY_POINT_DECL(l_write);

	// to_table(self) -> {[key1]=value1,...}
	ENTRY_POINT_DECL(l_to_table);

	Settings *m_settings = nullptr;
	std::string m_filename;
	bool m_is_own_settings = false;
	bool m_write_allowed = true;

public:
	LuaSettings(Settings *settings, const std::string &filename);
	LuaSettings(const std::string &filename, bool write_allowed);
	~LuaSettings();

	static void create(lua_State *L, Settings *settings, const std::string &filename);

	// LuaSettings(filename)
	// Creates a LuaSettings and leaves it on top of the stack
	ENTRY_POINT_DECL(create_object);

	static LuaSettings *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};
