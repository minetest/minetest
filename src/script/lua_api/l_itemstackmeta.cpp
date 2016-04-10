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

#include "lua_api/l_itemstackmeta.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_inventory.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "gamedef.h"
#include "inventory.h"
#include "itemstackmetadata.h"



/*
	ItemStackMetaRef
*/
ItemStackMetaRef* ItemStackMetaRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(ItemStackMetaRef**)ud;  // unbox pointer
}

ItemStackMetadata* ItemStackMetaRef::getmeta(ItemStackMetaRef *ref)
{
	ItemStackMetadata *meta = &(ref->m_istack->metadata);
	return meta;
}


void ItemStackMetaRef::reportMetadataChange(ItemStackMetaRef *ref)
{
	//At the moment, it does not seem to be neccessary to notify anything about changed metadata.
	//Reserved for future use.
}


// Exported functions

// garbage collector
int ItemStackMetaRef::gc_object(lua_State *L) {
	ItemStackMetaRef *o = *(ItemStackMetaRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// get_string(self, name)
int ItemStackMetaRef::l_get_string(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	ItemStackMetadata *meta = getmeta(ref);
	if(meta == NULL){
		lua_pushlstring(L, "", 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

// set_string(self, name, var)
int ItemStackMetaRef::l_set_string(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);
	size_t len = 0;
	const char *s = lua_tolstring(L, 3, &len);
	std::string str(s, len);

	ItemStackMetadata *meta = getmeta(ref);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_int(self, name)
int ItemStackMetaRef::l_get_int(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);

	ItemStackMetadata *meta = getmeta(ref);
	if(meta == NULL){
		lua_pushnumber(L, 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushnumber(L, stoi(str));
	return 1;
}

// set_int(self, name, var)
int ItemStackMetaRef::l_set_int(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);
	int a = lua_tointeger(L, 3);
	std::string str = itos(a);

	ItemStackMetadata *meta = getmeta(ref);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_float(self, name)
int ItemStackMetaRef::l_get_float(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);

	ItemStackMetadata *meta = getmeta(ref);
	if(meta == NULL){
		lua_pushnumber(L, 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushnumber(L, stof(str));
	return 1;
}

// set_float(self, name, var)
int ItemStackMetaRef::l_set_float(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);
	float a = lua_tonumber(L, 3);
	std::string str = ftos(a);

	ItemStackMetadata *meta = getmeta(ref);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// to_table(self)
int ItemStackMetaRef::l_to_table(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);

	ItemStackMetadata *meta = getmeta(ref);
	if (meta == NULL) {
		lua_pushnil(L);
		return 1;
	}
	lua_newtable(L);

	{
		StringMap fields = meta->getStrings();
		for (StringMap::const_iterator
				it = fields.begin(); it != fields.end(); ++it) {
			const std::string &name = it->first;
			const std::string &value = it->second;
			lua_pushlstring(L, name.c_str(), name.size());
			lua_pushlstring(L, value.c_str(), value.size());
			lua_settable(L, -3);
		}
	}

	return 1;
}

// from_table(self, table)
int ItemStackMetaRef::l_from_table(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	ItemStackMetaRef *ref = checkobject(L, 1);
	int base = 2;

	ItemStackMetadata *meta = getmeta(ref);

	if(meta == NULL){
		lua_pushboolean(L, false);
		return 1;
	}

	meta->clear();

	if(lua_isnil(L, base)){
		// No metadata
		lua_pushboolean(L, true);
		return 1;
	}

	// Set fields
	int fieldstable = lua_gettop(L);
	lua_pushnil(L);
	while(lua_next(L, fieldstable) != 0){
		// key at index -2 and value at index -1
		std::string name = lua_tostring(L, -2);
		size_t cl;
		const char *cs = lua_tolstring(L, -1, &cl);
		std::string value(cs, cl);
		meta->setString(name, value);
		lua_pop(L, 1); // removes value, keeps key for next iteration
	}

	reportMetadataChange(ref);
	lua_pushboolean(L, true);
	return 1;
}


ItemStackMetaRef::ItemStackMetaRef(ItemStack *istack):
	m_istack(istack)
{
}

ItemStackMetaRef::~ItemStackMetaRef()
{
}

// Creates an ItemStackMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void ItemStackMetaRef::create(lua_State *L, ItemStack *istack)
{
	ItemStackMetaRef *o = new ItemStackMetaRef(istack);
	//infostream<<"ItemStackMetaRef::create: o="<<o<<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ItemStackMetaRef::Register(lua_State *L)
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

const char ItemStackMetaRef::className[] = "ItemStackMetaRef";
const luaL_reg ItemStackMetaRef::methods[] = {
	luamethod(ItemStackMetaRef, get_string),
	luamethod(ItemStackMetaRef, set_string),
	luamethod(ItemStackMetaRef, get_int),
	luamethod(ItemStackMetaRef, set_int),
	luamethod(ItemStackMetaRef, get_float),
	luamethod(ItemStackMetaRef, set_float),
	luamethod(ItemStackMetaRef, to_table),
	luamethod(ItemStackMetaRef, from_table),
	{0,0}
};
