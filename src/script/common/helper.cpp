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

extern "C" {
#include <lauxlib.h>
}

#include "helper.h"
#include <cmath>
#include <irr_v2d.h>
#include <irr_v3d.h>
#include "c_converter.h"
#include "c_types.h"

/*
 * Read template functions
 */
template <>
bool LuaHelper::readParam(lua_State *L, int index)
{
	return lua_toboolean(L, index) != 0;
}

template <>
s16 LuaHelper::readParam(lua_State *L, int index)
{
	return luaL_checkinteger(L, index);
}

template <>
int LuaHelper::readParam(lua_State *L, int index)
{
	return luaL_checkinteger(L, index);
}

template <>
float LuaHelper::readParam(lua_State *L, int index)
{
	lua_Number v = luaL_checknumber(L, index);
	if (std::isnan(v) && std::isinf(v))
		throw LuaError("Invalid float value (NaN or infinity)");

	return static_cast<float>(v);
}

template <>
v2s16 LuaHelper::readParam(lua_State *L, int index)
{
	return read_v2s16(L, index);
}

template <>
v2f LuaHelper::readParam(lua_State *L, int index)
{
	return check_v2f(L, index);
}

template <>
v3f LuaHelper::readParam(lua_State *L, int index)
{
	return check_v3f(L, index);
}

template <>
std::string LuaHelper::readParam(lua_State *L, int index)
{
	size_t length;
	std::string result;
	const char *str = luaL_checklstring(L, index, &length);
	result.assign(str, length);
	return result;
}
