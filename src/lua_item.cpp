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


extern "C" {
#include <lauxlib.h>
}
#include "script.h"
#include "tool.h"
#include "nodedef.h"
#include "util/pointedthing.h"

#include "lua_item.h"
#include "lua_enum.h"
#include "lua_types.h"
#include "lua_common.h"
#include "lua_itemstack.h"
#include "lua_object.h"
#include "lua_content.h"

/*
	ItemDefinition
*/

ItemDefinition read_item_definition(lua_State *L, int index,
		ItemDefinition default_def)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	// Read the item definition
	ItemDefinition def = default_def;

	def.type = (ItemType)getenumfield(L, index, "type",
			es_ItemType, ITEM_NONE);
	getstringfield(L, index, "name", def.name);
	getstringfield(L, index, "description", def.description);
	getstringfield(L, index, "inventory_image", def.inventory_image);
	getstringfield(L, index, "wield_image", def.wield_image);

	lua_getfield(L, index, "wield_scale");
	if(lua_istable(L, -1)){
		def.wield_scale = check_v3f(L, -1);
	}
	lua_pop(L, 1);

	def.stack_max = getintfield_default(L, index, "stack_max", def.stack_max);
	if(def.stack_max == 0)
		def.stack_max = 1;

	lua_getfield(L, index, "on_use");
	def.usable = lua_isfunction(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "on_rightclick");
	def.rightclickable = lua_isfunction(L, -1);
	lua_pop(L, 1);

	getboolfield(L, index, "liquids_pointable", def.liquids_pointable);

	warn_if_field_exists(L, index, "tool_digging_properties",
			"deprecated: use tool_capabilities");

	lua_getfield(L, index, "tool_capabilities");
	if(lua_istable(L, -1)){
		def.tool_capabilities = new ToolCapabilities(
				read_tool_capabilities(L, -1));
	}

	// If name is "" (hand), ensure there are ToolCapabilities
	// because it will be looked up there whenever any other item has
	// no ToolCapabilities
	if(def.name == "" && def.tool_capabilities == NULL){
		def.tool_capabilities = new ToolCapabilities();
	}

	lua_getfield(L, index, "groups");
	read_groups(L, -1, def.groups);
	lua_pop(L, 1);

	// Client shall immediately place this node when player places the item.
	// Server will update the precise end result a moment later.
	// "" = no prediction
	getstringfield(L, index, "node_placement_prediction",
			def.node_placement_prediction);

	return def;
}

// register_item_raw({lots of stuff})
int l_register_item_raw(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	int table = 1;

	// Get the writable item and node definition managers from the server
	IWritableItemDefManager *idef =
			get_server(L)->getWritableItemDefManager();
	IWritableNodeDefManager *ndef =
			get_server(L)->getWritableNodeDefManager();

	// Check if name is defined
	std::string name;
	lua_getfield(L, table, "name");
	if(lua_isstring(L, -1)){
		name = lua_tostring(L, -1);
		verbosestream<<"register_item_raw: "<<name<<std::endl;
	} else {
		throw LuaError(L, "register_item_raw: name is not defined or not a string");
	}

	// Check if on_use is defined

	ItemDefinition def;
	// Set a distinctive default value to check if this is set
	def.node_placement_prediction = "__default";

	// Read the item definition
	def = read_item_definition(L, table, def);

	// Default to having client-side placement prediction for nodes
	// ("" in item definition sets it off)
	if(def.node_placement_prediction == "__default"){
		if(def.type == ITEM_NODE && !def.rightclickable)
			def.node_placement_prediction = name;
		else
			def.node_placement_prediction = "";
	}

	// Register item definition
	idef->registerItem(def);

	// Read the node definition (content features) and register it
	if(def.type == ITEM_NODE)
	{
		ContentFeatures f = read_content_features(L, table);
		ndef->set(f.name, f);
	}

	return 0; /* number of results */
}

// register_alias_raw(name, convert_to_name)
int l_register_alias_raw(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	std::string convert_to = luaL_checkstring(L, 2);

	// Get the writable item definition manager from the server
	IWritableItemDefManager *idef =
			get_server(L)->getWritableItemDefManager();

	idef->registerAlias(name, convert_to);

	return 0; /* number of results */
}

// Retrieves minetest.registered_items[name][callbackname]
// If that is nil or on error, return false and stack is unchanged
// If that is a function, returns true and pushes the
// function onto the stack
// If minetest.registered_items[name] doesn't exist, minetest.nodedef_default
// is tried instead so unknown items can still be manipulated to some degree
bool get_item_callback(lua_State *L,
		const char *name, const char *callbackname)
{
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_items");
	lua_remove(L, -2);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, name);
	lua_remove(L, -2);
	// Should be a table
	if(lua_type(L, -1) != LUA_TTABLE)
	{
		// Report error and clean up
		errorstream<<"Item \""<<name<<"\" not defined"<<std::endl;
		lua_pop(L, 1);

		// Try minetest.nodedef_default instead
		lua_getglobal(L, "minetest");
		lua_getfield(L, -1, "nodedef_default");
		lua_remove(L, -2);
		luaL_checktype(L, -1, LUA_TTABLE);
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
		errorstream<<"Item \""<<name<<"\" callback \""
			<<callbackname<<" is not a function"<<std::endl;
		lua_pop(L, 1);
		return false;
	}
}

bool scriptapi_item_on_drop(lua_State *L, ItemStack &item,
		ServerActiveObject *dropper, v3f pos)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_item_callback(L, item.name.c_str(), "on_drop"))
		return false;

	// Call function
	LuaItemStack::create(L, item);
	objectref_get_or_create(L, dropper);
	pushFloatPos(L, pos);
	if(lua_pcall(L, 3, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnil(L, -1))
		item = read_item(L, -1);
	return true;
}

bool scriptapi_item_on_place(lua_State *L, ItemStack &item,
		ServerActiveObject *placer, const PointedThing &pointed)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_item_callback(L, item.name.c_str(), "on_place"))
		return false;

	// Call function
	LuaItemStack::create(L, item);
	objectref_get_or_create(L, placer);
	push_pointed_thing(L, pointed);
	if(lua_pcall(L, 3, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnil(L, -1))
		item = read_item(L, -1);
	return true;
}

bool scriptapi_item_on_use(lua_State *L, ItemStack &item,
		ServerActiveObject *user, const PointedThing &pointed)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_item_callback(L, item.name.c_str(), "on_use"))
		return false;

	// Call function
	LuaItemStack::create(L, item);
	objectref_get_or_create(L, user);
	push_pointed_thing(L, pointed);
	if(lua_pcall(L, 3, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnil(L, -1))
		item = read_item(L, -1);
	return true;
}
