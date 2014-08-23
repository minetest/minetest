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
#include "log.h"
#include "main.h"
#include "settings.h"

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

int script_exception_wrapper(lua_State *L, lua_CFunction f)
{
	try {
		return f(L);  // Call wrapped function and return result.
	} catch (const char *s) {  // Catch and convert exceptions.
		lua_pushstring(L, s);
	} catch (std::exception& e) {
		lua_pushstring(L, e.what());
	} catch (...) {
		lua_pushliteral(L, "caught (...)");
	}
	return lua_error(L);  // Rethrow as a Lua error.
}

void script_error(lua_State *L)
{
	const char *s = lua_tostring(L, -1);
	std::string str(s ? s : "");
	throw LuaError(str);
}

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - replaces the table and arguments with the return value,
//     computed depending on mode
void script_run_callbacks(lua_State *L, int nargs, RunCallbacksMode mode)
{
	assert(lua_gettop(L) >= nargs + 1);

	// Insert error handler
	lua_pushcfunction(L, script_error_handler);
	int errorhandler = lua_gettop(L) - nargs - 1;
	lua_insert(L, errorhandler);

	// Insert run_callbacks between error handler and table
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "run_callbacks");
	lua_remove(L, -2);
	lua_insert(L, errorhandler + 1);

	// Insert mode after table
	lua_pushnumber(L, (int) mode);
	lua_insert(L, errorhandler + 3);

	// Stack now looks like this:
	// ... <error handler> <run_callbacks> <table> <mode> <arg#1> <arg#2> ... <arg#n>

	if (lua_pcall(L, nargs + 2, 1, errorhandler)) {
		script_error(L);
	}

	lua_remove(L, -2); // Remove error handler
}

void log_deprecated(lua_State *L, std::string message)
{
	static bool configured = false;
	static bool dolog      = false;
	static bool doerror    = false;

	// performance optimization to not have to read and compare setting for every logline
	if (!configured) {
		std::string value = g_settings->get("deprecated_lua_api_handling");
		if (value == "log") {
			dolog = true;
		}
		if (value == "error") {
			dolog = true;
			doerror = true;
		}
	}

	if (doerror) {
		if (L != NULL) {
			script_error(L);
		} else {
			/* As of april 2014 assert is not optimized to nop in release builds
			 * therefore this is correct. */
			assert("Can't do a scripterror for this deprecated message, so exit completely!");
		}
	}

	if (dolog) {
		/* abusing actionstream because of lack of file-only-logged loglevel */
		actionstream << message << std::endl;
		if (L != NULL) {
			actionstream << script_get_backtrace(L) << std::endl;
		}
	}
}

