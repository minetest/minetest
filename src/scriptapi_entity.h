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

#ifndef LUA_ENTITY_H_
#define LUA_ENTITY_H_

extern "C" {
#include <lua.h>
}

#include "object_properties.h"
#include "content_sao.h"
#include "tool.h"

/*****************************************************************************/
/* scriptapi internal                                                        */
/*****************************************************************************/
void luaentity_get(lua_State *L, u16 id);

/*****************************************************************************/
/* Minetest interface                                                        */
/*****************************************************************************/
// Returns true if succesfully added into Lua; false otherwise.
bool scriptapi_luaentity_add(lua_State *L, u16 id, const char *name);
void scriptapi_luaentity_activate(lua_State *L, u16 id,
		const std::string &staticdata, u32 dtime_s);
void scriptapi_luaentity_rm(lua_State *L, u16 id);
std::string scriptapi_luaentity_get_staticdata(lua_State *L, u16 id);
void scriptapi_luaentity_get_properties(lua_State *L, u16 id,
		ObjectProperties *prop);
void scriptapi_luaentity_step(lua_State *L, u16 id, float dtime);
void scriptapi_luaentity_punch(lua_State *L, u16 id,
		ServerActiveObject *puncher, float time_from_last_punch,
		const ToolCapabilities *toolcap, v3f dir);
void scriptapi_luaentity_rightclick(lua_State *L, u16 id,
		ServerActiveObject *clicker);

#endif /* LUA_ENTITY_H_ */
