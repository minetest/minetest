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

#ifndef LUA_NODE_H_
#define LUA_NODE_H_

#include <iostream>
#include <map>

extern "C" {
#include <lua.h>
}

#include "content_sao.h"
#include "map.h"

/*****************************************************************************/
/* Minetest interface                                                        */
/*****************************************************************************/
bool scriptapi_node_on_punch(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *puncher);
bool scriptapi_node_on_dig(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *digger);
void scriptapi_node_on_construct(lua_State *L, v3s16 p, MapNode node);
void scriptapi_node_on_destruct(lua_State *L, v3s16 p, MapNode node);
void scriptapi_node_after_destruct(lua_State *L, v3s16 p, MapNode node);
bool scriptapi_node_on_timer(lua_State *L, v3s16 p, MapNode node, f32 dtime);
void scriptapi_node_on_receive_fields(lua_State *L, v3s16 p,
		const std::string &formname,
		const std::map<std::string, std::string> &fields,
		ServerActiveObject *sender);

extern struct EnumString es_DrawType[];
extern struct EnumString es_ContentParamType[];
extern struct EnumString es_ContentParamType2[];
extern struct EnumString es_LiquidType[];
extern struct EnumString es_NodeBoxType[];

#endif /* LUA_NODE_H_ */
