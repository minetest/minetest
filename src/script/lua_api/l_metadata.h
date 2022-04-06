/*
Minetest
Copyright (C) 2013-8 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#pragma once

#include "irrlichttypes_bloated.h"
#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class Metadata;

/*
	NodeMetaRef
*/

class MetaDataRef : public ModApiBase
{
public:
	virtual ~MetaDataRef() = default;

protected:
	static MetaDataRef *checkobject(lua_State *L, int narg);

	virtual void reportMetadataChange(const std::string *name = nullptr) {}
	virtual Metadata *getmeta(bool auto_create) = 0;
	virtual void clearMeta() = 0;

	virtual void handleToTable(lua_State *L, Metadata *meta);
	virtual bool handleFromTable(lua_State *L, int table, Metadata *meta);

	// Exported functions

	// contains(self, name)
	ENTRY_POINT_DECL(l_contains);

	// get(self, name)
	ENTRY_POINT_DECL(l_get);

	// get_string(self, name)
	ENTRY_POINT_DECL(l_get_string);

	// set_string(self, name, var)
	ENTRY_POINT_DECL(l_set_string);

	// get_int(self, name)
	ENTRY_POINT_DECL(l_get_int);

	// set_int(self, name, var)
	ENTRY_POINT_DECL(l_set_int);

	// get_float(self, name)
	ENTRY_POINT_DECL(l_get_float);

	// set_float(self, name, var)
	ENTRY_POINT_DECL(l_set_float);

	// to_table(self)
	ENTRY_POINT_DECL(l_to_table);

	// from_table(self, table)
	ENTRY_POINT_DECL(l_from_table);

	// equals(self, other)
	ENTRY_POINT_DECL(l_equals);
};
