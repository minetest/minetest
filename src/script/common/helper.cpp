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

#include "helper.h"
#include <cmath>
#include <sstream>
#include <irr_v2d.h>
#include "c_types.h"
#include "c_internal.h"

// imported from c_converter.cpp with pure C++ style
static inline void check_lua_type(lua_State *L, int index, const char *name, int type)
{
	int t = lua_type(L, index);
	if (t != type) {
		std::string traceback = script_get_backtrace(L);
		throw LuaError(std::string("Invalid ") + (name) + " (expected " +
				lua_typename(L, (type)) + " got " + lua_typename(L, t) +
				").\n" + traceback);
	}
}

// imported from c_converter.cpp
#define CHECK_POS_COORD(name)                                                            \
	check_lua_type(L, -1, "position coordinate '" name "'", LUA_TNUMBER)
#define CHECK_POS_TAB(index) check_lua_type(L, index, "position", LUA_TTABLE)

bool LuaHelper::isNaN(lua_State *L, int idx)
{
	return lua_type(L, idx) == LUA_TNUMBER && std::isnan(lua_tonumber(L, idx));
}

/*
 * Read template functions
 */
template <> bool LuaHelper::readParam(lua_State *L, int index)
{
	return lua_toboolean(L, index) != 0;
}

template <> bool LuaHelper::readParam(lua_State *L, int index, const bool &default_value)
{
	if (lua_isnil(L, index))
		return default_value;

	return lua_toboolean(L, index) != 0;
}

template <> s16 LuaHelper::readParam(lua_State *L, int index)
{
	return lua_tonumber(L, index);
}

template <> float LuaHelper::readParam(lua_State *L, int index)
{
	if (isNaN(L, index))
		throw LuaError("NaN value is not allowed.");

	return (float)luaL_checknumber(L, index);
}

template <> v2s16 LuaHelper::readParam(lua_State *L, int index)
{
	v2s16 p;
	CHECK_POS_TAB(index);
	lua_getfield(L, index, "x");
	CHECK_POS_COORD("x");
	p.X = readParam<s16>(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	CHECK_POS_COORD("y");
	p.Y = readParam<s16>(L, -1);
	lua_pop(L, 1);
	return p;
}

template <> v2f LuaHelper::readParam(lua_State *L, int index)
{
	v2f p;
	CHECK_POS_TAB(index);
	lua_getfield(L, index, "x");
	CHECK_POS_COORD("x");
	p.X = readParam<float>(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	CHECK_POS_COORD("y");
	p.Y = readParam<float>(L, -1);
	lua_pop(L, 1);
	return p;
}

template <> std::string LuaHelper::readParam(lua_State *L, int index)
{
	size_t length;
	std::string result;
	const char *str = luaL_checklstring(L, index, &length);
	result.assign(str, length);
	return result;
}

template <>
std::string LuaHelper::readParam(
		lua_State *L, int index, const std::string &default_value)
{
	std::string result;
	const char *str = lua_tostring(L, index);
	if (str)
		result.append(str);
	else
		result = default_value;
	return result;
}
