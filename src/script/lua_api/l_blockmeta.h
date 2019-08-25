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

class ServerEnvironment;

/*
	BlockMetaRef
*/

class BlockMetaRef : public MetaDataRef {
private:
	v3s16 m_bp;
	ServerEnvironment *m_env = nullptr;
	Metadata *m_meta = nullptr;
	bool m_is_local = false;

	static const char className[];
	static const luaL_Reg methodsServer[];
	static const luaL_Reg methodsClient[];

	static BlockMetaRef *checkobject(lua_State *L, int narg);

	/**
	 * Retrieve metadata for a block.
	 * If @p auto_create is set and the specified block has no metadata information
	 * associated with it yet, the method attempts to attach a new metadata object
	 * to the blck and returns a pointer to the metadata when successful.
	 *
	 * However, it is NOT guaranteed that the method will return a pointer,
	 * and @c nullptr may be returned in case of an error regardless of @p auto_create.
	 *
	 * @param auto_create when true, try to create metadata information for the block if it has none.
	 * @return pointer to a @c NodeMetadata object or @c nullptr in case of error.
	 */
	virtual Metadata *getmeta(bool auto_create);
	virtual void clearMeta();

	virtual void reportMetadataChange(const std::string *name = nullptr);

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// mark_as_private(self, <string> or {<string>, <string>, ...})
	static int l_mark_as_private(lua_State *L);

public:
	BlockMetaRef(v3s16 bp, ServerEnvironment *env);
	BlockMetaRef(Metadata *meta);

	~BlockMetaRef() = default;

	// Creates a BlockMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, v3s16 bp, ServerEnvironment *env);

	// Client-sided version of the above
	static void createClient(lua_State *L, Metadata *meta);

	static void RegisterCommon(lua_State *L);
	static void Register(lua_State *L);
	static void RegisterClient(lua_State *L);
};
