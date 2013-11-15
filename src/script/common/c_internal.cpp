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
#include "debug.h"

std::string script_get_backtrace(lua_State *L)
{
	std::string s;
	lua_getglobal(L, "debug");
	if(lua_istable(L, -1)){
		lua_getfield(L, -1, "traceback");
		if(lua_isfunction(L, -1)) {
			lua_call(L, 0, 1);
			if(lua_isstring(L, -1)){
				s = lua_tostring(L, -1);
			}
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return s;
}

int script_error_handler(lua_State *L) {
	lua_getglobal(L, "debug");
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

void script_error(lua_State *L)
{
	throw LuaError(NULL, lua_tostring(L, -1));
}

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - removes the table and arguments from the lua stack
// - pushes the return value, computed depending on mode
void script_run_callbacks(lua_State *L, int nargs, RunCallbacksMode mode)
{
	// Insert the return value into the lua stack, below the table
	assert(lua_gettop(L) >= nargs + 1);

	lua_pushnil(L);
	int rv = lua_gettop(L) - nargs - 1;
	lua_insert(L, rv);

	// Insert error handler after return value
	lua_pushcfunction(L, script_error_handler);
	int errorhandler = rv + 1;
	lua_insert(L, errorhandler);

	// Stack now looks like this:
	// ... <return value = nil> <error handler> <table> <arg#1> <arg#2> ... <arg#n>

	int table = errorhandler + 1;
	int arg = table + 1;

	luaL_checktype(L, table, LUA_TTABLE);

	// Foreach
	lua_pushnil(L);
	bool first_loop = true;
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		for(int i = 0; i < nargs; i++)
			lua_pushvalue(L, arg+i);
		if(lua_pcall(L, nargs, 1, errorhandler))
			script_error(L);

		// Move return value to designated space in stack
		// Or pop it
		if(first_loop){
			// Result of first callback is always moved
			lua_replace(L, rv);
			first_loop = false;
		} else {
			// Otherwise, what happens depends on the mode
			if(mode == RUN_CALLBACKS_MODE_FIRST)
				lua_pop(L, 1);
			else if(mode == RUN_CALLBACKS_MODE_LAST)
				lua_replace(L, rv);
			else if(mode == RUN_CALLBACKS_MODE_AND ||
					mode == RUN_CALLBACKS_MODE_AND_SC){
				if((bool)lua_toboolean(L, rv) == true &&
						(bool)lua_toboolean(L, -1) == false)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else if(mode == RUN_CALLBACKS_MODE_OR ||
					mode == RUN_CALLBACKS_MODE_OR_SC){
				if((bool)lua_toboolean(L, rv) == false &&
						(bool)lua_toboolean(L, -1) == true)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else
				assert(0);
		}

		// Handle short circuit modes
		if(mode == RUN_CALLBACKS_MODE_AND_SC &&
				(bool)lua_toboolean(L, rv) == false)
			break;
		else if(mode == RUN_CALLBACKS_MODE_OR_SC &&
				(bool)lua_toboolean(L, rv) == true)
			break;

		// value removed, keep key for next iteration
	}

	// Remove stuff from stack, leaving only the return value
	lua_settop(L, rv);

	// Fix return value in case no callbacks were called
	if(first_loop){
		if(mode == RUN_CALLBACKS_MODE_AND ||
				mode == RUN_CALLBACKS_MODE_AND_SC){
			lua_pop(L, 1);
			lua_pushboolean(L, true);
		}
		else if(mode == RUN_CALLBACKS_MODE_OR ||
				mode == RUN_CALLBACKS_MODE_OR_SC){
			lua_pop(L, 1);
			lua_pushboolean(L, false);
		}
	}
}


