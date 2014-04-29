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

#ifndef C_INTERNAL_H_
#define C_INTERNAL_H_

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "common/c_types.h"

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
	// are converted by lua_toboolean to true or false, respectively.
};

std::string script_get_backtrace(lua_State *L);
int script_error_handler(lua_State *L);
int script_exception_wrapper(lua_State *L, lua_CFunction f);
void script_error(lua_State *L);
void script_run_callbacks(lua_State *L, int nargs, RunCallbacksMode mode);
void log_deprecated(lua_State *L, std::string message);

#endif /* C_INTERNAL_H_ */
