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

#ifndef LUA_ITEM_H_
#define LUA_ITEM_H_

extern "C" {
#include <lua.h>
}

#include <irr_v3d.h>

#include "itemdef.h"
#include "content_sao.h"
#include "util/pointedthing.h"
#include "inventory.h"

/*****************************************************************************/
/* Mod API                                                                   */
/*****************************************************************************/
int l_register_item_raw(lua_State *L);
int l_register_alias_raw(lua_State *L);

/*****************************************************************************/
/* scriptapi internal                                                        */
/*****************************************************************************/
bool get_item_callback(lua_State *L,
		const char *name, const char *callbackname);
ItemDefinition read_item_definition(lua_State *L, int index,
		ItemDefinition default_def = ItemDefinition());

/*****************************************************************************/
/* Minetest interface                                                        */
/*****************************************************************************/
bool scriptapi_item_on_drop(lua_State *L, ItemStack &item,
		ServerActiveObject *dropper, v3f pos);
bool scriptapi_item_on_place(lua_State *L, ItemStack &item,
		ServerActiveObject *placer, const PointedThing &pointed);
bool scriptapi_item_on_use(lua_State *L, ItemStack &item,
		ServerActiveObject *user, const PointedThing &pointed);


#endif /* LUA_ITEM_H_ */
