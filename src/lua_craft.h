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

#ifndef LUA_CRAFT_H_
#define LUA_CRAFT_H_

#include <vector>

extern "C" {
#include <lua.h>
}

#include "craftdef.h"

/*****************************************************************************/
/* Mod API                                                                   */
/*****************************************************************************/
int l_register_craft(lua_State *L);
int l_get_craft_recipe(lua_State *L);
int l_get_craft_result(lua_State *L);

/*****************************************************************************/
/* scriptapi internal                                                        */
/*****************************************************************************/
bool read_craft_replacements(lua_State *L, int index,
		CraftReplacements &replacements);
bool read_craft_recipe_shapeless(lua_State *L, int index,
		std::vector<std::string> &recipe);
bool read_craft_recipe_shaped(lua_State *L, int index,
		int &width, std::vector<std::string> &recipe);

extern struct EnumString es_CraftMethod[];

#endif /* LUA_CRAFT_H_ */
