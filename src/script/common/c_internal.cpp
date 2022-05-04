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
#include "util/numeric.h"
#include "debug.h"
#include "log.h"
#include "porting.h"
#include "settings.h"
#include <algorithm> // std::find

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
 * not execute the error handler, and by the time lua_pcall returns the
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

static void script_log_add_source(lua_State *L, std::string &message, int stack_depth)
{
	lua_Debug ar;

	if (lua_getstack(L, stack_depth, &ar)) {
		FATAL_ERROR_IF(!lua_getinfo(L, "Sl", &ar), "lua_getinfo() failed");
		message.append(" (at " + std::string(ar.short_src) + ":"
			+ std::to_string(ar.currentline) + ")");
	} else {
		message.append(" (at ?:?)");
	}
}

bool script_log_unique(lua_State *L, std::string message, std::ostream &log_to,
	int stack_depth)
{
	thread_local std::vector<u64> logged_messages;

	script_log_add_source(L, message, stack_depth);
	u64 hash = murmur_hash_64_ua(message.data(), message.length(), 0xBADBABE);

	if (std::find(logged_messages.begin(), logged_messages.end(), hash)
			== logged_messages.end()) {

		logged_messages.emplace_back(hash);
		log_to << message << std::endl;
		return true;
	}
	return false;
}

DeprecatedHandlingMode get_deprecated_handling_mode()
{
	static thread_local bool configured = false;
	static thread_local DeprecatedHandlingMode ret = DeprecatedHandlingMode::Ignore;

	// Only read settings on first call
	if (!configured) {
		std::string value = g_settings->get("deprecated_lua_api_handling");
		if (value == "log") {
			ret = DeprecatedHandlingMode::Log;
		} else if (value == "error") {
			ret = DeprecatedHandlingMode::Error;
		}
		configured = true;
	}

	return ret;
}

void log_deprecated(lua_State *L, std::string message, int stack_depth)
{
	DeprecatedHandlingMode mode = get_deprecated_handling_mode();
	if (mode == DeprecatedHandlingMode::Ignore)
		return;

	script_log_add_source(L, message, stack_depth);
	warningstream << message << std::endl;

	if (mode == DeprecatedHandlingMode::Error)
		script_error(L, LUA_ERRRUN, NULL, NULL);
	else
		infostream << script_get_backtrace(L) << std::endl;
}

void call_string_dump(lua_State *L, int idx)
{
	// Retrieve string.dump from insecure env to avoid it being tampered with
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_GLOBALS_BACKUP);
	if (!lua_isnil(L, -1))
		lua_getfield(L, -1, "string");
	else
		lua_getglobal(L, "string");
	lua_getfield(L, -1, "dump");
	lua_remove(L, -2); // remove _G
	lua_remove(L, -2); // remove 'string' table
	lua_pushvalue(L, idx);
	lua_call(L, 1, 1);
}
