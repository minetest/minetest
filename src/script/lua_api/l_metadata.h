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
#ifndef L_METADATA_H_
#define L_METADATA_H_

#include "irrlichttypes_bloated.h"
#include "lua_api/l_base.h"

class Metadata;

/*
	NodeMetaRef
*/

class MetaDataRef : public ModApiBase
{
public:
	virtual ~MetaDataRef() {}
protected:
	static MetaDataRef *checkobject(lua_State *L, int narg);

	virtual void reportMetadataChange() {}
	virtual Metadata *getmeta(bool auto_create) = 0;
	virtual void clearMeta() = 0;

	virtual void handleToTable(lua_State *L, Metadata *meta);
	virtual bool handleFromTable(lua_State *L, int table, Metadata *meta);

	// Exported functions

	// get_string(self, name)
	static int l_get_string(lua_State *L);

	// set_string(self, name, var)
	static int l_set_string(lua_State *L);

	// get_int(self, name)
	static int l_get_int(lua_State *L);

	// set_int(self, name, var)
	static int l_set_int(lua_State *L);

	// get_float(self, name)
	static int l_get_float(lua_State *L);

	// set_float(self, name, var)
	static int l_set_float(lua_State *L);

	// to_table(self)
	static int l_to_table(lua_State *L);

	// from_table(self, table)
	static int l_from_table(lua_State *L);

	// equals(self, other)
	static int l_equals(lua_State *L);
};

#endif /* L_NODEMETA_H_ */
