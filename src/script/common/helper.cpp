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
#include "irrlichttypes_extrabloated.h"
#include "c_types.h"
#include "c_internal.h"
#include "particleoverlay.h"

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
	if (lua_isnone(L, index) || lua_isnil(L, index))
		return default_value;

	return lua_toboolean(L, index) != 0;
}

template <> s16 LuaHelper::readParam(lua_State *L, int index)
{
	return lua_tonumber(L, index);
}

template <> u16 LuaHelper::readParam(lua_State *L, int index)
{
	return (u16)luaL_checknumber(L, index);
}

template <> u16 LuaHelper::readParam(lua_State *L, int index, const u16 &default_)
{
	if (lua_isnone(L, index) || lua_isnil(L, index))
		return default_;

	return (u16)lua_tonumber(L, index);
}

template <> u32 LuaHelper::readParam(lua_State *L, int index)
{
	return (u32)luaL_checknumber(L, index);
}

template <> u32 LuaHelper::readParam(lua_State *L, int index, const u32 &default_)
{
	if (lua_isnone(L, index) || lua_isnil(L, index))
		return default_;

	return (u32)lua_tonumber(L, index);
}

template <> float LuaHelper::readParam(lua_State *L, int index)
{
	if (isNaN(L, index))
		throw LuaError("NaN value is not allowed.");

	return (float)luaL_checknumber(L, index);
}

template <> float LuaHelper::readParam(lua_State *L, int index, const float &default_)
{
	if (lua_isnone(L, index) || lua_isnil(L, index))
		return default_;

	return readParam<float>(L, index);
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
	std::string result;
	const char *str = luaL_checkstring(L, index);
	result.append(str);
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

template <>
ParticleOverlaySpec LuaHelper::readParam(lua_State *L, int index)
{
	if (!lua_istable(L, index)) {
		throw LuaError("ParticleOverlay definition must be a table. Found: " +
			std::string(lua_typename(L, lua_type(L, index))));
	}

	ParticleOverlaySpec poSpec;
	lua_getfield(L, index, "name");
	poSpec.name = readParam<std::string>(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "texture");
	poSpec.texture_name = readParam<std::string>(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "enabled");
	poSpec.enabled = readParam<bool>(L, -1, true);
	lua_pop(L, 1);

	lua_getfield(L, index, "minpps");
	poSpec.minpps = readParam<u32>(L, -1, 700);
	lua_pop(L, 1);
	if (poSpec.minpps < 1 || poSpec.minpps > 20000)
		throw LuaError("ParticleOverlay minpps must be between 1 and 20000. Found: " +
			std::to_string(poSpec.minpps));

	lua_getfield(L, index, "maxpps");
	poSpec.maxpps = readParam<u32>(L, -1, 1000);
	lua_pop(L, 1);
	if (poSpec.maxpps < 1 || poSpec.maxpps > 20000)
		throw LuaError("ParticleOverlay maxpps must be between 1 and 20000. Found: " +
			std::to_string(poSpec.maxpps));

	lua_getfield(L, index, "direction");
	poSpec.direction = readParam<u16>(L, -1, 0);
	lua_pop(L, 1);
	if (poSpec.direction > 359)
		throw LuaError("ParticleOverlay direction must be between 0 and 359");

	lua_getfield(L, index, "velocity");
	poSpec.velocity = readParam<float>(L, -1, 0.0f);
	lua_pop(L, 1);
	if (poSpec.velocity < 0.0f || poSpec.velocity > 500.0f)
		throw LuaError("ParticleOverlay directional speed must be between 0.0 and 500.0");

	lua_getfield(L, index, "gravity_factor");
	poSpec.gravity_factor = readParam<float>(L, -1, 1.0f);
	lua_pop(L, 1);
	if (poSpec.gravity_factor < 0.0f || poSpec.gravity_factor > 100.0f)
		throw LuaError("ParticleOverlay gravity factor must be between 0.0 and 100.0");

	lua_getfield(L, index, "texture_scale_factor_x");
	poSpec.texture_scale_factor.Width = readParam<float>(L, -1, 1.0f);
	lua_pop(L, 1);

	lua_getfield(L, index, "texture_scale_factor_y");
	poSpec.texture_scale_factor.Height = readParam<float>(L, -1, 1.0f);
	lua_pop(L, 1);

	return poSpec;
}
