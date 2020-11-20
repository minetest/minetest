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

#define LUA_HELPER_KEEP_TEMPLATE_MACRO

#include "helper.h"
#include <cmath>
#include "c_types.h"
#include "c_internal.h"

void LuaHelper::invalidType(lua_State *L, int index, const std::string &name, int type)
{
	throw LuaError(std::string("Invalid ") + name + " (expected " +
		lua_typename(L, type) + " got " + lua_typename(L, lua_type(L, index)) +
		").\n" + script_get_backtrace(L));
}

template <typename T> T LuaHelper::readParamField(
	lua_State *L, int index, const char *field)
{
	lua_getfield(L, index, field);
	T value = readParam<T>(L, -1);
	lua_pop(L, 1);
	return value;
}

template <typename T> T LuaHelper::readParamField(
	lua_State *L, int index, const char *field, const T &def)
{
	if (lua_isnoneornil(L, index))
		return def;

	lua_getfield(L, index, field);
	T value = readParam<T>(L, -1, def);
	lua_pop(L, 1);

	return value;
}

template <typename T> T LuaHelper::readParamIndex(lua_State *L, int index, int n)
{
	lua_rawgeti(L, index, n);
	T ret = readParam<T>(L, -1);
	lua_pop(L, 1);
	return ret;
}

template <typename T> T LuaHelper::readParamIndex(
	lua_State *L, int index, int n, const T &def)
{
	if (lua_isnoneornil(L, index))
		return def;

	lua_rawgeti(L, index, n);
	T ret = readParam<T>(L, -1, def);
	lua_pop(L, 1);

	return ret;
}

/* bool */

template <> std::string LuaHelper::getTypeName<bool>(bool plural)
{
	return plural ? "booleans" : "boolean";
}

template <> bool LuaHelper::paramIsType<bool>(lua_State *L, int index)
{
	return lua_isboolean(L, index);
}

template <> void LuaHelper::checkParamType<bool>(lua_State *L, int index)
{
	if (!lua_isboolean(L, index))
		invalidType(L, index, getTypeName<bool>(), LUA_TBOOLEAN);
}

template <> bool LuaHelper::readParam(lua_State *L, int index)
{
	return lua_toboolean(L, index);
}

template <> void LuaHelper::pushParam(lua_State *L, const bool &value)
{
	lua_pushboolean(L, value);
}

/* integer types */

SPECIAL_TEMPLATE(LuaHelper::is_integer) std::string LuaHelper::getTypeName(
	bool plural)
{
	return plural ? "integers" : "integer";
}

SPECIAL_TEMPLATE(LuaHelper::is_integer) bool LuaHelper::paramIsType(
	lua_State *L, int index)
{
	return lua_isnumber(L, index);
}

SPECIAL_TEMPLATE(LuaHelper::is_integer) void LuaHelper::checkParamType(
	lua_State *L, int index)
{
	if (!lua_isnumber(L, index))
		invalidType(L, index, getTypeName<T>(), LUA_TNUMBER);
}

SPECIAL_TEMPLATE(LuaHelper::is_integer) T LuaHelper::readParam(lua_State *L, int index)
{
	return lua_tonumber(L, index);
}

SPECIAL_TEMPLATE(LuaHelper::is_integer) void LuaHelper::pushParam(
	lua_State *L, const T &value)
{
	lua_pushnumber(L, value);
}

/* float */

template <> std::string LuaHelper::getTypeName<float>(bool plural)
{
	return plural ? "numbers" : "number";
}

template <> bool LuaHelper::paramIsType<float>(lua_State *L, int index)
{
	return lua_isnumber(L, index);
}

template <> void LuaHelper::checkParamType<float>(lua_State *L, int index)
{
	if (!lua_isnumber(L, index))
		invalidType(L, index, getTypeName<float>(), LUA_TNUMBER);
}

template <> float LuaHelper::readParam(lua_State *L, int index)
{
	float num = lua_tonumber(L, index);
	if (std::isnan(num))
		throw LuaError("NaN value is not allowed.");
	return num;
}

template <> void LuaHelper::pushParam(lua_State *L, const float &value)
{
	lua_pushnumber(L, value);
}

/* double */

template <> std::string LuaHelper::getTypeName<double>(bool plural)
{
	return plural ? "numbers" : "number";
}

template <> bool LuaHelper::paramIsType<double>(lua_State *L, int index)
{
	return lua_isnumber(L, index);
}

template <> void LuaHelper::checkParamType<double>(lua_State *L, int index)
{
	if (!lua_isnumber(L, index))
		invalidType(L, index, getTypeName<double>(), LUA_TNUMBER);
}

template <> double LuaHelper::readParam(lua_State *L, int index)
{
	return lua_tonumber(L, index);
}

template <> void LuaHelper::pushParam(lua_State *L, const double &value)
{
	lua_pushnumber(L, value);
}

/* std::string */

template <> std::string LuaHelper::getTypeName<std::string>(bool plural)
{
	return plural ? "strings" : "string";
}

template <> bool LuaHelper::paramIsType<std::string>(lua_State *L, int index)
{
	return lua_isstring(L, index);
}

template <> void LuaHelper::checkParamType<std::string>(lua_State *L, int index)
{
	if (!lua_isstring(L, index))
		invalidType(L, index, getTypeName<std::string>(), LUA_TSTRING);
}

template <> std::string LuaHelper::readParam(lua_State *L, int index)
{
	checkParamType<std::string>(L, index);

	size_t length;
	std::string result;

	const char *str = lua_tolstring(L, index, &length);

	result.assign(str, length);
	return result;
}

template <> void LuaHelper::pushParam(lua_State *L, const std::string &value)
{
	lua_pushlstring(L, value.c_str(), value.size());
}

/* core::vector2d */

SPECIAL_TEMPLATE(LuaHelper::is_vector2d) std::string LuaHelper::getTypeName(
	bool plural)
{
	return plural ? "2D positions/vectors" : "2D position/vector";
}

SPECIAL_TEMPLATE(LuaHelper::is_vector2d) bool LuaHelper::paramIsType(
	lua_State *L, int index)
{
	return lua_istable(L, index);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector2d) void LuaHelper::checkParamType(
	lua_State *L, int index)
{
	if (!lua_istable(L, index))
		invalidType(L, index, getTypeName<T>(), LUA_TTABLE);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector2d) T LuaHelper::readParam(lua_State *L, int index)
{
	checkParamType<T>(L, index);
	return {
		readParamField<is_vector2d<T>::value_type>(L, -1, "x"),
		readParamField<is_vector2d<T>::value_type>(L, -1, "y")
	};
}

SPECIAL_TEMPLATE(LuaHelper::is_vector2d) void LuaHelper::pushParam(
	lua_State *L, const T &value)
{
	lua_createtable(L, 0, 2);

	pushParam<is_vector2d<T>::value_type>(L, value.X);
	lua_setfield(L, -2, "x");

	pushParam<is_vector2d<T>::value_type>(L, value.Y);
	lua_setfield(L, -2, "y");
}

/* core::vector3d */

SPECIAL_TEMPLATE(LuaHelper::is_vector3d) std::string LuaHelper::getTypeName(
bool plural)
{
	return plural ? "3D positions/vectors" : "3D position/vector";
}

SPECIAL_TEMPLATE(LuaHelper::is_vector3d) bool LuaHelper::paramIsType(
	lua_State *L, int index)
{
	return lua_istable(L, index);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector3d) void LuaHelper::checkParamType(
	lua_State *L, int index)
{
	if (!lua_istable(L, index))
		invalidType(L, index, getTypeName<T>(), LUA_TTABLE);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector3d) T LuaHelper::readParam(lua_State *L, int index)
{
	checkParamType<T>(L, index);
	return {
		readParamField<is_vector2d<T>::value_type>(L, -1, "x"),
		readParamField<is_vector2d<T>::value_type>(L, -1, "y"),
		readParamField<is_vector2d<T>::value_type>(L, -1, "z")
	};
}

SPECIAL_TEMPLATE(LuaHelper::is_vector3d) void LuaHelper::pushParam(
	lua_State *L, const T &value)
{
	lua_createtable(L, 0, 3);

	pushParam<is_vector2d<T>::value_type>(L, value.X);
	lua_setfield(L, -2, "x");

	pushParam<is_vector2d<T>::value_type>(L, value.Y);
	lua_setfield(L, -2, "y");

	pushParam<is_vector2d<T>::value_type>(L, value.Z);
	lua_setfield(L, -2, "z");
}

/* std::vector */

SPECIAL_TEMPLATE(LuaHelper::is_vector) std::string LuaHelper::getTypeName(
	bool plural)
{
	std::string start;
	if (plural)
		start = "arrays of ";
	else
		start = "array of ";
	return start + getTypeName<typename T::value_type>(true);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector) bool LuaHelper::paramIsType(
	lua_State *L, int index)
{
	return lua_istable(L, index);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector) void LuaHelper::checkParamType(
	lua_State *L, int index)
{
	if (!lua_istable(L, index))
		invalidType(L, index, getTypeName<T>(), LUA_TTABLE);
}

SPECIAL_TEMPLATE(LuaHelper::is_vector) T LuaHelper::readParam(lua_State *L, int index)
{
	checkParamType<T>(L, index, getTypeName<T>(), LUA_TTABLE);

	size_t len = lua_objlen(L, index);
	T vector;
	vector.reserve(len);

	for (size_t i = 1; i <= len; i++) {
		lua_rawgeti(L, index, i);
		vector.push_back(readParam<typename T::value_type>(L, -1));
		lua_pop(L, 1);
	}

	return vector;
}
