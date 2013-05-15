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

#include "cpp_api/s_nodemeta.h"
#include "common/c_converter.h"
#include "nodedef.h"
#include "mapnode.h"
#include "server.h"
#include "lua_api/l_item.h"

extern "C" {
#include "lauxlib.h"
}

// Return number of accepted items to be moved
int ScriptApiNodemeta::nodemeta_inventory_AllowMove(v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!getItemCallback(ndef->get(node).name.c_str(),
			"allow_metadata_inventory_move"))
		return count;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	// pos
	push_v3s16(L, p);
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
	objectrefGetOrCreate(player);
	if(lua_pcall(L, 7, 1, 0))
		scriptError("error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_move should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be put
int ScriptApiNodemeta::nodemeta_inventory_AllowPut(v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!getItemCallback(ndef->get(node).name.c_str(),
			"allow_metadata_inventory_put"))
		return stack.count;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectrefGetOrCreate(player);
	if(lua_pcall(L, 5, 1, 0))
		scriptError("error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_put should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be taken
int ScriptApiNodemeta::nodemeta_inventory_AllowTake(v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!getItemCallback(ndef->get(node).name.c_str(),
			"allow_metadata_inventory_take"))
		return stack.count;

	// Call function(pos, listname, index, count, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectrefGetOrCreate(player);
	if(lua_pcall(L, 5, 1, 0))
		scriptError("error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_take should return a number");
	return luaL_checkinteger(L, -1);
}

// Report moved items
void ScriptApiNodemeta::nodemeta_inventory_OnMove(v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!getItemCallback(ndef->get(node).name.c_str(),
			"on_metadata_inventory_move"))
		return;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	// pos
	push_v3s16(L, p);
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
	objectrefGetOrCreate(player);
	if(lua_pcall(L, 7, 0, 0))
		scriptError("error: %s", lua_tostring(L, -1));
}

// Report put items
void ScriptApiNodemeta::nodemeta_inventory_OnPut(v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!getItemCallback(ndef->get(node).name.c_str(),
			"on_metadata_inventory_put"))
		return;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectrefGetOrCreate(player);
	if(lua_pcall(L, 5, 0, 0))
		scriptError("error: %s", lua_tostring(L, -1));
}

// Report taken items
void ScriptApiNodemeta::nodemeta_inventory_OnTake(v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!getItemCallback(ndef->get(node).name.c_str(),
			"on_metadata_inventory_take"))
		return;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectrefGetOrCreate(player);
	if(lua_pcall(L, 5, 0, 0))
		scriptError("error: %s", lua_tostring(L, -1));
}

ScriptApiNodemeta::ScriptApiNodemeta() {
}

ScriptApiNodemeta::~ScriptApiNodemeta() {
}



