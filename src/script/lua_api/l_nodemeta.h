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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_metadata.h"
#include "irrlichttypes_bloated.h"
#include "nodemetadata.h"

class ServerEnvironment;
class NodeMetadata;

/*
	NodeMetaRef
*/

class NodeMetaRef : public MetaDataRef {
private:
	v3s16 m_p;
	ServerEnvironment *m_env = nullptr;
	Metadata *m_meta = nullptr;
	bool m_is_local = false;

	static const char className[];
	static const luaL_Reg methodsServer[];
	static const luaL_Reg methodsClient[];

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
	virtual Metadata* getmeta(bool auto_create);
	virtual void clearMeta();

	virtual void reportMetadataChange(const std::string *name = nullptr);

	virtual void handleToTable(lua_State *L, Metadata *_meta);
	virtual bool handleFromTable(lua_State *L, int table, Metadata *_meta);

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// get_inventory(self)
	static int l_get_inventory(lua_State *L);

	// mark_as_private(self, <string> or {<string>, <string>, ...})
	static int l_mark_as_private(lua_State *L);

public:
	NodeMetaRef(v3s16 p, ServerEnvironment *env);
	NodeMetaRef(Metadata *meta);

	~NodeMetaRef() = default;

	// Creates an NodeMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, v3s16 p, ServerEnvironment *env);

	// Client-sided version of the above
	static void createClient(lua_State *L, Metadata *meta);

	static void RegisterCommon(lua_State *L);
	static void Register(lua_State *L);
	static void RegisterClient(lua_State *L);
};
