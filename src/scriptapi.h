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

#ifndef SCRIPTAPI_HEADER
#define SCRIPTAPI_HEADER

#include <string>
#include <set>
#include <map>
#include "irr_v3d.h"
#include "irr_v2d.h"

extern "C" {
#include <lua.h>
}
#include "scriptapi_inventory.h"
#include "scriptapi_nodemeta.h"
#include "scriptapi_entity.h"
#include "scriptapi_object.h"
#include "scriptapi_env.h"
#include "scriptapi_item.h"
#include "scriptapi_node.h"

#define luamethod(class, name) {#name, class::l_##name}

class Server;

void scriptapi_export(lua_State *L, Server *server);
bool scriptapi_loadmod(lua_State *L, const std::string &scriptpath,
		const std::string &modname);

// Returns true if script handled message
bool scriptapi_on_chat_message(lua_State *L, const std::string &name,
		const std::string &message);

/* server */
void scriptapi_on_shutdown(lua_State *L);

/* misc */
void scriptapi_on_newplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_on_dieplayer(lua_State *L, ServerActiveObject *player);
bool scriptapi_on_respawnplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_on_joinplayer(lua_State *L, ServerActiveObject *player);
void scriptapi_on_leaveplayer(lua_State *L, ServerActiveObject *player);
bool scriptapi_get_auth(lua_State *L, const std::string &playername,
		std::string *dst_password, std::set<std::string> *dst_privs);
void scriptapi_create_auth(lua_State *L, const std::string &playername,
		const std::string &password);
bool scriptapi_set_password(lua_State *L, const std::string &playername,
		const std::string &password);

/* player */
void scriptapi_on_player_receive_fields(lua_State *L, 
		ServerActiveObject *player,
		const std::string &formname,
		const std::map<std::string, std::string> &fields);

#endif

