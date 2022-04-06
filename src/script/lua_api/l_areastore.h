/*
Minetest
Copyright (C) 2015 est31 <mtest31@outlook.com>

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

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class AreaStore;

class LuaAreaStore : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	ENTRY_POINT_DECL(gc_object);

	ENTRY_POINT_DECL(l_get_area);

	ENTRY_POINT_DECL(l_get_areas_for_pos);
	ENTRY_POINT_DECL(l_get_areas_in_area);
	ENTRY_POINT_DECL(l_insert_area);
	ENTRY_POINT_DECL(l_reserve);
	ENTRY_POINT_DECL(l_remove_area);

	ENTRY_POINT_DECL(l_set_cache_params);

	ENTRY_POINT_DECL(l_to_string);
	ENTRY_POINT_DECL(l_to_file);

	ENTRY_POINT_DECL(l_from_string);
	ENTRY_POINT_DECL(l_from_file);

public:
	AreaStore *as = nullptr;

	LuaAreaStore();
	LuaAreaStore(const std::string &type);
	~LuaAreaStore();

	// AreaStore()
	// Creates a AreaStore and leaves it on top of stack
	ENTRY_POINT_DECL(create_object);

	static LuaAreaStore *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};
