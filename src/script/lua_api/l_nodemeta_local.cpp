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

#include "lua_api/l_nodemeta_local.h"
#include "inventory.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_inventory.h"
#include "common/c_content.h"
//#include "lua_api/l_inventory.h"
/*
	NodeMetaRef
*/
NodeMetaLocalRef* NodeMetaLocalRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(NodeMetaLocalRef**)ud;  // unbox pointer
}

Metadata* NodeMetaLocalRef::getmeta(bool auto_create)
{
	return m_meta;
}

void NodeMetaLocalRef::clearMeta()
{
}

void NodeMetaLocalRef::reportMetadataChange()
{
}

// Exported functions

// garbage collector
int NodeMetaLocalRef::gc_object(lua_State *L) {
	NodeMetaLocalRef *o = *(NodeMetaLocalRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

void NodeMetaLocalRef::handleToTable(lua_State *L, Metadata *_meta)
{
	// fields
	MetaDataRef::handleToTable(L, _meta);

	NodeMetadata *meta = (NodeMetadata*) _meta;

	// inventory
	lua_newtable(L);
	Inventory *inv = meta->getInventory();
	if (inv) {
		std::vector<const InventoryList *> lists = inv->getLists();
		for(std::vector<const InventoryList *>::const_iterator
				i = lists.begin(); i != lists.end(); i++) {
			push_inventory_list(L, inv, (*i)->getName().c_str());
			lua_setfield(L, -2, (*i)->getName().c_str());
		}
	}
	lua_setfield(L, -2, "inventory");
}

// from_table(self, table)
bool NodeMetaLocalRef::handleFromTable(lua_State *L, int table, Metadata *_meta)
{
	return false;
}


NodeMetaLocalRef::NodeMetaLocalRef(Metadata *meta):
	m_meta(meta)
{
}

NodeMetaLocalRef::~NodeMetaLocalRef()
{
}

// Creates an NodeMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void NodeMetaLocalRef::create(lua_State *L, Metadata *meta)
{
	NodeMetaLocalRef *o = new NodeMetaLocalRef(meta);
	//infostream<<"NodeMetaRef::create: o="<<o<<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void NodeMetaLocalRef::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "metadata_class");
	lua_pushlstring(L, className, strlen(className));
	lua_settable(L, metatable);

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

const char NodeMetaLocalRef::className[] = "NodeMetaLocalRef";
const luaL_reg NodeMetaLocalRef::methods[] = {
	luamethod(MetaDataRef, get_string),
	luamethod(MetaDataRef, get_int),
	luamethod(MetaDataRef, get_float),
	luamethod(MetaDataRef, to_table),
	{0,0}
};
