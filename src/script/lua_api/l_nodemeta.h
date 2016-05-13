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
#ifndef L_NODEMETA_H_
#define L_NODEMETA_H_

#include "lua_api/l_base.h"
#include "irrlichttypes_bloated.h"

class ServerEnvironment;
class NodeMetadata;

/*
	NodeMetaRef
*/

class NodeMetaRef : public ModApiBase {
private:
	v3s16 m_p;
	ServerEnvironment *m_env;

	static const char className[];
	static const luaL_reg methods[];

	static NodeMetaRef *checkobject(lua_State *L, int narg);

	/**
	 * Retrieve metadata for a node.
	 * If @p auto_create is set and the specified node has no metadata information
	 * associated with it yet, the method attempts to attach a new metadata object
	 * to the node and returns a pointer to the metadata when successful.
	 *
	 * However, it is NOT guaranteed that the method will return a pointer,
	 * and @c NULL may be returned in case of an error regardless of @p auto_create.
	 *
	 * @param ref specifies the node for which the associated metadata is retrieved.
	 * @param auto_create when true, try to create metadata information for the node if it has none.
	 * @return pointer to a @c NodeMetadata object or @c NULL in case of error.
	 */
	static NodeMetadata* getmeta(NodeMetaRef *ref, bool auto_create);

	static void reportMetadataChange(NodeMetaRef *ref);

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

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

	// get_inventory(self)
	static int l_get_inventory(lua_State *L);

	// to_table(self)
	static int l_to_table(lua_State *L);

	// from_table(self, table)
	static int l_from_table(lua_State *L);

public:
	NodeMetaRef(v3s16 p, ServerEnvironment *env);

	~NodeMetaRef();

	// Creates an NodeMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, v3s16 p, ServerEnvironment *env);

	static void Register(lua_State *L);
};

#endif /* L_NODEMETA_H_ */
