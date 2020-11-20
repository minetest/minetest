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

#include <vector>
#include <string>
#include "irrlichttypes_extrabloated.h"

class LuaHelper
{
private:
#define MAKE_SINGLE_ARG(name, type)									\
	template <typename T> struct name : public std::false_type {	\
		typedef T value_type;										\
	};																\
	template <typename T>											\
	struct name<type<T>> : public std::true_type {					\
		typedef T value_type;										\
	};

#define MAKE_MULTI_ARG(name, type)									\
	template <typename T> struct name : public std::false_type {	\
		typedef T value_type;										\
	};																\
	template <typename T, typename... Args>							\
	struct name<type<T, Args...>> : public std::true_type {			\
		typedef T value_type;										\
	};

	MAKE_SINGLE_ARG(is_vector2d, core::vector2d)
	MAKE_SINGLE_ARG(is_vector3d, core::vector3d)

	MAKE_MULTI_ARG(is_vector, std::vector)

#undef MAKE_SINGLE_ARG
#undef MAKE_MULTI_ARG

	// Specializations are outside of the class
	template <typename T> struct is_integer : public std::false_type {
		typedef T value_type;
	};

#define SPECIAL_TEMPLATE(name) template <typename T, \
	typename std::enable_if<name<T>::value>::type* = nullptr>
#define DISABLE_TEMPLATE template <typename T, typename std::enable_if< \
		!is_integer<T>::value &&	\
		!is_vector<T>::value &&		\
		!is_vector2d<T>::value &&	\
		!is_vector3d<T>::value		\
	>::type* = nullptr>

#define TEMPLATES(function)					\
	DISABLE_TEMPLATE function;				\
	SPECIAL_TEMPLATE(is_integer) function;	\
	SPECIAL_TEMPLATE(is_vector2d) function;	\
	SPECIAL_TEMPLATE(is_vector3d) function;	\
	SPECIAL_TEMPLATE(is_vector) function

protected:
	/**
	 * Displays an error message appropriate for invalid or unexpected types
	 *
	 * @param L Lua state
	 * @param index Index on the Lua stack that was erroneous
	 * @param name Name of the expected type
	 * @param type Expected Lua type
	 */
	static void invalidType(lua_State *L, int index, const std::string &name, int type);

	/**
	 * Returns a human-readable description string of the type T.
	 */
	TEMPLATES(static std::string getTypeName(bool plural = false));

	/**
	 * Returns whether the given value on the Lua stack is of type T.
	 *
	 * @tparam T Type to check
	 * @param L Lua state
	 * @param index Index on the Lua stack to check
	 */
	TEMPLATES(static bool paramIsType(lua_State *L, int index));

	/**
	 * Throws an error if the given value on the Lua stack is not of type T.
	 *
	 * @tparam T Type to check
	 * @param L Lua state
	 * @param index Index on the Lua stack to check
	 */
	TEMPLATES(static void checkParamType(lua_State *L, int index));

	/**
	 * Read a value of type T from the Lua stack.
	 *
	 * @tparam T Type to read from Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack to read from
	 * @return Value read from the stack
	 */
	TEMPLATES(static T readParam(lua_State *L, int index));

	/**
	 * Read a value of type T from the Lua stack, returning a default value if the
	 * index is nil or none.
	 *
	 * @tparam T Type to read from Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack to read from
	 * @param def Default value to return if nil
	 * @return Value read from the stack or `def` if nil
	 */
	template <typename T> static T readParam(
		lua_State *L, int index, const T &def)
	{
		return paramIsType<T>(L, index) ? readParam<T>(L, index) : def;
	}

	/**
	 * Read a value of type T from a table field on the Lua stack.
	 *
	 * @tparam T Type to read from Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack of the table to read from
	 * @param field Name of the field in the table to read from
	 * @return Value read from the table
	 */
	template <typename T> static T readParamField(
		lua_State *L, int index, const char *field);

	/**
	 * Read a value of type T from a table field on the Lua stack, returning a default
	 * value if the index is nil or none or the table field is nonexistent.
	 *
	 * @tparam T Type to read from Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack of the table to read from
	 * @param field Name of the field in the table to read from
	 * @param def Default value to return if the field is nonexistent
	 * @return Value read from the table or `def` if nil or nonexistent
	 */
	template <typename T> static T readParamField(
		lua_State *L, int index, const char *field, const T &def);

	/**
	 * Read a value of type T from a table index on the Lua stack.
	 *
	 * @tparam T Type to read from Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack of the table to read from
	 * @param n Index in the table to read from
	 * @return Value read from the table
	 */
	template <typename T> static T readParamIndex(lua_State *L, int index, int n);

	/**
	 * Read a value of type T from a table index on the Lua stack, returning a default
	 * value if the index is nil or none or the table field is nonexistent.
	 *
	 * @tparam T Type to read from Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack of the table to read from
	 * @param n Index in the table to read from
	 * @param def Default value to return if the field is nonexistent
	 * @return Value read from the table or `def` if nil or nonexistent
	 */
	template <typename T> static T readParamIndex(
		lua_State *L, int index, int n, const T &def);

	/**
	 * Pushes a value of type T to the top of the Lua stack.
	 *
	 * @tparam T Type to push to Lua
	 * @param L Lua state
	 * @param value Value to push to the stack
	 */
	TEMPLATES(static void pushParam(lua_State *L, const T &value));

	/**
	 * Sets a specified field in a Lua table to a value of type T.
	 *
	 * @tparam T Type to push to Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack of the table to write to
	 * @param field Name of the field in the table to write to
	 * @param value Value to push to the stack
	 */
	template <typename T> static void setParamField(
		lua_State *L, int index, const char *field, const T &value)
	{
		pushParam<T>(L, value);
		lua_setfield(L, index, field);
	}

	/**
	 * Sets a specified index in a Lua table to a value of type T.
	 *
	 * @tparam T Type to push to Lua
	 * @param L Lua state
	 * @param index Index on the Lua stack of the table to write to
	 * @param n Index in the table to write to
	 * @param value Value to push to the stack
	 */
	template <typename T> static void setParamIndex(
		lua_State *L, int index, int n, const T &value)
	{
		pushParam<T>(L, value);
		lua_rawseti(L, index, n);
	}
};

// C++11 has a defect where explicit specializations must be outside of the class scope
#define MAKE_INT(type)															\
	template <> struct LuaHelper::is_integer<type> : public std::true_type {	\
		typedef type value_type;												\
	};
MAKE_INT(s8)
MAKE_INT(s16)
MAKE_INT(s32)
MAKE_INT(s64)
MAKE_INT(u8)
MAKE_INT(u16)
MAKE_INT(u32)
MAKE_INT(u64)
#undef MAKE_INT

#ifndef LUA_HELPER_KEEP_TEMPLATE_MACRO
#undef SPECIAL_TEMPLATE
#endif

#undef DISABLE_TEMPLATE
#undef TEMPLATES
