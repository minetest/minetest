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

#include "lua_api/l_inventory.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_item.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"
#include "player.h"

/*
	InvRef
*/
InvRef* InvRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(InvRef**)ud;  // unbox pointer
}

Inventory* InvRef::getinv(lua_State *L, InvRef *ref)
{
	return getServer(L)->getInventory(ref->m_loc);
}

InventoryList* InvRef::getlist(lua_State *L, InvRef *ref,
		const char *listname)
{
	NO_MAP_LOCK_REQUIRED;
	Inventory *inv = getinv(L, ref);
	if(!inv)
		return NULL;
	return inv->getList(listname);
}

void InvRef::reportInventoryChange(lua_State *L, InvRef *ref)
{
	// Inform other things that the inventory has changed
	getServer(L)->setInventoryModified(ref->m_loc);
}

// Exported functions

// garbage collector
int InvRef::gc_object(lua_State *L) {
	InvRef *o = *(InvRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// is_empty(self, listname) -> true/false
int InvRef::l_is_empty(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	InventoryList *list = getlist(L, ref, listname);
	if(list && list->getUsedSlots() > 0){
		lua_pushboolean(L, false);
	} else {
		lua_pushboolean(L, true);
	}
	return 1;
}

// get_size(self, listname)
int InvRef::l_get_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	InventoryList *list = getlist(L, ref, listname);
	if(list){
		lua_pushinteger(L, list->getSize());
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

// get_width(self, listname)
int InvRef::l_get_width(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	InventoryList *list = getlist(L, ref, listname);
	if(list){
		lua_pushinteger(L, list->getWidth());
	} else {
		lua_pushinteger(L, 0);
	}
	return 1;
}

// set_size(self, listname, size)
int InvRef::l_set_size(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);

	int newsize = luaL_checknumber(L, 3);
	if (newsize < 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Inventory *inv = getinv(L, ref);
	if(inv == NULL){
		lua_pushboolean(L, false);
		return 1;
	}
	if(newsize == 0){
		inv->deleteList(listname);
		reportInventoryChange(L, ref);
		lua_pushboolean(L, true);
		return 1;
	}
	InventoryList *list = inv->getList(listname);
	if(list){
		list->setSize(newsize);
	} else {
		list = inv->addList(listname, newsize);
		if (!list)
		{
			lua_pushboolean(L, false);
			return 1;
		}
	}
	reportInventoryChange(L, ref);
	lua_pushboolean(L, true);
	return 1;
}

// set_width(self, listname, size)
int InvRef::l_set_width(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	int newwidth = luaL_checknumber(L, 3);
	Inventory *inv = getinv(L, ref);
	if(inv == NULL){
		return 0;
	}
	InventoryList *list = inv->getList(listname);
	if(list){
		list->setWidth(newwidth);
	} else {
		return 0;
	}
	reportInventoryChange(L, ref);
	return 0;
}

// get_stack(self, listname, i) -> itemstack
int InvRef::l_get_stack(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	int i = luaL_checknumber(L, 3) - 1;
	InventoryList *list = getlist(L, ref, listname);
	ItemStack item;
	if(list != NULL && i >= 0 && i < (int) list->getSize())
		item = list->getItem(i);
	LuaItemStack::create(L, item);
	return 1;
}

// set_stack(self, listname, i, stack) -> true/false
int InvRef::l_set_stack(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	int i = luaL_checknumber(L, 3) - 1;
	ItemStack newitem = read_item(L, 4, getServer(L));
	InventoryList *list = getlist(L, ref, listname);
	if(list != NULL && i >= 0 && i < (int) list->getSize()){
		list->changeItem(i, newitem);
		reportInventoryChange(L, ref);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

// get_list(self, listname) -> list or nil
int InvRef::l_get_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	Inventory *inv = getinv(L, ref);
	if(inv){
		push_inventory_list(L, inv, listname);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// set_list(self, listname, list)
int InvRef::l_set_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	Inventory *inv = getinv(L, ref);
	if(inv == NULL){
		return 0;
	}
	InventoryList *list = inv->getList(listname);
	if(list)
		read_inventory_list(L, 3, inv, listname,
				getServer(L), list->getSize());
	else
		read_inventory_list(L, 3, inv, listname, getServer(L));
	reportInventoryChange(L, ref);
	return 0;
}

// get_lists(self) -> list of InventoryLists
int InvRef::l_get_lists(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	Inventory *inv = getinv(L, ref);
	if (!inv) {
		return 0;
	}
	std::vector<const InventoryList*> lists = inv->getLists();
	std::vector<const InventoryList*>::iterator iter = lists.begin();
	lua_createtable(L, 0, lists.size());
	for (; iter != lists.end(); iter++) {
		const char* name = (*iter)->getName().c_str();
		lua_pushstring(L, name);
		push_inventory_list(L, inv, name);
		lua_rawset(L, -3);
	}
	return 1;
}

// set_lists(self, lists)
int InvRef::l_set_lists(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	Inventory *inv = getinv(L, ref);
	if (!inv) {
		return 0;
	}

	// Make a temporary inventory in case reading fails
	Inventory *tempInv(inv);
	tempInv->clear();

	Server *server = getServer(L);

	lua_pushnil(L);
	while (lua_next(L, 2)) {
		const char *listname = lua_tostring(L, -2);
		read_inventory_list(L, -1, tempInv, listname, server);
		lua_pop(L, 1);
	}
	inv = tempInv;
	return 0;
}

// add_item(self, listname, itemstack or itemstring or table or nil) -> itemstack
// Returns the leftover stack
int InvRef::l_add_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	ItemStack item = read_item(L, 3, getServer(L));
	InventoryList *list = getlist(L, ref, listname);
	if(list){
		ItemStack leftover = list->addItem(item);
		if(leftover.count != item.count)
			reportInventoryChange(L, ref);
		LuaItemStack::create(L, leftover);
	} else {
		LuaItemStack::create(L, item);
	}
	return 1;
}

// room_for_item(self, listname, itemstack or itemstring or table or nil) -> true/false
// Returns true if the item completely fits into the list
int InvRef::l_room_for_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	ItemStack item = read_item(L, 3, getServer(L));
	InventoryList *list = getlist(L, ref, listname);
	if(list){
		lua_pushboolean(L, list->roomForItem(item));
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

// contains_item(self, listname, itemstack or itemstring or table or nil) -> true/false
// Returns true if the list contains the given count of the given item name
int InvRef::l_contains_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	ItemStack item = read_item(L, 3, getServer(L));
	InventoryList *list = getlist(L, ref, listname);
	if(list){
		lua_pushboolean(L, list->containsItem(item));
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

// remove_item(self, listname, itemstack or itemstring or table or nil) -> itemstack
// Returns the items that were actually removed
int InvRef::l_remove_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const char *listname = luaL_checkstring(L, 2);
	ItemStack item = read_item(L, 3, getServer(L));
	InventoryList *list = getlist(L, ref, listname);
	if(list){
		ItemStack removed = list->removeItem(item);
		if(!removed.empty())
			reportInventoryChange(L, ref);
		LuaItemStack::create(L, removed);
	} else {
		LuaItemStack::create(L, ItemStack());
	}
	return 1;
}

// get_location() -> location (like get_inventory(location))
int InvRef::l_get_location(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *ref = checkobject(L, 1);
	const InventoryLocation &loc = ref->m_loc;
	switch(loc.type){
	case InventoryLocation::PLAYER:
		lua_newtable(L);
		lua_pushstring(L, "player");
		lua_setfield(L, -2, "type");
		lua_pushstring(L, loc.name.c_str());
		lua_setfield(L, -2, "name");
		return 1;
	case InventoryLocation::NODEMETA:
		lua_newtable(L);
		lua_pushstring(L, "node");
		lua_setfield(L, -2, "type");
		push_v3s16(L, loc.p);
		lua_setfield(L, -2, "pos");
		return 1;
	case InventoryLocation::DETACHED:
		lua_newtable(L);
		lua_pushstring(L, "detached");
		lua_setfield(L, -2, "type");
		lua_pushstring(L, loc.name.c_str());
		lua_setfield(L, -2, "name");
		return 1;
	case InventoryLocation::UNDEFINED:
	case InventoryLocation::CURRENT_PLAYER:
		break;
	}
	lua_newtable(L);
	lua_pushstring(L, "undefined");
	lua_setfield(L, -2, "type");
	return 1;
}


InvRef::InvRef(const InventoryLocation &loc):
	m_loc(loc)
{
}

InvRef::~InvRef()
{
}

// Creates an InvRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void InvRef::create(lua_State *L, const InventoryLocation &loc)
{
	NO_MAP_LOCK_REQUIRED;
	InvRef *o = new InvRef(loc);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}
void InvRef::createPlayer(lua_State *L, Player *player)
{
	NO_MAP_LOCK_REQUIRED;
	InventoryLocation loc;
	loc.setPlayer(player->getName());
	create(L, loc);
}
void InvRef::createNodeMeta(lua_State *L, v3s16 p)
{
	InventoryLocation loc;
	loc.setNodeMeta(p);
	create(L, loc);
}

void InvRef::Register(lua_State *L)
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

	// Cannot be created from Lua
	//lua_register(L, className, create_object);
}

const char InvRef::className[] = "InvRef";
const luaL_reg InvRef::methods[] = {
	luamethod(InvRef, is_empty),
	luamethod(InvRef, get_size),
	luamethod(InvRef, set_size),
	luamethod(InvRef, get_width),
	luamethod(InvRef, set_width),
	luamethod(InvRef, get_stack),
	luamethod(InvRef, set_stack),
	luamethod(InvRef, get_list),
	luamethod(InvRef, set_list),
	luamethod(InvRef, get_lists),
	luamethod(InvRef, set_lists),
	luamethod(InvRef, add_item),
	luamethod(InvRef, room_for_item),
	luamethod(InvRef, contains_item),
	luamethod(InvRef, remove_item),
	luamethod(InvRef, get_location),
	{0,0}
};

// get_inventory(location)
int ModApiInventory::l_get_inventory(lua_State *L)
{
	InventoryLocation loc;

	std::string type = checkstringfield(L, 1, "type");

	if(type == "node"){
		lua_getfield(L, 1, "pos");
		v3s16 pos = check_v3s16(L, -1);
		loc.setNodeMeta(pos);

		if(getServer(L)->getInventory(loc) != NULL)
			InvRef::create(L, loc);
		else
			lua_pushnil(L);
		return 1;
	} else {
		NO_MAP_LOCK_REQUIRED;
		if(type == "player"){
			std::string name = checkstringfield(L, 1, "name");
			loc.setPlayer(name);
		} else if(type == "detached"){
			std::string name = checkstringfield(L, 1, "name");
			loc.setDetached(name);
		}

		if(getServer(L)->getInventory(loc) != NULL)
			InvRef::create(L, loc);
		else
			lua_pushnil(L);
		return 1;	
		// END NO_MAP_LOCK_REQUIRED;
	}
}

// create_detached_inventory_raw(name)
int ModApiInventory::l_create_detached_inventory_raw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	const char *name = luaL_checkstring(L, 1);
	if(getServer(L)->createDetachedInventory(name) != NULL){
		InventoryLocation loc;
		loc.setDetached(name);
		InvRef::create(L, loc);
	}else{
		lua_pushnil(L);
	}
	return 1;
}

void ModApiInventory::Initialize(lua_State *L, int top)
{
	API_FCT(create_detached_inventory_raw);
	API_FCT(get_inventory);
}
