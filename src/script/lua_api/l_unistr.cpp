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

#include "l_unistr.h"

#include "l_internal.h"

const char LuaUniLocale::class_name[] = "uni.Locale";
const luaL_Reg LuaUniLocale::methods[] =
{
	luamethod(LuaUniLocale, equals),
	luamethod(LuaUniLocale, to_string),
	luamethod(LuaUniLocale, get_language),
	luamethod(LuaUniLocale, get_script),
	luamethod(LuaUniLocale, get_country),
	{0, 0}
};

LuaUniLocale *LuaUniLocale::read_object(lua_State *L, int index)
{
	LuaUniLocale *l = *(LuaUniLocale **)luaL_checkudata(L, index, class_name);
	return l;
}

uni::Locale LuaUniLocale::read_locale(lua_State *L, int index)
{
	if (lua_isstring(L, index))
		return uni::Locale(readParam<std::string>(L, index).c_str());

	return read_object(L, index)->locale;
}

void LuaUniLocale::push_object(lua_State *L, const uni::Locale &locale)
{
	*(void **)lua_newuserdata(L, sizeof(void *)) = new LuaUniLocale(locale);
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
}

void LuaUniLocale::Register(lua_State *L)
{
	lua_newtable(L);
	int method_table = lua_gettop(L);

	luaL_newmetatable(L, class_name);
	int metatable = lua_gettop(L);

	lua_pushvalue(L, method_table);
	lua_setfield(L, metatable, "__metatable");

	lua_pushvalue(L, method_table);
	lua_setfield(L, metatable, "__index");

	lua_pushcfunction(L, gc_object);
	lua_setfield(L, metatable, "__gc");

	lua_pushcfunction(L, l_equals);
	lua_setfield(L, metatable, "__eq");

	lua_pushcfunction(L, l_to_string);
	lua_setfield(L, metatable, "__tostring");

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_pushcfunction(L, create);
	lua_setfield(L, -1, class_name);
}

int LuaUniLocale::gc_object(lua_State *L)
{
	LuaUniLocale *l = *(LuaUniLocale **)lua_touserdata(L, 1);
	delete l;
	return 0;
}

// uni.Locale([locale]) -> locale
int LuaUniLocale::create(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	push_object(L, read_locale(L, 1));
	return 1;
}

// equals(other) -> bool
int LuaUniLocale::l_equals(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->locale == read_locale(L, 2));
	return 1;
}

// to_string() -> string
int LuaUniLocale::l_to_string(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, read_object(L, 1)->locale.toString());
	return 1;
}

// get_langauge() -> string
int LuaUniLocale::l_get_language(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, read_object(L, 1)->locale.getLanguage());
	return 1;
}

// get_script() -> string
int LuaUniLocale::l_get_script(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, read_object(L, 1)->locale.getScript());
	return 1;
}

// get_country() -> string
int LuaUniLocale::l_get_country(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushstring(L, read_object(L, 1)->locale.getCountry());
	return 1;
}

const char LuaUniStrIt::class_name[] = "uni.StrIt";
const luaL_Reg LuaUniStrIt::methods[] =
{
	luamethod(LuaUniStrIt, get_code),
	luamethod(LuaUniStrIt, next_code),
	luamethod(LuaUniStrIt, prev_code),
	luamethod(LuaUniStrIt, next_char),
	luamethod(LuaUniStrIt, prev_char),
	luamethod(LuaUniStrIt, next_word),
	luamethod(LuaUniStrIt, prev_word),
	luamethod(LuaUniStrIt, next_break),
	luamethod(LuaUniStrIt, prev_break),
	luamethod(LuaUniStrIt, next_line),
	luamethod(LuaUniStrIt, prev_line),
	luamethod(LuaUniStrIt, is_at_start),
	luamethod(LuaUniStrIt, is_at_end),
	luamethod(LuaUniStrIt, to_start),
	luamethod(LuaUniStrIt, to_end),
	{0, 0}
};

LuaUniStrIt *LuaUniStrIt::read_object(lua_State *L, int index)
{
	return *(LuaUniStrIt **)luaL_checkudata(L, index, class_name);
}

const uni::StrIt &LuaUniStrIt::read_it(lua_State *L, int index)
{
	return read_object(L, index)->it;
}

void LuaUniStrIt::push_object(lua_State *L, const uni::StrIt &it)
{
	*(void **)lua_newuserdata(L, sizeof(void *)) = new LuaUniStrIt(it);
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
}

void LuaUniStrIt::Register(lua_State *L)
{
	lua_newtable(L);
	int method_table = lua_gettop(L);

	luaL_newmetatable(L, class_name);
	int metatable = lua_gettop(L);

	lua_pushvalue(L, method_table);
	lua_setfield(L, metatable, "__metatable");

	lua_pushvalue(L, method_table);
	lua_setfield(L, metatable, "__index");

	lua_pushcfunction(L, gc_object);
	lua_setfield(L, metatable, "__gc");

	lua_pushcfunction(L, eq);
	lua_setfield(L, metatable, "__eq");

	lua_pushcfunction(L, lt);
	lua_setfield(L, metatable, "__lt");

	lua_pushcfunction(L, le);
	lua_setfield(L, metatable, "__le");

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);
}

int LuaUniStrIt::gc_object(lua_State *L)
{
	LuaUniStrIt *i = *(LuaUniStrIt **)lua_touserdata(L, 1);
	delete i;
	return 0;
}

int LuaUniStrIt::eq(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it == read_object(L, 2)->it);
	return 1;
}

int LuaUniStrIt::lt(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it < read_object(L, 2)->it);
	return 1;
}

int LuaUniStrIt::le(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it <= read_object(L, 2)->it);
	return 1;
}

// get_code() -> code
int LuaUniStrIt::l_get_code(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushinteger(L, read_object(L, 1)->it.getCode());
	return 1;
}

// next_code() -> code
int LuaUniStrIt::l_next_code(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushinteger(L, read_object(L, 1)->it.nextCode());
	return 1;
}

// prev_code() -> code
int LuaUniStrIt::l_prev_code(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushinteger(L, read_object(L, 1)->it.prevCode());
	return 1;
}

// next_char()
int LuaUniStrIt::l_next_char(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.nextChar();
	return 0;
}

// prev_char()
int LuaUniStrIt::l_prev_char(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.prevChar();
	return 0;
}

// next_word()
int LuaUniStrIt::l_next_word(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.nextWord();
	return 0;
}

// prev_word()
int LuaUniStrIt::l_prev_word(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.prevWord();
	return 0;
}

// next_break() -> is_hard
int LuaUniStrIt::l_next_break(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it.nextBreak());
	return 1;
}

// prev_break() -> is_hard
int LuaUniStrIt::l_prev_break(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it.prevBreak());
	return 1;
}

// next_line()
int LuaUniStrIt::l_next_line(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.nextLine();
	return 0;
}

// prev_line()
int LuaUniStrIt::l_prev_line(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.prevLine();
	return 0;
}

// is_at_start() -> at_start
int LuaUniStrIt::l_is_at_start(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it.isAtStart());
	return 1;
}

// is_at_end() -> at_end
int LuaUniStrIt::l_is_at_end(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->it.isAtEnd());
	return 1;
}

// to_start()
int LuaUniStrIt::l_to_start(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.toStart();
	return 0;
}

// to_end()
int LuaUniStrIt::l_to_end(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->it.toEnd();
	return 0;
}

const char LuaUniStr::class_name[] = "uni.Str";
const luaL_Reg LuaUniStr::methods[] =
{
	luamethod(LuaUniStr, equals),
	luamethod(LuaUniStr, to_string),
	luamethod(LuaUniStr, is_empty),
	luamethod(LuaUniStr, it_start),
	luamethod(LuaUniStr, it_end),
	luamethod(LuaUniStr, sub),
	luamethod(LuaUniStr, append),
	luamethod(LuaUniStr, insert),
	luamethod(LuaUniStr, replace),
	luamethod(LuaUniStr, remove),
	luamethod(LuaUniStr, clear),
	luamethod(LuaUniStr, trim),
	luamethod(LuaUniStr, to_lower),
	luamethod(LuaUniStr, to_upper),
	luamethod(LuaUniStr, to_title),
	luamethod(LuaUniStr, normalize),
	luamethod(LuaUniStr, get_locale),
	luamethod(LuaUniStr, set_locale),
	{0, 0}
};

LuaUniStr *LuaUniStr::read_object(lua_State *L, int index)
{
	LuaUniStr *s = *(LuaUniStr **)luaL_checkudata(L, index, class_name);
	return s;
}

std::shared_ptr<uni::Str> LuaUniStr::read_str(lua_State *L, int index)
{
	if (lua_isstring(L, index))
		return uni::Str::create(readParam<std::string>(L, index));

	return read_object(L, index)->str;
}

void LuaUniStr::push_object(lua_State *L, std::shared_ptr<uni::Str> str)
{
	*(void **)lua_newuserdata(L, sizeof(void *)) = new LuaUniStr(str);
	luaL_getmetatable(L, class_name);
	lua_setmetatable(L, -2);
}

void LuaUniStr::Register(lua_State *L)
{
	lua_newtable(L);
	int method_table = lua_gettop(L);

	luaL_newmetatable(L, class_name);
	int metatable = lua_gettop(L);

	lua_pushvalue(L, method_table);
	lua_setfield(L, metatable, "__metatable");

	lua_pushvalue(L, method_table);
	lua_setfield(L, metatable, "__index");

	lua_pushcfunction(L, gc_object);
	lua_setfield(L, metatable, "__gc");

	lua_pushcfunction(L, l_equals);
	lua_setfield(L, metatable, "__eq");

	lua_pushcfunction(L, l_to_string);
	lua_setfield(L, metatable, "__tostring");

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_pushcfunction(L, create);
	lua_setfield(L, -1, class_name);
}

int LuaUniStr::gc_object(lua_State *L)
{
	LuaUniStr *s = *(LuaUniStr **)lua_touserdata(L, 1);
	delete s;
	return 0;
}

// uni.Str([str, [locale]]) -> str
int LuaUniStr::create(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::shared_ptr<uni::Str> str;

	if (!lua_isnoneornil(L, 1))
		str = read_str(L, 1);
	else
		str = uni::Str::create();

	if (!lua_isnoneornil(L, 2))
		str->setLocale(LuaUniLocale::read_locale(L, 2));

	push_object(L, str);

	return 1;
}

// equals(other) -> bool
int LuaUniStr::l_equals(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, *read_str(L, 1) == *read_str(L, 2));
	return 1;
}

// to_string() -> string
int LuaUniStr::l_to_string(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string str = read_object(L, 1)->str->toString();
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

// is_empty() -> bool
int LuaUniStr::l_is_empty(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	lua_pushboolean(L, read_object(L, 1)->str->isEmpty());
	return 1;
}

// it_start() -> it
int LuaUniStr::l_it_start(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaUniStrIt::push_object(L, read_object(L, 1)->str->itStart());
	return 1;
}

// it_end() -> it
int LuaUniStr::l_it_end(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaUniStrIt::push_object(L, read_object(L, 1)->str->itEnd());
	return 1;
}

// sub(begin, end) -> substr
int LuaUniStr::l_sub(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	push_object(L, read_object(L, 1)->str->sub(
			LuaUniStrIt::read_it(L, 2), LuaUniStrIt::read_it(L, 3)));
	return 1;
}

// append(str)
int LuaUniStr::l_append(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->append(read_str(L, 2));
	return 0;
}

// insert(pos, str)
int LuaUniStr::l_insert(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->insert(LuaUniStrIt::read_it(L, 2), read_str(L, 3));
	return 0;
}

// replace(begin, end, str)
int LuaUniStr::l_replace(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->replace(
			LuaUniStrIt::read_it(L, 2), LuaUniStrIt::read_it(L, 3), read_str(L, 4));
	return 0;
}

// remove(begin, end)
int LuaUniStr::l_remove(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->remove(
			LuaUniStrIt::read_it(L, 2), LuaUniStrIt::read_it(L, 3));
	return 0;
}

// clear()
int LuaUniStr::l_clear(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->clear();
	return 0;
}

// trim()
int LuaUniStr::l_trim(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->trim();
	return 0;
}

// to_lower()
int LuaUniStr::l_to_lower(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->toLower();
	return 0;
}

// to_upper()
int LuaUniStr::l_to_upper(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->toUpper();
	return 0;
}

// to_title([first_only])
int LuaUniStr::l_to_title(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->toTitle(lua_toboolean(L, 2));
	return 0;
}

// normalize([compose])
int LuaUniStr::l_normalize(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->normalize(lua_toboolean(L, 2));
	return 0;
}

// get_locale()
int LuaUniStr::l_get_locale(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaUniLocale::push_object(L, read_object(L, 1)->str->getLocale());
	return 1;
}

// set_locale(locale)
int LuaUniStr::l_set_locale(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	read_object(L, 1)->str->setLocale(LuaUniLocale::read_locale(L, 2));
	return 0;
}
