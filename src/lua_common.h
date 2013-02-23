/*
Minetest-c55
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

#ifndef LUA_COMMON_H_
#define LUA_COMMON_H_

extern "C" {
#include <lua.h>
}

#include "server.h"
#include "environment.h"
#include "nodedef.h"
#include "util/pointedthing.h"
#include "tool.h"

Server* get_server(lua_State *L);
ServerEnvironment* get_env(lua_State *L);

void warn_if_field_exists(lua_State *L, int table,
        const char *fieldname, const std::string &message);

ToolCapabilities   read_tool_capabilities        (lua_State *L, int table);
void               push_tool_capabilities        (lua_State *L,
        const ToolCapabilities &prop);
void               set_tool_capabilities         (lua_State *L, int table,
        const ToolCapabilities &toolcap);

void               realitycheck                  (lua_State *L);

void               push_pointed_thing            (lua_State *L,
        const PointedThing& pointed);

void               stackDump                     (lua_State *L, std::ostream &o);

class StackUnroller
{
private:
	lua_State *m_lua;
	int m_original_top;
public:
	StackUnroller(lua_State *L):
		m_lua(L),
		m_original_top(-1)
	{
		m_original_top = lua_gettop(m_lua); // store stack height
	}
	~StackUnroller()
	{
		lua_settop(m_lua, m_original_top); // restore stack height
	}
};

/* definitions */
// What scriptapi_run_callbacks does with the return values of callbacks.
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

struct EnumString
{
	int num;
	const char *str;
};

bool string_to_enum(const EnumString *spec, int &result,
		const std::string &str);

int getenumfield(lua_State *L, int table,
		const char *fieldname, const EnumString *spec, int default_);
#endif /* LUA_COMMON_H_ */
