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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_metadata.h"
#include "lua_api/l_item.h"
#include "irrlichttypes_bloated.h"

class ItemStackMetaRef : public MetaDataRef
{
private:
	LuaItemStack *istack;

	static const luaL_Reg methods[];

	virtual IMetadata* getmeta(bool auto_create);

	virtual void clearMeta();

	virtual void reportMetadataChange(const std::string *name = nullptr);

	void setToolCapabilities(const ToolCapabilities &caps)
	{
		istack->getItem().metadata.setToolCapabilities(caps);
	}

	void clearToolCapabilities()
	{
		istack->getItem().metadata.clearToolCapabilities();
	}

	// Exported functions
	static int l_set_tool_capabilities(lua_State *L);
public:
	// takes a reference
	ItemStackMetaRef(LuaItemStack *istack);
	~ItemStackMetaRef();

	DISABLE_CLASS_COPY(ItemStackMetaRef)

	// Creates an ItemStackMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, LuaItemStack *istack);

	static void Register(lua_State *L);

	static const char className[];
};
