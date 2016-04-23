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
#ifndef L_ITEMSTACKMETA_H_
#define L_ITEMSTACKMETA_H_

#include "lua_api/l_base.h"
#include "irrlichttypes_bloated.h"
#include "inventory.h"

class ServerEnvironment;
class ItemStackMetadata;

/*
	ItemStackMetaRef
*/

class ItemStackMetaRef : public ModApiBase {
private:
	ItemStack *m_istack;

	static const char className[];
	static const luaL_reg methods[];

	static ItemStackMetaRef *checkobject(lua_State *L, int narg);

	/**
	 * Retrieve metadata for an itemstack (the one given as pointer as member variable).
	 * @return pointer to a @c ItemStackMetadata object.
	 */
	static ItemStackMetadata* getmeta(ItemStackMetaRef *ref);

	static void reportMetadataChange(ItemStackMetaRef *ref);

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// get_string(self, name)
	static int l_get_string(lua_State *L);

	// set_string(self, name, var)
	static int l_set_string(lua_State *L);

	// get_int(self, name)
	static int l_get_int(lua_State *L);

	// set_int(self, name, var)
	static int l_set_int(lua_State *L);

	// get_float(self, name)
	static int l_get_float(lua_State *L);

	// set_float(self, name, var)
	static int l_set_float(lua_State *L);

	// get_inventory(self)
	static int l_get_inventory(lua_State *L);

	// to_table(self)
	static int l_to_table(lua_State *L);

	// from_table(self, table)
	static int l_from_table(lua_State *L);

public:
	ItemStackMetaRef(ItemStack *istack);

	~ItemStackMetaRef();

	// Creates an ItemStackMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, ItemStack *istack);

	static void Register(lua_State *L);
};

#endif /* L_NODEMETA_H_ */
