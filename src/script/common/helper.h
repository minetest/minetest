/*
Minetest
Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

class LuaHelper
{
protected:
	static bool isNaN(lua_State *L, int idx);

	/**
	 * Read a value using a template type T from Lua State L and index
	 *
	 * @tparam T type to read from Lua
	 * @param L Lua state
	 * @param index Lua Index to read
	 * @return read value from Lua
	 */
	template <typename T> static T readParam(lua_State *L, int index);
};
