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
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"
#include "nodedef.h"
#include "mapnode.h"
#include "server.h"
#include "environment.h"
#include "lua_api/l_item.h"

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
	if (node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	std::string nodename = ndef->get(node).name;
	if (!getItemCallback(nodename.c_str(), "allow_metadata_inventory_move"))
		return count;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	push_v3s16(L, p);                     // pos
	lua_pushstring(L, from_list.c_str()); // from_list
	lua_pushinteger(L, from_index + 1);   // from_index
	lua_pushstring(L, to_list.c_str());   // to_list
	lua_pushinteger(L, to_index + 1);     // to_index
	lua_pushinteger(L, count);            // count
	objectrefGetOrCreate(L, player);      // player
	if (lua_pcall(L, 7, 1, m_errorhandler))
		scriptError();
	if (!lua_isnumber(L, -1))
		throw LuaError("allow_metadata_inventory_move should"
				" return a number, guilty node: " + nodename);
	int num = luaL_checkinteger(L, -1);
	lua_pop(L, 1); // Pop integer
	return num;
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
	if (node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	std::string nodename = ndef->get(node).name;
	if (!getItemCallback(nodename.c_str(), "allow_metadata_inventory_put"))
		return stack.count;

	// Call function(pos, listname, index, stack, player)
	push_v3s16(L, p);                    // pos
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 1, m_errorhandler))
		scriptError();
	if(!lua_isnumber(L, -1))
		throw LuaError("allow_metadata_inventory_put should"
				" return a number, guilty node: " + nodename);
	int num = luaL_checkinteger(L, -1);
	lua_pop(L, 1); // Pop integer
	return num;
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
	if (node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	std::string nodename = ndef->get(node).name;
	if (!getItemCallback(nodename.c_str(), "allow_metadata_inventory_take"))
		return stack.count;

	// Call function(pos, listname, index, count, player)
	push_v3s16(L, p);                    // pos
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 1, m_errorhandler))
		scriptError();
	if (!lua_isnumber(L, -1))
		throw LuaError("allow_metadata_inventory_take should"
				" return a number, guilty node: " + nodename);
	int num = luaL_checkinteger(L, -1);
	lua_pop(L, 1); // Pop integer
	return num;
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
	if (node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	std::string nodename = ndef->get(node).name;
	if (!getItemCallback(nodename.c_str(), "on_metadata_inventory_move"))
		return;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	push_v3s16(L, p);                     // pos
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
void ScriptApiNodemeta::nodemeta_inventory_OnPut(v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	INodeDefManager *ndef = getServer()->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = getEnv()->getMap().getNodeNoEx(p);
	if (node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	std::string nodename = ndef->get(node).name;
	if (!getItemCallback(nodename.c_str(), "on_metadata_inventory_put"))
		return;

	// Call function(pos, listname, index, stack, player)
	push_v3s16(L, p);                    // pos
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 0, m_errorhandler))
		scriptError();
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
	if (node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	std::string nodename = ndef->get(node).name;
	if (!getItemCallback(nodename.c_str(), "on_metadata_inventory_take"))
		return;

	// Call function(pos, listname, index, stack, player)
	push_v3s16(L, p);                    // pos
	lua_pushstring(L, listname.c_str()); // listname
	lua_pushinteger(L, index + 1);       // index
	LuaItemStack::create(L, stack);      // stack
	objectrefGetOrCreate(L, player);     // player
	if (lua_pcall(L, 5, 0, m_errorhandler))
		scriptError();
}

ScriptApiNodemeta::ScriptApiNodemeta() {
}

ScriptApiNodemeta::~ScriptApiNodemeta() {
}

