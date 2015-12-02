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

#include <iostream>

#include "common/c_types.h"
#include "common/c_internal.h"
#include "itemdef.h"


struct EnumString es_ItemType[] =
	{
		{ITEM_NONE, "none"},
		{ITEM_NODE, "node"},
		{ITEM_CRAFT, "craft"},
		{ITEM_TOOL, "tool"},
		{0, NULL},
	};

#if LUA_VERSION_NUM >= 502
LUALIB_API int luaL_pushtype (lua_State *L, int narg) {
	if (!luaL_callmeta(L, narg, "__type"))
		lua_pushstring(L, luaL_typename(L, narg));
	return 1;
}

LUALIB_API int luaL_typerror (lua_State *L, int narg, const char *tname) {
	const char *msg;
	luaL_pushtype(L, narg);
	msg = lua_pushfstring(L, "%s expected, got %s",
			tname, lua_tostring(L, -1));
	return luaL_argerror(L, narg, msg);
}
#endif
