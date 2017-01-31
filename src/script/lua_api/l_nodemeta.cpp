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
#include "common/c_content.h"
#include "serverenvironment.h"
#include "map.h"
#include "server.h"

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

Metadata* NodeMetaRef::getmeta(bool auto_create)
{
	NodeMetadata *meta = m_env->getMap().getNodeMetadata(m_p);
	if (meta == NULL && auto_create) {
		meta = new NodeMetadata(m_env->getGameDef()->idef());
		if (!m_env->getMap().setNodeMetadata(m_p, meta)) {
			delete meta;
			return NULL;
		}
	}
	return meta;
}

void NodeMetaRef::clearMeta()
{
	m_env->getMap().removeNodeMetadata(m_p);
}

void NodeMetaRef::reportMetadataChange()
{
	// NOTE: This same code is in rollback_interface.cpp
	// Inform other things that the metadata has changed
	v3s16 blockpos = getNodeBlockPos(m_p);
	MapEditEvent event;
	event.type = MEET_BLOCK_NODE_METADATA_CHANGED;
	event.p = blockpos;
	m_env->getMap().dispatchEvent(&event);
	// Set the block to be saved
	MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
	if (block) {
		block->raiseModified(MOD_STATE_WRITE_NEEDED,
			MOD_REASON_REPORT_META_CHANGE);
	}
}

// Exported functions

// garbage collector
int NodeMetaRef::gc_object(lua_State *L) {
	NodeMetaRef *o = *(NodeMetaRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// get_inventory(self)
int NodeMetaRef::l_get_inventory(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	NodeMetaRef *ref = checkobject(L, 1);
	ref->getmeta(true);  // try to ensure the metadata exists
	InvRef::createNodeMeta(L, ref->m_p);
	return 1;
}

void NodeMetaRef::handleToTable(lua_State *L, Metadata *_meta)
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
bool NodeMetaRef::handleFromTable(lua_State *L, int table, Metadata *_meta)
{
	// fields
	if (!MetaDataRef::handleFromTable(L, table, _meta))
		return false;

	NodeMetadata *meta = (NodeMetadata*) _meta;

	// inventory
	Inventory *inv = meta->getInventory();
	lua_getfield(L, table, "inventory");
	if (lua_istable(L, -1)) {
		int inventorytable = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, inventorytable) != 0) {
			// key at index -2 and value at index -1
			std::string name = lua_tostring(L, -2);
			read_inventory_list(L, -1, inv, name.c_str(), getServer(L));
			lua_pop(L, 1); // Remove value, keep key for next iteration
		}
		lua_pop(L, 1);
	}

	return true;
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

const char NodeMetaRef::className[] = "NodeMetaRef";
const luaL_reg NodeMetaRef::methods[] = {
	luamethod(MetaDataRef, get_string),
	luamethod(MetaDataRef, set_string),
	luamethod(MetaDataRef, get_int),
	luamethod(MetaDataRef, set_int),
	luamethod(MetaDataRef, get_float),
	luamethod(MetaDataRef, set_float),
	luamethod(MetaDataRef, to_table),
	luamethod(MetaDataRef, from_table),
	luamethod(NodeMetaRef, get_inventory),
	{0,0}
};
