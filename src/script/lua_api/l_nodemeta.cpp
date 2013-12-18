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

#include "lua_api/l_nodemeta.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_inventory.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "environment.h"
#include "map.h"
#include "nodemetadata.h"



/*
	NodeMetaRef
*/
NodeMetaRef* NodeMetaRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(NodeMetaRef**)ud;  // unbox pointer
}

NodeMetadata* NodeMetaRef::getmeta(NodeMetaRef *ref, bool auto_create)
{
	NodeMetadata *meta = ref->m_env->getMap().getNodeMetadata(ref->m_p);
	if(meta == NULL && auto_create)	{
		meta = new NodeMetadata(ref->m_env->getGameDef());
		if(!ref->m_env->getMap().setNodeMetadata(ref->m_p, meta)) {
			delete meta;
			return NULL;
		}
	}
	return meta;
}

void NodeMetaRef::reportMetadataChange(NodeMetaRef *ref)
{
	// NOTE: This same code is in rollback_interface.cpp
	// Inform other things that the metadata has changed
	v3s16 blockpos = getNodeBlockPos(ref->m_p);
	MapEditEvent event;
	event.type = MEET_BLOCK_NODE_METADATA_CHANGED;
	event.p = blockpos;
	ref->m_env->getMap().dispatchEvent(&event);
	// Set the block to be saved
	MapBlock *block = ref->m_env->getMap().getBlockNoCreateNoEx(blockpos);
	if(block)
		block->raiseModified(MOD_STATE_WRITE_NEEDED,
				"NodeMetaRef::reportMetadataChange");
}

// Exported functions

// garbage collector
int NodeMetaRef::gc_object(lua_State *L) {
	NodeMetaRef *o = *(NodeMetaRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// get_string(self, name)
int NodeMetaRef::l_get_string(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	NodeMetadata *meta = getmeta(ref, false);
	if(meta == NULL){
		lua_pushlstring(L, "", 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

// set_string(self, name, var)
int NodeMetaRef::l_set_string(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);
	size_t len = 0;
	const char *s = lua_tolstring(L, 3, &len);
	std::string str(s, len);

	NodeMetadata *meta = getmeta(ref, !str.empty());
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_int(self, name)
int NodeMetaRef::l_get_int(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);

	NodeMetadata *meta = getmeta(ref, false);
	if(meta == NULL){
		lua_pushnumber(L, 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushnumber(L, stoi(str));
	return 1;
}

// set_int(self, name, var)
int NodeMetaRef::l_set_int(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);
	int a = lua_tointeger(L, 3);
	std::string str = itos(a);

	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_float(self, name)
int NodeMetaRef::l_get_float(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);

	NodeMetadata *meta = getmeta(ref, false);
	if(meta == NULL){
		lua_pushnumber(L, 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushnumber(L, stof(str));
	return 1;
}

// set_float(self, name, var)
int NodeMetaRef::l_set_float(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);
	float a = lua_tonumber(L, 3);
	std::string str = ftos(a);

	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_inventory(self)
int NodeMetaRef::l_get_inventory(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	getmeta(ref, true);  // try to ensure the metadata exists
	InvRef::createNodeMeta(L, ref->m_p);
	return 1;
}

// to_table(self)
int NodeMetaRef::l_to_table(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);

	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL){
		lua_pushnil(L);
		return 1;
	}
	lua_newtable(L);
	// fields
	lua_newtable(L);
	{
		std::map<std::string, std::string> fields = meta->getStrings();
		for(std::map<std::string, std::string>::const_iterator
				i = fields.begin(); i != fields.end(); i++){
			const std::string &name = i->first;
			const std::string &value = i->second;
			lua_pushlstring(L, name.c_str(), name.size());
			lua_pushlstring(L, value.c_str(), value.size());
			lua_settable(L, -3);
		}
	}
	lua_setfield(L, -2, "fields");
	// inventory
	lua_newtable(L);
	Inventory *inv = meta->getInventory();
	if(inv){
		std::vector<const InventoryList*> lists = inv->getLists();
		for(std::vector<const InventoryList*>::const_iterator
				i = lists.begin(); i != lists.end(); i++){
			push_inventory_list(L, inv, (*i)->getName().c_str());
			lua_setfield(L, -2, (*i)->getName().c_str());
		}
	}
	lua_setfield(L, -2, "inventory");
	return 1;
}

// from_table(self, table)
int NodeMetaRef::l_from_table(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	int base = 2;

	// clear old metadata first
	ref->m_env->getMap().removeNodeMetadata(ref->m_p);

	if(lua_isnil(L, base)){
		// No metadata
		lua_pushboolean(L, true);
		return 1;
	}

	// Create new metadata
	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL){
		lua_pushboolean(L, false);
		return 1;
	}
	// Set fields
	lua_getfield(L, base, "fields");
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
	// Set inventory
	Inventory *inv = meta->getInventory();
	lua_getfield(L, base, "inventory");
	int inventorytable = lua_gettop(L);
	lua_pushnil(L);
	while(lua_next(L, inventorytable) != 0){
		// key at index -2 and value at index -1
		std::string name = lua_tostring(L, -2);
		read_inventory_list(L, -1, inv, name.c_str(), getServer(L));
		lua_pop(L, 1); // removes value, keeps key for next iteration
	}
	reportMetadataChange(ref);
	lua_pushboolean(L, true);
	return 1;
}


NodeMetaRef::NodeMetaRef(v3s16 p, ServerEnvironment *env):
	m_p(p),
	m_env(env)
{
}

NodeMetaRef::~NodeMetaRef()
{
}

// Creates an NodeMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void NodeMetaRef::create(lua_State *L, v3s16 p, ServerEnvironment *env)
{
	NodeMetaRef *o = new NodeMetaRef(p, env);
	//infostream<<"NodeMetaRef::create: o="<<o<<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void NodeMetaRef::Register(lua_State *L)
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

const char NodeMetaRef::className[] = "NodeMetaRef";
const luaL_reg NodeMetaRef::methods[] = {
	luamethod(NodeMetaRef, get_string),
	luamethod(NodeMetaRef, set_string),
	luamethod(NodeMetaRef, get_int),
	luamethod(NodeMetaRef, set_int),
	luamethod(NodeMetaRef, get_float),
	luamethod(NodeMetaRef, set_float),
	luamethod(NodeMetaRef, get_inventory),
	luamethod(NodeMetaRef, to_table),
	luamethod(NodeMetaRef, from_table),
	{0,0}
};
