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
#include "c_types.h"

bool LuaHelper::isNaN(lua_State *L, int idx)
{
	return lua_type(L, idx) == LUA_TNUMBER && std::isnan(lua_tonumber(L, idx));
}

static inline std::string luahelper_type_error(int index, const std::string &type)
{
	std::ostringstream oss;
	oss << "Argument " << index << " is not a " << type;
	return oss.str();
}

/*
 * Read template functions
 */
template <> bool LuaHelper::readParam(lua_State *L, int index)
{
	if (!lua_isboolean(L, index))
		throw LuaError(luahelper_type_error(index, "bool"));

	return lua_toboolean(L, index) != 0;
}

template <> float LuaHelper::readParam(lua_State *L, int index)
{
	if (isNaN(L, index))
		throw LuaError("NaN value is not allowed.");

	return (float)luaL_checknumber(L, index);
}

template <> std::string LuaHelper::readParam(lua_State *L, int index)
{
	if (lua_isnil(L, index))
		return "";

	if (!lua_isstring(L, index))
		throw LuaError(luahelper_type_error(index, "string"));

	std::string result;
	const char *str = lua_tostring(L, index);
	if (str) {
		result.append(str);
	}

	return str;
}
