/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "l_metadata.h"
#include "lua_api/l_base.h"
#include "l_internal.h"

class ModMetadata;

class ModApiStorage : public ModApiBase
{
protected:
	static int l_get_mod_storage(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};

class LuaModMetadata : public MetaDataRef
{
	LUAREF_OBJECT(ModMetadata);
private:
	virtual Metadata *getmeta(bool auto_create);
	virtual void clearMeta();
};
