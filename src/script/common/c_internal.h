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

/******************************************************************************/
/******************************************************************************/
/* WARNING!!!! do NOT add this header in any include file or any code file    */
/*             not being a modapi file!!!!!!!!                                */
/******************************************************************************/
/******************************************************************************/

#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "config.h"
#include "common/c_types.h"


/*
	Define our custom indices into the Lua registry table.

	Lua 5.2 and above define the LUA_RIDX_LAST macro. Only numbers above that
	may be used for custom indices, anything else is reserved.

	Lua 5.1 / LuaJIT do not use any numeric indices (only string indices),
	so we can use numeric indices freely.
*/
#ifdef LUA_RIDX_LAST
#define CUSTOM_RIDX_BASE ((LUA_RIDX_LAST)+1)
#else
#define CUSTOM_RIDX_BASE 1
#endif

#define CUSTOM_RIDX_SCRIPTAPI           (CUSTOM_RIDX_BASE)
#define CUSTOM_RIDX_GLOBALS_BACKUP      (CUSTOM_RIDX_BASE + 1)
#define CUSTOM_RIDX_CURRENT_MOD_NAME    (CUSTOM_RIDX_BASE + 2)
#define CUSTOM_RIDX_BACKTRACE           (CUSTOM_RIDX_BASE + 3)
#define CUSTOM_RIDX_HTTP_API_LUA        (CUSTOM_RIDX_BASE + 4)


// Determine if CUSTOM_RIDX_SCRIPTAPI will hold a light or full userdata
#if defined(__aarch64__) && USE_LUAJIT
/* LuaJIT has a 47-bit limit for lightuserdata on this platform and we cannot
 * assume that the ScriptApi class was allocated at a fitting address. */
#define INDIRECT_SCRIPTAPI_RIDX 1
#else
#define INDIRECT_SCRIPTAPI_RIDX 0
#endif

// Pushes the error handler onto the stack and returns its index
#define PUSH_ERROR_HANDLER(L) \
	(lua_rawgeti((L), LUA_REGISTRYINDEX, CUSTOM_RIDX_BACKTRACE), lua_gettop((L)))

#define PCALL_RESL(L, RES) {                            \
	int result_ = (RES);                                \
	if (result_ != 0) {                                 \
		script_error((L), result_, NULL, __FUNCTION__); \
	}                                                   \
}

// What script_run_callbacks does with the return values of callbacks.
// Regardless of the mode, if only one callback is defined,
// its return value is the total return value.
// Modes only affect the case where 0 or >= 2 callbacks are defined.
enum RunCallbacksMode
{
	// Returns the return value of the first callback
	// Returns nil if list of callbacks is empty
	RUN_CALLBACKS_MODE_FIRST,
	// Returns the return value of the last callback
	// Returns nil if list of callbacks is empty
	RUN_CALLBACKS_MODE_LAST,
	// If any callback returns a false value, the first such is returned
	// Otherwise, the first callback's return value (trueish) is returned
	// Returns true if list of callbacks is empty
	RUN_CALLBACKS_MODE_AND,
	// Like above, but stops calling callbacks (short circuit)
	// after seeing the first false value
	RUN_CALLBACKS_MODE_AND_SC,
	// If any callback returns a true value, the first such is returned
	// Otherwise, the first callback's return value (falseish) is returned
	// Returns false if list of callbacks is empty
	RUN_CALLBACKS_MODE_OR,
	// Like above, but stops calling callbacks (short circuit)
	// after seeing the first true value
	RUN_CALLBACKS_MODE_OR_SC,
	// Note: "a true value" and "a false value" refer to values that
	// are converted by readParam<bool> to true or false, respectively.
};

// Gets a backtrace of the current execution point
std::string script_get_backtrace(lua_State *L);
// Wrapper for CFunction calls that converts C++ exceptions to Lua errors
int script_exception_wrapper(lua_State *L, lua_CFunction f);
// Takes an error from lua_pcall and throws it as a LuaError
void script_error(lua_State *L, int pcall_result, const char *mod, const char *fxn);

bool script_log_unique(lua_State *L, std::string message, std::ostream &log_to,
	int stack_depth = 1);

enum DeprecatedHandlingMode {
	Ignore,
	Log,
	Error
};

/**
 * Reads `deprecated_lua_api_handling` in settings, returns cached value.
 *
 * @return DeprecatedHandlingMode
 */
DeprecatedHandlingMode get_deprecated_handling_mode();

/**
 * Handles a deprecation warning based on user settings
 *
 * @param L Lua State
 * @param message The deprecation method
 * @param stack_depth How far on the stack to the first user function (ie: not builtin or core)
 */
void log_deprecated(lua_State *L, std::string message, int stack_depth = 1);
