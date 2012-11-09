/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "script.h"
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "log.h"
#include <iostream>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

LuaError::LuaError(lua_State *L, const std::string &s)
{
	m_s = "LuaError: ";
	m_s += s + "\n";
	m_s += script_get_backtrace(L);
}

std::string script_get_backtrace(lua_State *L)
{
	std::string s;
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if(lua_istable(L, -1)){
		lua_getfield(L, -1, "traceback");
		if(lua_isfunction(L, -1)){
			lua_call(L, 0, 1);
			if(lua_isstring(L, -1)){
				s += lua_tostring(L, -1);
			}
			lua_pop(L, 1);
		}
		else{
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	return s;
}

void script_error(lua_State *L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char buf[10000];
	vsnprintf(buf, 10000, fmt, argp);
	va_end(argp);
	//errorstream<<"SCRIPT ERROR: "<<buf;
	throw LuaError(L, buf);
}

int luaErrorHandler(lua_State *L) {
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

bool script_load(lua_State *L, const char *path)
{
	verbosestream<<"Loading and running script from "<<path<<std::endl;

	lua_pushcfunction(L, luaErrorHandler);
	int errorhandler = lua_gettop(L);

	int ret = luaL_loadfile(L, path) || lua_pcall(L, 0, 0, errorhandler);
	if(ret){
		errorstream<<"========== ERROR FROM LUA ==========="<<std::endl;
		errorstream<<"Failed to load and run script from "<<std::endl;
		errorstream<<path<<":"<<std::endl;
		errorstream<<std::endl;
		errorstream<<lua_tostring(L, -1)<<std::endl;
		errorstream<<std::endl;
		errorstream<<"=======END OF ERROR FROM LUA ========"<<std::endl;
		lua_pop(L, 1); // Pop error message from stack
		lua_pop(L, 1); // Pop the error handler from stack
		return false;
	}
	lua_pop(L, 1); // Pop the error handler from stack
	return true;
}

lua_State* script_init()
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	return L;
}

void script_deinit(lua_State *L)
{
	lua_close(L);
}


