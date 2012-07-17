/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SCRIPTAPI_HEADER
#define SCRIPTAPI_HEADER

#include "irrlichttypes_bloated.h"
#include <string>
#include "mapnode.h"
#include <set>
#include <map>

class Server;
class ServerEnvironment;
class ServerActiveObject;
typedef struct lua_State lua_State;
struct ObjectProperties;
struct ItemStack;
struct PointedThing;
//class IGameDef;
struct ToolCapabilities;

void scriptapi_export(lua_State *L, Server *server);
bool scriptapi_loadmod(lua_State *L, const std::string &scriptpath,
		const std::string &modname);
void scriptapi_add_environment(lua_State *L, ServerEnvironment *env);

void scriptapi_add_object_reference(lua_State *L, ServerActiveObject *cobj);
void scriptapi_rm_object_reference(lua_State *L, ServerActiveObject *cobj);

// Returns true if script handled message
bool scriptapi_on_chat_message(lua_State *L, const std::string &name,
		const std::string &message);

/* environment */
// On environment step
void scriptapi_environment_step(lua_State *L, float dtime);
// After generating a piece of map
void scriptapi_environment_on_generated(lua_State *L, v3s16 minp, v3s16 maxp,
		u32 blockseed);

/* misc */
void scriptapi_on_newplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_on_dieplayer(lua_State *L, ServerActiveObject *player);
bool scriptapi_on_respawnplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_on_joinplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_on_leaveplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_get_creative_inventory(lua_State *L, ServerActiveObject *player);
bool scriptapi_get_auth(lua_State *L, const std::string &playername,
		std::string *dst_password, std::set<std::string> *dst_privs);
void scriptapi_create_auth(lua_State *L, const std::string &playername,
		const std::string &password);
bool scriptapi_set_password(lua_State *L, const std::string &playername,
		const std::string &password);

/* item callbacks */
bool scriptapi_item_on_drop(lua_State *L, ItemStack &item,
		ServerActiveObject *dropper, v3f pos);
bool scriptapi_item_on_place(lua_State *L, ItemStack &item,
		ServerActiveObject *placer, const PointedThing &pointed);
bool scriptapi_item_on_use(lua_State *L, ItemStack &item,
		ServerActiveObject *user, const PointedThing &pointed);

/* node callbacks */
bool scriptapi_node_on_punch(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *puncher);
bool scriptapi_node_on_dig(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *digger);
// Node constructor
void scriptapi_node_on_construct(lua_State *L, v3s16 p, MapNode node);
// Node destructor
void scriptapi_node_on_destruct(lua_State *L, v3s16 p, MapNode node);
// Node post-destructor
void scriptapi_node_after_destruct(lua_State *L, v3s16 p, MapNode node);
// Node Timer event
bool scriptapi_node_on_timer(lua_State *L, v3s16 p, MapNode node, f32 dtime);
// Called when a metadata form returns values
void scriptapi_node_on_receive_fields(lua_State *L, v3s16 p,
		const std::string &formname,
		const std::map<std::string, std::string> &fields,
		ServerActiveObject *sender);
// Moves items
void scriptapi_node_on_metadata_inventory_move(lua_State *L, v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player);
// Inserts items, returns rejected items
ItemStack scriptapi_node_on_metadata_inventory_offer(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player);
// Takes items, returns taken items
ItemStack scriptapi_node_on_metadata_inventory_take(lua_State *L, v3s16 p,
		const std::string &listname, int index, int count,
		ServerActiveObject *player);

/* luaentity */
// Returns true if succesfully added into Lua; false otherwise.
bool scriptapi_luaentity_add(lua_State *L, u16 id, const char *name);
void scriptapi_luaentity_activate(lua_State *L, u16 id,
		const std::string &staticdata);
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

#endif

