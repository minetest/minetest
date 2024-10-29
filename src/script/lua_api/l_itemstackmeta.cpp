/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017-8 rubenwardy <rw@rubenwardy.com>
Copyright (C) 2017 raymoo

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
#include "common/c_content.h"
#include "common/c_converter.h"
#include "tool.h"

/*
	ItemStackMetaRef
*/

IMetadata* ItemStackMetaRef::getmeta(bool auto_create)
{
	return &istack->getItem().metadata;
}

void ItemStackMetaRef::clearMeta()
{
	istack->getItem().metadata.clear();
}

void ItemStackMetaRef::reportMetadataChange(const std::string *name)
{
	// nothing to do
}

// Exported functions
int ItemStackMetaRef::l_set_tool_capabilities(lua_State *L)
{
	ItemStackMetaRef *metaref = checkObject<ItemStackMetaRef>(L, 1);
	if (lua_isnoneornil(L, 2)) {
		metaref->clearToolCapabilities();
	} else if (lua_istable(L, 2)) {
		ToolCapabilities caps = read_tool_capabilities(L, 2);
		metaref->setToolCapabilities(caps);
	} else {
		luaL_typerror(L, 2, "table or nil");
	}

	return 0;
}

int ItemStackMetaRef::l_set_wear_bar_params(lua_State *L)
{
	ItemStackMetaRef *metaref = checkObject<ItemStackMetaRef>(L, 1);
	if (lua_isnoneornil(L, 2)) {
		metaref->clearWearBarParams();
	} else if (lua_istable(L, 2) || lua_isstring(L, 2)) {
		metaref->setWearBarParams(read_wear_bar_params(L, 2));
	} else {
		luaL_typerror(L, 2, "table, ColorString, or nil");
	}

	return 0;
}

ItemStackMetaRef::ItemStackMetaRef(LuaItemStack *istack): istack(istack)
{
	istack->grab();
}

ItemStackMetaRef::~ItemStackMetaRef()
{
	istack->drop();
}

// Creates an NodeMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void ItemStackMetaRef::create(lua_State *L, LuaItemStack *istack)
{
	ItemStackMetaRef *o = new ItemStackMetaRef(istack);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ItemStackMetaRef::Register(lua_State *L)
{
	registerMetadataClass(L, className, methods);
}

const char ItemStackMetaRef::className[] = "ItemStackMetaRef";
const luaL_Reg ItemStackMetaRef::methods[] = {
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
	luamethod(ItemStackMetaRef, set_tool_capabilities),
	luamethod(ItemStackMetaRef, set_wear_bar_params),
	{0,0}
};
