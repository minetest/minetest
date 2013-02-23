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

#include "server.h"
#include "script.h"

#include "lua_itemstack.h"
#include "lua_types.h"
#include "lua_common.h"

//TODO move this macro to a more generic place
#define method(class, name) {#name, class::l_##name}

// garbage collector
int LuaItemStack::gc_object(lua_State *L)
{
	LuaItemStack *o = *(LuaItemStack **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// is_empty(self) -> true/false
int LuaItemStack::l_is_empty(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushboolean(L, item.empty());
	return 1;
}

// get_name(self) -> string
int LuaItemStack::l_get_name(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushstring(L, item.name.c_str());
	return 1;
}

// get_count(self) -> number
int LuaItemStack::l_get_count(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.count);
	return 1;
}

// get_wear(self) -> number
int LuaItemStack::l_get_wear(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.wear);
	return 1;
}

// get_metadata(self) -> string
int LuaItemStack::l_get_metadata(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushlstring(L, item.metadata.c_str(), item.metadata.size());
	return 1;
}

// clear(self) -> true
int LuaItemStack::l_clear(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	o->m_stack.clear();
	lua_pushboolean(L, true);
	return 1;
}

// replace(self, itemstack or itemstring or table or nil) -> true
int LuaItemStack::l_replace(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	o->m_stack = read_item(L, 2);
	lua_pushboolean(L, true);
	return 1;
}

// to_string(self) -> string
int LuaItemStack::l_to_string(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	std::string itemstring = o->m_stack.getItemString();
	lua_pushstring(L, itemstring.c_str());
	return 1;
}

// to_table(self) -> table or nil
int LuaItemStack::l_to_table(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	const ItemStack &item = o->m_stack;
	if(item.empty())
	{
		lua_pushnil(L);
	}
	else
	{
		lua_newtable(L);
		lua_pushstring(L, item.name.c_str());
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, item.count);
		lua_setfield(L, -2, "count");
		lua_pushinteger(L, item.wear);
		lua_setfield(L, -2, "wear");
		lua_pushlstring(L, item.metadata.c_str(), item.metadata.size());
		lua_setfield(L, -2, "metadata");
	}
	return 1;
}

// get_stack_max(self) -> number
int LuaItemStack::l_get_stack_max(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.getStackMax(get_server(L)->idef()));
	return 1;
}

// get_free_space(self) -> number
int LuaItemStack::l_get_free_space(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.freeSpace(get_server(L)->idef()));
	return 1;
}

// is_known(self) -> true/false
// Checks if the item is defined.
int LuaItemStack::l_is_known(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	bool is_known = item.isKnown(get_server(L)->idef());
	lua_pushboolean(L, is_known);
	return 1;
}

// get_definition(self) -> table
// Returns the item definition table from minetest.registered_items,
// or a fallback one (name="unknown")
int LuaItemStack::l_get_definition(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;

	// Get minetest.registered_items[name]
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_items");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, item.name.c_str());
	if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_getfield(L, -1, "unknown");
	}
	return 1;
}

// get_tool_capabilities(self) -> table
// Returns the effective tool digging properties.
// Returns those of the hand ("") if this item has none associated.
int LuaItemStack::l_get_tool_capabilities(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	const ToolCapabilities &prop =
		item.getToolCapabilities(get_server(L)->idef());
	push_tool_capabilities(L, prop);
	return 1;
}

// add_wear(self, amount) -> true/false
// The range for "amount" is [0,65535]. Wear is only added if the item
// is a tool. Adding wear might destroy the item.
// Returns true if the item is (or was) a tool.
int LuaItemStack::l_add_wear(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	int amount = lua_tointeger(L, 2);
	bool result = item.addWear(amount, get_server(L)->idef());
	lua_pushboolean(L, result);
	return 1;
}

// add_item(self, itemstack or itemstring or table or nil) -> itemstack
// Returns leftover item stack
int LuaItemStack::l_add_item(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	ItemStack newitem = read_item(L, 2);
	ItemStack leftover = item.addItem(newitem, get_server(L)->idef());
	create(L, leftover);
	return 1;
}

// item_fits(self, itemstack or itemstring or table or nil) -> true/false, itemstack
// First return value is true iff the new item fits fully into the stack
// Second return value is the would-be-left-over item stack
int LuaItemStack::l_item_fits(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	ItemStack newitem = read_item(L, 2);
	ItemStack restitem;
	bool fits = item.itemFits(newitem, &restitem, get_server(L)->idef());
	lua_pushboolean(L, fits);  // first return value
	create(L, restitem);       // second return value
	return 2;
}

// take_item(self, takecount=1) -> itemstack
int LuaItemStack::l_take_item(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	u32 takecount = 1;
	if(!lua_isnone(L, 2))
		takecount = luaL_checkinteger(L, 2);
	ItemStack taken = item.takeItem(takecount);
	create(L, taken);
	return 1;
}

// peek_item(self, peekcount=1) -> itemstack
int LuaItemStack::l_peek_item(lua_State *L)
{
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	u32 peekcount = 1;
	if(!lua_isnone(L, 2))
		peekcount = lua_tointeger(L, 2);
	ItemStack peekaboo = item.peekItem(peekcount);
	create(L, peekaboo);
	return 1;
}

LuaItemStack::LuaItemStack(const ItemStack &item):
	m_stack(item)
{
}

LuaItemStack::~LuaItemStack()
{
}

const ItemStack& LuaItemStack::getItem() const
{
	return m_stack;
}
ItemStack& LuaItemStack::getItem()
{
	return m_stack;
}

// LuaItemStack(itemstack or itemstring or table or nil)
// Creates an LuaItemStack and leaves it on top of stack
int LuaItemStack::create_object(lua_State *L)
{
	ItemStack item = read_item(L, 1);
	LuaItemStack *o = new LuaItemStack(item);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}
// Not callable from Lua
int LuaItemStack::create(lua_State *L, const ItemStack &item)
{
	LuaItemStack *o = new LuaItemStack(item);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

LuaItemStack* LuaItemStack::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(LuaItemStack**)ud;  // unbox pointer
}

void LuaItemStack::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_openlib(L, 0, methods, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable

	// Can be created from Lua (LuaItemStack(itemstack or itemstring or table or nil))
	lua_register(L, className, create_object);
}

const char LuaItemStack::className[] = "ItemStack";
const luaL_reg LuaItemStack::methods[] = {
	method(LuaItemStack, is_empty),
	method(LuaItemStack, get_name),
	method(LuaItemStack, get_count),
	method(LuaItemStack, get_wear),
	method(LuaItemStack, get_metadata),
	method(LuaItemStack, clear),
	method(LuaItemStack, replace),
	method(LuaItemStack, to_string),
	method(LuaItemStack, to_table),
	method(LuaItemStack, get_stack_max),
	method(LuaItemStack, get_free_space),
	method(LuaItemStack, is_known),
	method(LuaItemStack, get_definition),
	method(LuaItemStack, get_tool_capabilities),
	method(LuaItemStack, add_wear),
	method(LuaItemStack, add_item),
	method(LuaItemStack, item_fits),
	method(LuaItemStack, take_item),
	method(LuaItemStack, peek_item),
	{0,0}
};

ItemStack read_item(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(lua_isnil(L, index))
	{
		return ItemStack();
	}
	else if(lua_isuserdata(L, index))
	{
		// Convert from LuaItemStack
		LuaItemStack *o = LuaItemStack::checkobject(L, index);
		return o->getItem();
	}
	else if(lua_isstring(L, index))
	{
		// Convert from itemstring
		std::string itemstring = lua_tostring(L, index);
		IItemDefManager *idef = get_server(L)->idef();
		try
		{
			ItemStack item;
			item.deSerialize(itemstring, idef);
			return item;
		}
		catch(SerializationError &e)
		{
			infostream<<"WARNING: unable to create item from itemstring"
					<<": "<<itemstring<<std::endl;
			return ItemStack();
		}
	}
	else if(lua_istable(L, index))
	{
		// Convert from table
		IItemDefManager *idef = get_server(L)->idef();
		std::string name = getstringfield_default(L, index, "name", "");
		int count = getintfield_default(L, index, "count", 1);
		int wear = getintfield_default(L, index, "wear", 0);
		std::string metadata = getstringfield_default(L, index, "metadata", "");
		return ItemStack(name, count, wear, metadata, idef);
	}
	else
	{
		throw LuaError(L, "Expecting itemstack, itemstring, table or nil");
	}
}

std::vector<ItemStack> read_items(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	std::vector<ItemStack> items;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_pushnil(L);
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		items.push_back(read_item(L, -1));
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	return items;
}

// creates a table of ItemStacks
void push_items(lua_State *L, const std::vector<ItemStack> &items)
{
	// Get the table insert function
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	// Create and fill table
	lua_newtable(L);
	int table = lua_gettop(L);
	for(u32 i=0; i<items.size(); i++){
		ItemStack item = items[i];
		lua_pushvalue(L, table_insert);
		lua_pushvalue(L, table);
		LuaItemStack::create(L, item);
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
	lua_remove(L, -2); // Remove table
	lua_remove(L, -2); // Remove insert
}







