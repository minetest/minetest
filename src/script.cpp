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

void script_call_va(lua_State *L, const char *func, const char *sig, ...)
{
	va_list vl;
	int narg, nres; /* number of arguments and results */

	va_start(vl, sig);
	lua_getglobal(L, func); /* push function */

	for (narg = 0; *sig; narg++) {
		/* repeat for each argument */
		/* check stack space */
		luaL_checkstack(L, 1, "too many arguments");
		switch (*sig++) {
		case 'd': /* double argument */
			lua_pushnumber(L, va_arg(vl, double));
			break;
		case 'i': /* int argument */
			lua_pushinteger(L, va_arg(vl, int));
			break;
		case 's': /* string argument */
			lua_pushstring(L, va_arg(vl, char *));
			break;
		case '>': /* end of arguments */
			goto endargs;
		default:
			script_error(L, "invalid option (%c)", *(sig - 1));
		}
	}
endargs:

	nres = strlen(sig); /* number of expected results */

	if (lua_pcall(L, narg, nres, 0) != 0) /* do the call */
		script_error(L, "error calling '%s': %s", func, lua_tostring(L, -1));
	
	nres = -nres; /* stack index of first result */
	while (*sig) { /* repeat for each result */
		switch (*sig++) {
		case 'd': /* double result */
			if (!lua_isnumber(L, nres))
			script_error(L, "wrong result type");
			*va_arg(vl, double *) = lua_tonumber(L, nres);
			break;
		case 'i': /* int result */
			if (!lua_isnumber(L, nres))
			script_error(L, "wrong result type");
			*va_arg(vl, int *) = lua_tointeger(L, nres);
			break;
		case 's': /* string result */
			if (!lua_isstring(L, nres))
			script_error(L, "wrong result type");
			*va_arg(vl, const char **) = lua_tostring(L, nres);
			break;
		default:
			script_error(L, "invalid option (%c)", *(sig - 1));
		}
		nres++;
	}

	va_end(vl);
}

bool script_load(lua_State *L, const char *path)
{
	infostream<<"Loading and running script from "<<path<<std::endl;
	int ret = luaL_loadfile(L, path) || lua_pcall(L, 0, 0, 0);
	if(ret){
		errorstream<<"Failed to load and run script from "<<path<<": "<<lua_tostring(L, -1)<<std::endl;
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


