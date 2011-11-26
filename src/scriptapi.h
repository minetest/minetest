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

#ifndef SCRIPTAPI_HEADER
#define SCRIPTAPI_HEADER

#include "irrlichttypes.h"
#include <string>
#include "mapnode.h"

class Server;
class ServerEnvironment;
class ServerActiveObject;
typedef struct lua_State lua_State;
struct LuaEntityProperties;
//class IGameDef;

void scriptapi_export(lua_State *L, Server *server);
void scriptapi_add_environment(lua_State *L, ServerEnvironment *env);

void scriptapi_add_object_reference(lua_State *L, ServerActiveObject *cobj);
void scriptapi_rm_object_reference(lua_State *L, ServerActiveObject *cobj);

/* environment */
// On environment step
void scriptapi_environment_step(lua_State *L, float dtime);
// After adding node
void scriptapi_environment_on_placenode(lua_State *L, v3s16 p, MapNode newnode,
		ServerActiveObject *placer);
// After removing node
void scriptapi_environment_on_dignode(lua_State *L, v3s16 p, MapNode oldnode,
		ServerActiveObject *digger);
// When punching node
void scriptapi_environment_on_punchnode(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *puncher);

/* misc */
void scriptapi_on_newplayer(lua_State *L, ServerActiveObject *player);
bool scriptapi_on_respawnplayer(lua_State *L, ServerActiveObject *player);

/* luaentity */
// Returns true if succesfully added into Lua; false otherwise.
bool scriptapi_luaentity_add(lua_State *L, u16 id, const char *name,
		const std::string &staticdata);
void scriptapi_luaentity_rm(lua_State *L, u16 id);
std::string scriptapi_luaentity_get_staticdata(lua_State *L, u16 id);
void scriptapi_luaentity_get_properties(lua_State *L, u16 id,
		LuaEntityProperties *prop);
void scriptapi_luaentity_step(lua_State *L, u16 id, float dtime);
void scriptapi_luaentity_punch(lua_State *L, u16 id,
		ServerActiveObject *puncher);
void scriptapi_luaentity_rightclick(lua_State *L, u16 id,
		ServerActiveObject *clicker);

#endif

