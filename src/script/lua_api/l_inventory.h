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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

#include "inventory.h"
#include "inventorymanager.h"

class RemotePlayer;

/*
	InvRef
*/

class InvRef : public ModApiBase {
private:
	InventoryLocation m_loc;

	static const char className[];
	static const luaL_Reg methods[];

	static InvRef *checkobject(lua_State *L, int narg);

	static Inventory* getinv(lua_State *L, InvRef *ref);

	static InventoryList* getlist(lua_State *L, InvRef *ref,
			const char *listname);

	static void reportInventoryChange(lua_State *L, InvRef *ref);

	// Exported functions

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	// is_empty(self, listname) -> true/false
	ENTRY_POINT_DECL(l_is_empty);

	// get_size(self, listname)
	ENTRY_POINT_DECL(l_get_size);

	// get_width(self, listname)
	ENTRY_POINT_DECL(l_get_width);

	// set_size(self, listname, size)
	ENTRY_POINT_DECL(l_set_size);

	// set_width(self, listname, size)
	ENTRY_POINT_DECL(l_set_width);

	// get_stack(self, listname, i) -> itemstack
	ENTRY_POINT_DECL(l_get_stack);

	// set_stack(self, listname, i, stack) -> true/false
	ENTRY_POINT_DECL(l_set_stack);

	// get_list(self, listname) -> list or nil
	ENTRY_POINT_DECL(l_get_list);

	// set_list(self, listname, list)
	ENTRY_POINT_DECL(l_set_list);

	// get_lists(self) -> list of InventoryLists
	ENTRY_POINT_DECL(l_get_lists);

	// set_lists(self, lists)
	ENTRY_POINT_DECL(l_set_lists);

	// add_item(self, listname, itemstack or itemstring or table or nil) -> itemstack
	// Returns the leftover stack
	ENTRY_POINT_DECL(l_add_item);

	// room_for_item(self, listname, itemstack or itemstring or table or nil) -> true/false
	// Returns true if the item completely fits into the list
	ENTRY_POINT_DECL(l_room_for_item);

	// contains_item(self, listname, itemstack or itemstring or table or nil, [match_meta]) -> true/false
	// Returns true if the list contains the given count of the given item name
	ENTRY_POINT_DECL(l_contains_item);

	// remove_item(self, listname, itemstack or itemstring or table or nil) -> itemstack
	// Returns the items that were actually removed
	ENTRY_POINT_DECL(l_remove_item);

	// get_location() -> location (like get_inventory(location))
	ENTRY_POINT_DECL(l_get_location);

public:
	InvRef(const InventoryLocation &loc);

	~InvRef() = default;

	// Creates an InvRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, const InventoryLocation &loc);
	static void Register(lua_State *L);
};

class ModApiInventory : public ModApiBase {
private:
	ENTRY_POINT_DECL(l_create_detached_inventory_raw);

	ENTRY_POINT_DECL(l_remove_detached_inventory_raw);

	ENTRY_POINT_DECL(l_get_inventory);

public:
	static void Initialize(lua_State *L, int top);
};
