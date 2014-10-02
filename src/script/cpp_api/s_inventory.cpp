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

#include "cpp_api/s_inventory.h"
#include "cpp_api/s_internal.h"
#include "inventorymanager.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "log.h"

// Return number of accepted items to be moved
int ScriptApiDetached::detached_inventory_AllowMove(
		const std::string &name,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push callback function on stack
	if (!getDetachedInventoryCallback(name, "allow_move"))
		return count;

	// function(inv, from_list, from_index, to_list, to_index, count, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	lua_pushstring(L, from_list.c_str()); // from_list
	lua_pushinteger(L, from_index + 1);   // from_index
	lua_pushstring(L, to_list.c_str());   // to_list
	lua_pushinteger(L, to_index + 1);     // to_index
	lua_pushinteger(L, count);            // count
	objectrefGetOrCreate(L, player);      // player
	if (lua_pcall(L, 7, 1, m_errorhandler))
		scriptError();
	if(!lua_isnumber(L, -1))
		throw LuaError("allow_move should return a number. name=" + name);
	int ret = luaL_checkinteger(L, -1);
	lua_pop(L, 1); // Pop integer
	return ret;
}

// Return number of accepted items to be put
int ScriptApiDetached::detached_inventory_AllowPut(
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push callback function on stack
	if (!getDetachedInventoryCallback(name, "allow_put"))
		return stack.count; // All will be accepted

	// Call function(inv, listname, index, stack, player)
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);              // inv
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 1, m_errorhandler))
		scriptError();
	if (!lua_isnumber(L, -1))
		throw LuaError("allow_put should return a number. name=" + name);
	int ret = luaL_checkinteger(L, -1);
	lua_pop(L, 1); // Pop integer
	return ret;
}

// Return number of accepted items to be taken
int ScriptApiDetached::detached_inventory_AllowTake(
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push callback function on stack
	if (!getDetachedInventoryCallback(name, "allow_take"))
		return stack.count; // All will be accepted

	// Call function(inv, listname, index, stack, player)
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);              // inv
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 1, m_errorhandler))
		scriptError();
	if (!lua_isnumber(L, -1))
		throw LuaError("allow_take should return a number. name=" + name);
	int ret = luaL_checkinteger(L, -1);
	lua_pop(L, 1); // Pop integer
	return ret;
}

// Report moved items
void ScriptApiDetached::detached_inventory_OnMove(
		const std::string &name,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push callback function on stack
	if (!getDetachedInventoryCallback(name, "on_move"))
		return;

	// function(inv, from_list, from_index, to_list, to_index, count, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	lua_pushstring(L, from_list.c_str()); // from_list
	lua_pushinteger(L, from_index + 1);   // from_index
	lua_pushstring(L, to_list.c_str());   // to_list
	lua_pushinteger(L, to_index + 1);     // to_index
	lua_pushinteger(L, count);            // count
	objectrefGetOrCreate(L, player);      // player
	if (lua_pcall(L, 7, 0, m_errorhandler))
		scriptError();
}

// Report put items
void ScriptApiDetached::detached_inventory_OnPut(
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push callback function on stack
	if (!getDetachedInventoryCallback(name, "on_put"))
		return;

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 0, m_errorhandler))
		scriptError();
}

// Report taken items
void ScriptApiDetached::detached_inventory_OnTake(
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Push callback function on stack
	if (!getDetachedInventoryCallback(name, "on_take"))
		return;

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 0, m_errorhandler))
		scriptError();
}

// Retrieves core.detached_inventories[name][callbackname]
// If that is nil or on error, return false and stack is unchanged
// If that is a function, returns true and pushes the
// function onto the stack
bool ScriptApiDetached::getDetachedInventoryCallback(
		const std::string &name, const char *callbackname)
{
	lua_State *L = getStack();

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "detached_inventories");
	lua_remove(L, -2);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, name.c_str());
	lua_remove(L, -2);
	// Should be a table
	if(lua_type(L, -1) != LUA_TTABLE)
	{
		errorstream<<"Detached inventory \""<<name<<"\" not defined"<<std::endl;
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
