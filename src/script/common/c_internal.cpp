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
#include "porting.h"
#include "settings.h"

std::string script_get_backtrace(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_BACKTRACE);
	lua_call(L, 0, 1);
	std::string result = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	return result;
}

int script_exception_wrapper(lua_State *L, lua_CFunction f)
{
	try {
		return f(L);  // Call wrapped function and return result.
	} catch (const char *s) {  // Catch and convert exceptions.
		lua_pushstring(L, s);
	} catch (std::exception &e) {
		lua_pushstring(L, e.what());
	}
	return lua_error(L);  // Rethrow as a Lua error.
}

/*
 * Note that we can't get tracebacks for LUA_ERRMEM or LUA_ERRERR (without
 * hacking Lua internals).  For LUA_ERRMEM, this is because memory errors will
 * not execute the the error handler, and by the time lua_pcall returns the
 * execution stack will have already been unwound.  For LUA_ERRERR, there was
 * another error while trying to generate a backtrace from a LUA_ERRRUN.  It is
 * presumed there is an error with the internal Lua state and thus not possible
 * to gather a coherent backtrace.  Realistically, the best we can do here is
 * print which C function performed the failing pcall.
 */
void script_error(lua_State *L, int pcall_result, const char *mod, const char *fxn)
{
	if (pcall_result == 0)
		return;

	const char *err_type;
	switch (pcall_result) {
	case LUA_ERRRUN:
		err_type = "Runtime";
		break;
	case LUA_ERRMEM:
		err_type = "OOM";
		break;
	case LUA_ERRERR:
		err_type = "Double fault";
		break;
	default:
		err_type = "Unknown";
	}

	if (!mod)
		mod = "??";

	if (!fxn)
		fxn = "??";

	const char *err_descr = lua_tostring(L, -1);
	if (!err_descr)
		err_descr = "<no description>";

	char buf[256];
	porting::mt_snprintf(buf, sizeof(buf), "%s error from mod '%s' in callback %s(): ",
		err_type, mod, fxn);

	std::string err_msg(buf);
	err_msg += err_descr;

	if (pcall_result == LUA_ERRMEM) {
		err_msg += "\nCurrent Lua memory usage: "
			+ itos(lua_gc(L, LUA_GCCOUNT, 0) >> 10) + " MB";
	}

	throw LuaError(err_msg);
}

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - replaces the table and arguments with the return value,
//     computed depending on mode
void script_run_callbacks_f(lua_State *L, int nargs,
	RunCallbacksMode mode, const char *fxn)
{
	FATAL_ERROR_IF(lua_gettop(L) < nargs + 1, "Not enough arguments");

	// Insert error handler
	PUSH_ERROR_HANDLER(L);
	int error_handler = lua_gettop(L) - nargs - 1;
	lua_insert(L, error_handler);

	// Insert run_callbacks between error handler and table
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "run_callbacks");
	lua_remove(L, -2);
	lua_insert(L, error_handler + 1);

	// Insert mode after table
	lua_pushnumber(L, (int) mode);
	lua_insert(L, error_handler + 3);

	// Stack now looks like this:
	// ... <error handler> <run_callbacks> <table> <mode> <arg#1> <arg#2> ... <arg#n>

	int result = lua_pcall(L, nargs + 2, 1, error_handler);
	if (result != 0)
		script_error(L, result, NULL, fxn);

	lua_remove(L, error_handler);
}

void log_deprecated(lua_State *L, const std::string &message)
{
	static bool configured = false;
	static bool do_log     = false;
	static bool do_error   = false;

	// Only read settings on first call
	if (!configured) {
		std::string value = g_settings->get("deprecated_lua_api_handling");
		if (value == "log") {
			do_log = true;
		} else if (value == "error") {
			do_log   = true;
			do_error = true;
		}
	}

	if (do_log) {
		warningstream << message;
		if (L) { // L can be NULL if we get called from scripting_game.cpp
			lua_Debug ar;

			if (!lua_getstack(L, 2, &ar))
				FATAL_ERROR_IF(!lua_getstack(L, 1, &ar), "lua_getstack() failed");
			FATAL_ERROR_IF(!lua_getinfo(L, "Sl", &ar), "lua_getinfo() failed");
			warningstream << " (at " << ar.short_src << ":" << ar.currentline << ")";
		}
		warningstream << std::endl;

		if (L) {
			if (do_error)
				script_error(L, LUA_ERRRUN, NULL, NULL);
			else
				infostream << script_get_backtrace(L) << std::endl;
		}
	}
}

