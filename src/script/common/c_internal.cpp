/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "common/c_internal.h"
#include "server.h"
#include "cpp_api/scriptapi.h"

ScriptApi* get_scriptapi(lua_State *L)
{
	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "scriptapi");
	ScriptApi* sapi_ptr = (ScriptApi*) lua_touserdata(L, -1);
	lua_pop(L, 1);
	return sapi_ptr;
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

void script_error(lua_State* L,const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char buf[10000];
	vsnprintf(buf, 10000, fmt, argp);
	va_end(argp);
	//errorstream<<"SCRIPT ERROR: "<<buf;
	throw LuaError(L, buf);
}


