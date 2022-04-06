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
#include "inventory.h"  // ItemStack

class LuaItemStack : public ModApiBase {
private:
	ItemStack m_stack;

	static const char className[];
	static const luaL_Reg methods[];

	// Exported functions

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	// __tostring metamethod
	ENTRY_POINT_DECL(mt_tostring);

	// is_empty(self) -> true/false
	ENTRY_POINT_DECL(l_is_empty);

	// get_name(self) -> string
	ENTRY_POINT_DECL(l_get_name);

	// set_name(self, name)
	ENTRY_POINT_DECL(l_set_name);

	// get_count(self) -> number
	ENTRY_POINT_DECL(l_get_count);

	// set_count(self, number)
	ENTRY_POINT_DECL(l_set_count);

	// get_wear(self) -> number
	ENTRY_POINT_DECL(l_get_wear);

	// set_wear(self, number)
	ENTRY_POINT_DECL(l_set_wear);

	// get_meta(self) -> string
	ENTRY_POINT_DECL(l_get_meta);

	// DEPRECATED
	// get_metadata(self) -> string
	ENTRY_POINT_DECL(l_get_metadata);

	// DEPRECATED
	// set_metadata(self, string)
	ENTRY_POINT_DECL(l_set_metadata);

	// get_description(self)
	ENTRY_POINT_DECL(l_get_description);

	// get_short_description(self)
	ENTRY_POINT_DECL(l_get_short_description);

	// clear(self) -> true
	ENTRY_POINT_DECL(l_clear);

	// replace(self, itemstack or itemstring or table or nil) -> true
	ENTRY_POINT_DECL(l_replace);

	// to_string(self) -> string
	ENTRY_POINT_DECL(l_to_string);

	// to_table(self) -> table or nil
	ENTRY_POINT_DECL(l_to_table);

	// get_stack_max(self) -> number
	ENTRY_POINT_DECL(l_get_stack_max);

	// get_free_space(self) -> number
	ENTRY_POINT_DECL(l_get_free_space);

	// is_known(self) -> true/false
	// Checks if the item is defined.
	ENTRY_POINT_DECL(l_is_known);

	// get_definition(self) -> table
	// Returns the item definition table from core.registered_items,
	// or a fallback one (name="unknown")
	ENTRY_POINT_DECL(l_get_definition);

	// get_tool_capabilities(self) -> table
	// Returns the effective tool digging properties.
	// Returns those of the hand ("") if this item has none associated.
	ENTRY_POINT_DECL(l_get_tool_capabilities);

	// add_wear(self, amount) -> true/false
	// The range for "amount" is [0,65535]. Wear is only added if the item
	// is a tool. Adding wear might destroy the item.
	// Returns true if the item is (or was) a tool.
	ENTRY_POINT_DECL(l_add_wear);

	// add_item(self, itemstack or itemstring or table or nil) -> itemstack
	// Returns leftover item stack
	ENTRY_POINT_DECL(l_add_item);

	// item_fits(self, itemstack or itemstring or table or nil) -> true/false, itemstack
	// First return value is true iff the new item fits fully into the stack
	// Second return value is the would-be-left-over item stack
	ENTRY_POINT_DECL(l_item_fits);

	// take_item(self, takecount=1) -> itemstack
	ENTRY_POINT_DECL(l_take_item);

	// peek_item(self, peekcount=1) -> itemstack
	ENTRY_POINT_DECL(l_peek_item);

public:
	LuaItemStack(const ItemStack &item);
	~LuaItemStack() = default;

	const ItemStack& getItem() const;
	ItemStack& getItem();

	// LuaItemStack(itemstack or itemstring or table or nil)
	// Creates an LuaItemStack and leaves it on top of stack
	ENTRY_POINT_DECL(create_object);
	// Not callable from Lua
	static int create(lua_State *L, const ItemStack &item);
	static LuaItemStack* checkobject(lua_State *L, int narg);
	static void Register(lua_State *L);

};

class ModApiItemMod : public ModApiBase {
private:
	ENTRY_POINT_DECL(l_register_item_raw);
	ENTRY_POINT_DECL(l_unregister_item_raw);
	ENTRY_POINT_DECL(l_register_alias_raw);
	ENTRY_POINT_DECL(l_get_content_id);
	ENTRY_POINT_DECL(l_get_name_from_content_id);
public:
	static void Initialize(lua_State *L, int top);
};
