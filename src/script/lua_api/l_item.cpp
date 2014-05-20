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

#include "lua_api/l_item.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "itemdef.h"
#include "nodedef.h"
#include "server.h"
#include "content_sao.h"
#include "inventory.h"
#include "log.h"


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
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushboolean(L, item.empty());
	return 1;
}

// get_name(self) -> string
int LuaItemStack::l_get_name(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushstring(L, item.name.c_str());
	return 1;
}

// set_name(self, name)
int LuaItemStack::l_set_name(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	item.name = luaL_checkstring(L, 2);

	if (item.name == "" || item.empty())
		item.clear();

	return 1;
}

// get_count(self) -> number
int LuaItemStack::l_get_count(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.count);
	return 1;
}

// set_count(self, number)
int LuaItemStack::l_set_count(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	item.count = luaL_checkinteger(L, 2);

	if (item.name == "" || item.empty())
		item.clear();

	return 1;
}

// get_wear(self) -> number
int LuaItemStack::l_get_wear(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.wear);
	return 1;
}

// set_wear(self, number)
int LuaItemStack::l_set_wear(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	item.wear = luaL_checkinteger(L, 2);

	if (item.wear > 65535)
		item.clear();

	return 1;
}

// get_metadata(self) -> string
int LuaItemStack::l_get_metadata(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushlstring(L, item.metadata.c_str(), item.metadata.size());
	return 1;
}

// set_metadata(self, string)
int LuaItemStack::l_set_metadata(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;

	size_t len = 0;
	const char *ptr = luaL_checklstring(L, 2, &len);
	if (ptr)
		item.metadata.assign(ptr, len);
	else
		item.metadata = "";

	return 1;
}

// clear(self) -> true
int LuaItemStack::l_clear(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	o->m_stack.clear();
	lua_pushboolean(L, true);
	return 1;
}

// replace(self, itemstack or itemstring or table or nil) -> true
int LuaItemStack::l_replace(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	o->m_stack = read_item(L,2,getServer(L));
	lua_pushboolean(L, true);
	return 1;
}

// to_string(self) -> string
int LuaItemStack::l_to_string(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	std::string itemstring = o->m_stack.getItemString();
	lua_pushstring(L, itemstring.c_str());
	return 1;
}

// to_table(self) -> table or nil
int LuaItemStack::l_to_table(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
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
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.getStackMax(getServer(L)->idef()));
	return 1;
}

// get_free_space(self) -> number
int LuaItemStack::l_get_free_space(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	lua_pushinteger(L, item.freeSpace(getServer(L)->idef()));
	return 1;
}

// is_known(self) -> true/false
// Checks if the item is defined.
int LuaItemStack::l_is_known(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	bool is_known = item.isKnown(getServer(L)->idef());
	lua_pushboolean(L, is_known);
	return 1;
}

// get_definition(self) -> table
// Returns the item definition table from registered_items,
// or a fallback one (name="unknown")
int LuaItemStack::l_get_definition(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;

	// Get registered_items[name]
	lua_getglobal(L, "core");
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
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	const ToolCapabilities &prop =
		item.getToolCapabilities(getServer(L)->idef());
	push_tool_capabilities(L, prop);
	return 1;
}

// add_wear(self, amount) -> true/false
// The range for "amount" is [0,65535]. Wear is only added if the item
// is a tool. Adding wear might destroy the item.
// Returns true if the item is (or was) a tool.
int LuaItemStack::l_add_wear(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	int amount = lua_tointeger(L, 2);
	bool result = item.addWear(amount, getServer(L)->idef());
	lua_pushboolean(L, result);
	return 1;
}

// add_item(self, itemstack or itemstring or table or nil) -> itemstack
// Returns leftover item stack
int LuaItemStack::l_add_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	ItemStack newitem = read_item(L,-1, getServer(L));
	ItemStack leftover = item.addItem(newitem, getServer(L)->idef());
	create(L, leftover);
	return 1;
}

// item_fits(self, itemstack or itemstring or table or nil) -> true/false, itemstack
// First return value is true iff the new item fits fully into the stack
// Second return value is the would-be-left-over item stack
int LuaItemStack::l_item_fits(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaItemStack *o = checkobject(L, 1);
	ItemStack &item = o->m_stack;
	ItemStack newitem = read_item(L, 2, getServer(L));
	ItemStack restitem;
	bool fits = item.itemFits(newitem, &restitem, getServer(L)->idef());
	lua_pushboolean(L, fits);  // first return value
	create(L, restitem);       // second return value
	return 2;
}

// take_item(self, takecount=1) -> itemstack
int LuaItemStack::l_take_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
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
	NO_MAP_LOCK_REQUIRED;
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
	NO_MAP_LOCK_REQUIRED;
	ItemStack item = read_item(L, 1, getServer(L));
	LuaItemStack *o = new LuaItemStack(item);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}
// Not callable from Lua
int LuaItemStack::create(lua_State *L, const ItemStack &item)
{
	NO_MAP_LOCK_REQUIRED;
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
	luamethod(LuaItemStack, is_empty),
	luamethod(LuaItemStack, get_name),
	luamethod(LuaItemStack, set_name),
	luamethod(LuaItemStack, get_count),
	luamethod(LuaItemStack, set_count),
	luamethod(LuaItemStack, get_wear),
	luamethod(LuaItemStack, set_wear),
	luamethod(LuaItemStack, get_metadata),
	luamethod(LuaItemStack, set_metadata),
	luamethod(LuaItemStack, clear),
	luamethod(LuaItemStack, replace),
	luamethod(LuaItemStack, to_string),
	luamethod(LuaItemStack, to_table),
	luamethod(LuaItemStack, get_stack_max),
	luamethod(LuaItemStack, get_free_space),
	luamethod(LuaItemStack, is_known),
	luamethod(LuaItemStack, get_definition),
	luamethod(LuaItemStack, get_tool_capabilities),
	luamethod(LuaItemStack, add_wear),
	luamethod(LuaItemStack, add_item),
	luamethod(LuaItemStack, item_fits),
	luamethod(LuaItemStack, take_item),
	luamethod(LuaItemStack, peek_item),
	{0,0}
};

/*
	ItemDefinition
*/

// register_item_raw({lots of stuff})
int ModApiItemMod::l_register_item_raw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	luaL_checktype(L, 1, LUA_TTABLE);
	int table = 1;

	// Get the writable item and node definition managers from the server
	IWritableItemDefManager *idef =
			getServer(L)->getWritableItemDefManager();
	IWritableNodeDefManager *ndef =
			getServer(L)->getWritableNodeDefManager();

	// Check if name is defined
	std::string name;
	lua_getfield(L, table, "name");
	if(lua_isstring(L, -1)){
		name = lua_tostring(L, -1);
		verbosestream<<"register_item_raw: "<<name<<std::endl;
	} else {
		throw LuaError("register_item_raw: name is not defined or not a string");
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
		if(def.type == ITEM_NODE)
			def.node_placement_prediction = name;
		else
			def.node_placement_prediction = "";
	}

	// Register item definition
	idef->registerItem(def);

	// Read the node definition (content features) and register it
	if(def.type == ITEM_NODE){
		ContentFeatures f = read_content_features(L, table);
		content_t id = ndef->set(f.name, f);

		if(id > MAX_REGISTERED_CONTENT){
			throw LuaError("Number of registerable nodes ("
					+ itos(MAX_REGISTERED_CONTENT+1)
					+ ") exceeded (" + name + ")");
		}
	}

	return 0; /* number of results */
}

// register_alias_raw(name, convert_to_name)
int ModApiItemMod::l_register_alias_raw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);
	std::string convert_to = luaL_checkstring(L, 2);

	// Get the writable item definition manager from the server
	IWritableItemDefManager *idef =
			getServer(L)->getWritableItemDefManager();

	idef->registerAlias(name, convert_to);

	return 0; /* number of results */
}

// get_content_id(name)
int ModApiItemMod::l_get_content_id(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::string name = luaL_checkstring(L, 1);

	INodeDefManager *ndef = getServer(L)->getNodeDefManager();
	content_t c = ndef->getId(name);
	
	lua_pushinteger(L, c);
	return 1; /* number of results */
}

// get_name_from_content_id(name)
int ModApiItemMod::l_get_name_from_content_id(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	content_t c = luaL_checkint(L, 1);

	INodeDefManager *ndef = getServer(L)->getNodeDefManager();
	const char *name = ndef->get(c).name.c_str();
	
	lua_pushstring(L, name);
	return 1; /* number of results */
}

void ModApiItemMod::Initialize(lua_State *L, int top)
{
	API_FCT(register_item_raw);
	API_FCT(register_alias_raw);
	API_FCT(get_content_id);
	API_FCT(get_name_from_content_id);
}
