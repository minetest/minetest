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

/*
	PlayerMetaRef
*/

IMetadata *PlayerMetaRef::getmeta(bool auto_create)
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

// Creates an PlayerMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void PlayerMetaRef::create(lua_State *L, IMetadata *metadata)
{
	PlayerMetaRef *o = new PlayerMetaRef(metadata);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void PlayerMetaRef::Register(lua_State *L)
{
	registerMetadataClass(L, className, methods);

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
	luamethod(MetaDataRef, get_keys),
	luamethod(MetaDataRef, to_table),
	luamethod(MetaDataRef, from_table),
	luamethod(MetaDataRef, equals),
	{0,0}
};
// clang-format on
