/*
Minetest
Copyright (C) 2022 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include <memory>

#include "lua_api/l_base.h"
#include "util/unistr.h"

class LuaUniLocale : public ModApiBase
{
private:
	static const char class_name[];
	static const luaL_Reg methods[];

	uni::Locale locale;

public:
	static LuaUniLocale *read_object(lua_State *L, int index);
	static uni::Locale read_locale(lua_State *L, int index);

	static void push_object(lua_State *L, const uni::Locale &locale);

	const uni::Locale &get() const
	{
		return locale;
	}

	static void Register(lua_State *L);

private:
	LuaUniLocale(const uni::Locale &locale) :
		locale(locale)
	{}

	static int gc_object(lua_State *L);

	static int create(lua_State *L);

	static int l_equals(lua_State *L);
	static int l_to_string(lua_State *L);

	static int l_get_language(lua_State *L);
	static int l_get_script(lua_State *L);
	static int l_get_country(lua_State *L);
};

class LuaUniStrIt : public ModApiBase
{
private:
	static const char class_name[];
	static const luaL_Reg methods[];

	uni::StrIt it;

public:
	static LuaUniStrIt *read_object(lua_State *L, int index);
	static const uni::StrIt &read_it(lua_State *L, int index);

	static void push_object(lua_State *L, const uni::StrIt &it);

	const uni::StrIt &get() const
	{
		return it;
	}

	static void Register(lua_State *L);

private:
	LuaUniStrIt(const uni::StrIt &it) :
		it(it)
	{}

	static int gc_object(lua_State *L);

	static int eq(lua_State *L);
	static int lt(lua_State *L);
	static int le(lua_State *L);

	static int l_get_code(lua_State *L);
	static int l_next_code(lua_State *L);
	static int l_prev_code(lua_State *L);
	static int l_next_char(lua_State *L);
	static int l_prev_char(lua_State *L);
	static int l_next_word(lua_State *L);
	static int l_prev_word(lua_State *L);
	static int l_next_break(lua_State *L);
	static int l_prev_break(lua_State *L);
	static int l_next_line(lua_State *L);
	static int l_prev_line(lua_State *L);

	static int l_is_at_start(lua_State *L);
	static int l_is_at_end(lua_State *L);
	static int l_to_start(lua_State *L);
	static int l_to_end(lua_State *L);
};

class LuaUniStr : public ModApiBase
{
private:
	static const char class_name[];
	static const luaL_Reg methods[];

	std::shared_ptr<uni::Str> str;

public:
	static LuaUniStr *read_object(lua_State *L, int index);
	static std::shared_ptr<uni::Str> read_str(lua_State *L, int index);

	static void push_object(lua_State *L, std::shared_ptr<uni::Str> str);

	std::shared_ptr<uni::Str> get() const
	{
		return str;
	}

	static void Register(lua_State *L);

private:
	LuaUniStr(std::shared_ptr<uni::Str> str) :
		str(str)
	{}

	static int gc_object(lua_State *L);

	static int create(lua_State *L);

	static int l_equals(lua_State *L);
	static int l_to_string(lua_State *L);
	static int l_is_empty(lua_State *L);

	static int l_it_start(lua_State *L);
	static int l_it_end(lua_State *L);

	static int l_sub(lua_State *L);
	static int l_append(lua_State *L);
	static int l_insert(lua_State *L);
	static int l_replace(lua_State *L);
	static int l_remove(lua_State *L);
	static int l_clear(lua_State *L);

	static int l_trim(lua_State *L);
	static int l_to_lower(lua_State *L);
	static int l_to_upper(lua_State *L);
	static int l_to_title(lua_State *L);

	static int l_normalize(lua_State *L);

	static int l_get_locale(lua_State *L);
	static int l_set_locale(lua_State *L);
};
