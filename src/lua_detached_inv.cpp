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

#include "lua_detached_inv.h"
#include "script.h"
#include "log.h"

extern "C" {
#include <lauxlib.h>
}

#include "lua_inventory.h"
#include "lua_itemstack.h"
#include "lua_types.h"
#include "lua_object.h"
#include "lua_common.h"

/*
	Detached inventory callbacks
*/

// Retrieves minetest.detached_inventories[name][callbackname]
// If that is nil or on error, return false and stack is unchanged
// If that is a function, returns true and pushes the
// function onto the stack
static bool get_detached_inventory_callback(lua_State *L,
		const std::string &name, const char *callbackname)
{
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "detached_inventories");
	lua_remove(L, -2);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, name.c_str());
	lua_remove(L, -2);
	// Should be a table
	if(lua_type(L, -1) != LUA_TTABLE)
	{
		errorstream<<"Item \""<<name<<"\" not defined"<<std::endl;
		lua_pop(L, 1);
		return false;
	}
	lua_getfield(L, -1, callbackname);
	lua_remove(L, -2);
	// Should be a function or nil
	if(lua_type(L, -1) == LUA_TFUNCTION)
	{
		return true;
	}
	else if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}
	else
	{
		errorstream<<"Detached inventory \""<<name<<"\" callback \""
			<<callbackname<<"\" is not a function"<<std::endl;
		lua_pop(L, 1);
		return false;
	}
}

// Return number of accepted items to be moved
int scriptapi_detached_inventory_allow_move(lua_State *L,
		const std::string &name,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "allow_move"))
		return count;

	// function(inv, from_list, from_index, to_list, to_index, count, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_move should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be put
int scriptapi_detached_inventory_allow_put(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "allow_put"))
		return stack.count; // All will be accepted

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_put should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be taken
int scriptapi_detached_inventory_allow_take(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "allow_take"))
		return stack.count; // All will be accepted

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_take should return a number");
	return luaL_checkinteger(L, -1);
}

// Report moved items
void scriptapi_detached_inventory_on_move(lua_State *L,
		const std::string &name,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "on_move"))
		return;

	// function(inv, from_list, from_index, to_list, to_index, count, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report put items
void scriptapi_detached_inventory_on_put(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "on_put"))
		return;

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report taken items
void scriptapi_detached_inventory_on_take(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "on_take"))
		return;

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// create_detached_inventory_raw(name)
int l_create_detached_inventory_raw(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	if(get_server(L)->createDetachedInventory(name) != NULL){
		InventoryLocation loc;
		loc.setDetached(name);
		InvRef::create(L, loc);
	}else{
		lua_pushnil(L);
	}
	return 1;
}
