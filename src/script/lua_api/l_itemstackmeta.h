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
#include "lua_api/l_metadata.h"
#include "irrlichttypes_bloated.h"
#include "inventory.h"

class ItemStackMetaRef : public MetaDataRef
{
private:
	ItemStack *istack;

	static const char className[];
	static const luaL_reg methods[];

	static ItemStackMetaRef *checkobject(lua_State *L, int narg);

	virtual Metadata* getmeta(bool auto_create);

	virtual void clearMeta();

	virtual void reportMetadataChange();

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);
public:
	ItemStackMetaRef(ItemStack *istack): istack(istack) {}
	~ItemStackMetaRef() {}

	// Creates an ItemStackMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, ItemStack *istack);

	static void Register(lua_State *L);
};

#endif
