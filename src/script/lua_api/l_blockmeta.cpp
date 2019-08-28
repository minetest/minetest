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

#include "lua_api/l_blockmeta.h"
#include "lua_api/l_internal.h"
#include "serverenvironment.h"
#include "map.h"
#include "server.h"
#include "blockmetadata.h"

/*
	BlockMetaRef
*/
BlockMetaRef *BlockMetaRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud) luaL_typerror(L, narg, className);
	return *(BlockMetaRef **)ud;  // unbox pointer
}

Metadata *BlockMetaRef::getmeta(bool auto_create)
{
	if (m_is_local)
		return m_meta;

	BlockMetadata *meta = m_env->getMap().getBlockMetadata(m_bp);
	if (!meta && auto_create) {
		meta = new BlockMetadata();
		if (!m_env->getMap().setBlockMetadata(m_bp, meta)) {
			delete meta;
			return nullptr;
		}
	}
	return meta;
}

void BlockMetaRef::clearMeta()
{
	m_env->getMap().removeBlockMetadata(m_bp);
}

void BlockMetaRef::reportMetadataChange(const std::string *name)
{
	// Inform other things that the metadata has changed
	BlockMetadata *meta = dynamic_cast<BlockMetadata*>(m_meta);

	MapEditEvent event;
	event.type = MEET_BLOCK_METADATA_CHANGED;
	event.p = m_bp;
	event.is_private_change = name && meta && meta->isPrivate(*name);
	m_env->getMap().dispatchEvent(&event);
}

// Exported functions

// garbage collector
int BlockMetaRef::gc_object(lua_State *L)
{
	BlockMetaRef *o = *(BlockMetaRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// mark_as_private(self, <string> or {<string>, <string>, ...})
int BlockMetaRef::l_mark_as_private(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	BlockMetaRef *ref = checkobject(L, 1);
	BlockMetadata *meta = dynamic_cast<BlockMetadata*>(ref->getmeta(true));
	assert(meta);

	if (lua_istable(L, 2)) {
		lua_pushnil(L);
		while (lua_next(L, 2) != 0) {
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TSTRING);
			meta->markPrivate(readParam<std::string>(L, -1), true);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	} else if (lua_isstring(L, 2)) {
		meta->markPrivate(readParam<std::string>(L, 2), true);
	}
	ref->reportMetadataChange();

	return 0;
}


BlockMetaRef::BlockMetaRef(v3s16 bp, ServerEnvironment *env):
	m_bp(bp),
	m_env(env)
{
}

BlockMetaRef::BlockMetaRef(Metadata *meta):
	m_meta(meta),
	m_is_local(true)
{
}

// Creates a BlockMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void BlockMetaRef::create(lua_State *L, v3s16 bp, ServerEnvironment *env)
{
	BlockMetaRef *o = new BlockMetaRef(bp, env);
	//infostream << "BlockMetaRef::create: o=" << o <<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

// Client-sided version of the above
void BlockMetaRef::createClient(lua_State *L, Metadata *meta)
{
	BlockMetaRef *o = new BlockMetaRef(meta);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

const char BlockMetaRef::className[] = "BlockMetaRef";
void BlockMetaRef::RegisterCommon(lua_State *L)
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

	lua_pushliteral(L, "__eq");
	lua_pushcfunction(L, l_equals);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable
}

void BlockMetaRef::Register(lua_State *L)
{
	RegisterCommon(L);
	luaL_openlib(L, 0, methodsServer, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable
}


const luaL_Reg BlockMetaRef::methodsServer[] = {
	luamethod(MetaDataRef, contains),
	luamethod(MetaDataRef, get),
	luamethod(MetaDataRef, get_string),
	luamethod(MetaDataRef, set_string),
	luamethod(MetaDataRef, get_int),
	luamethod(MetaDataRef, set_int),
	luamethod(MetaDataRef, get_float),
	luamethod(MetaDataRef, set_float),
	luamethod(MetaDataRef, to_table),
	luamethod(MetaDataRef, from_table),
	luamethod(BlockMetaRef, mark_as_private),
	luamethod(MetaDataRef, equals),
	{0,0}
};


void BlockMetaRef::RegisterClient(lua_State *L)
{
	RegisterCommon(L);
	luaL_openlib(L, 0, methodsClient, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable
}


const luaL_Reg BlockMetaRef::methodsClient[] = {
	luamethod(MetaDataRef, contains),
	luamethod(MetaDataRef, get),
	luamethod(MetaDataRef, get_string),
	luamethod(MetaDataRef, get_int),
	luamethod(MetaDataRef, get_float),
	luamethod(MetaDataRef, to_table),
	{0,0}
};
