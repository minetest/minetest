/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017-8 rubenwardy <rw@rubenwardy.com>

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

#include "lua_api/l_playermeta.h"
#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "database/database.h"
#include "content_sao.h"
#include "remoteplayer.h"
#include "serverenvironment.h"

/*
	PlayerMetaRef
*/
PlayerMetaRef::PlayerMetaRef(Metadata *metadata, bool copy)
{
	if (copy)
		this->metadata = new Metadata(*metadata);
	else
		this->metadata = metadata;

	m_is_copy = copy;
}

PlayerMetaRef::~PlayerMetaRef()
{
	if (m_is_copy)
		delete metadata;
}

PlayerMetaRef *PlayerMetaRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);

	return *(PlayerMetaRef **)ud; // unbox pointer
}

Metadata *PlayerMetaRef::getmeta(bool auto_create)
{
	return metadata;
}

void PlayerMetaRef::clearMeta()
{
	metadata->clear();
}

void PlayerMetaRef::reportMetadataChange(const std::string *name)
{
	// TODO
}

// garbage collector
int PlayerMetaRef::gc_object(lua_State *L)
{
	PlayerMetaRef *o = *(PlayerMetaRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// Creates an PlayerMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void PlayerMetaRef::create(lua_State *L, Metadata *metadata, bool copy)
{
	PlayerMetaRef *o = new PlayerMetaRef(metadata, copy);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void PlayerMetaRef::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable); // hide metatable from Lua getmetatable()

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

	lua_pop(L, 1); // drop metatable

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	// Cannot be created from Lua
	// lua_register(L, className, create_object);
}

// clang-format off
const char PlayerMetaRef::className[] = "PlayerMetaRef";
const luaL_Reg PlayerMetaRef::methods[] = {
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
	luamethod(MetaDataRef, equals),
	{0,0}
};
// clang-format on

// get_player_meta(name)
int ModApiPlayerMeta::l_get_player_meta(lua_State *L)
{
	GET_ENV_PTR_NO_MAP_LOCK;

	const char *name = luaL_checkstring(L, 1);

	if (RemotePlayer *player = env->getPlayer(name)) {
		lua_pushboolean(L, true);
		PlayerMetaRef::create(L, &player->getPlayerSAO()->getMeta());
		return 2;
	}

	RemotePlayer player(name, nullptr);
	PlayerSAO sao(nullptr, &player, 15000, false);

	// Load everything from database but we don't care
	bool success = env->getPlayerDatabase()->loadPlayer(&player, &sao);

	lua_pushboolean(L, false);
	if (!success)
		return 1;

	PlayerMetaRef::create(L, &sao.getMeta(), true);
	return 2;
}


void ModApiPlayerMeta::Initialize(lua_State *L, int top)
{
	API_FCT(get_player_meta);
}
