/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
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

void script_error(lua_State *L, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	lua_close(L);
	exit(EXIT_FAILURE);
}

bool script_load(lua_State *L, const char *path)
{
	infostream<<"Loading and running script from "<<path<<std::endl;
	int ret = luaL_loadfile(L, path) || lua_pcall(L, 0, 0, 0);
	if(ret){
		errorstream<<"Failed to load and run script from "<<path<<":"<<std::endl;
		errorstream<<"[LUA] "<<lua_tostring(L, -1)<<std::endl;
		lua_pop(L, 1); // Pop error message from stack
		return false;
	}
	return true;
}

lua_State* script_init()
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	return L;
}

lua_State* script_deinit(lua_State *L)
{
	lua_close(L);
}


