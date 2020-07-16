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

#include "lua_api/l_metadata.h"
#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "serverenvironment.h"
#include "map.h"
#include "server.h"

// LUALIB_API
void *luaL_checkudata_is_metadataref(lua_State *L, int ud) {
	void *p = lua_touserdata(L, ud);
	if (p != NULL &&  // value is a userdata?
			lua_getmetatable(L, ud)) {  // does it have a metatable?
		lua_getfield(L, -1, "metadata_class");
		if (lua_type(L, -1) == LUA_TSTRING) { // does it have a metadata_class field?
			return p;
		}
	}
	luaL_typerror(L, ud, "MetaDataRef");
	return NULL;
}

MetaDataRef* MetaDataRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata_is_metadataref(L, narg);
	if (!ud)
		luaL_typerror(L, narg, "MetaDataRef");

	return *(MetaDataRef**)ud;  // unbox pointer
}

// Exported functions

// contains(self, name)
int MetaDataRef::l_contains(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	Metadata *meta = ref->getmeta(false);
	if (meta == NULL)
		return 0;

	lua_pushboolean(L, meta->contains(name));
	return 1;
}

// get(self, name)
int MetaDataRef::l_get(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	Metadata *meta = ref->getmeta(false);
	if (meta == NULL)
		return 0;

	std::string str;
	if (meta->getStringToRef(name, str)) {
		lua_pushlstring(L, str.c_str(), str.size());
		return 1;
	}
	return 0;
}

// get_string(self, name)
int MetaDataRef::l_get_string(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	Metadata *meta = ref->getmeta(false);
	if (meta == NULL) {
		lua_pushlstring(L, "", 0);
		return 1;
	}

	const std::string &str = meta->getString(name);
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

// set_string(self, name, var)
int MetaDataRef::l_set_string(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);
	size_t len = 0;
	const char *s = lua_tolstring(L, 3, &len);
	std::string str(s, len);

	Metadata *meta = ref->getmeta(!str.empty());
	if (meta == NULL || str == meta->getString(name))
		return 0;

	meta->setString(name, str);
	ref->reportMetadataChange(&name);
	return 0;
}

// get_int(self, name)
int MetaDataRef::l_get_int(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	Metadata *meta = ref->getmeta(false);
	if (meta == NULL) {
		lua_pushnumber(L, 0);
		return 1;
	}

	const std::string &str = meta->getString(name);
	lua_pushnumber(L, stoi(str));
	return 1;
}

// set_int(self, name, var)
int MetaDataRef::l_set_int(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);
	int a = luaL_checkint(L, 3);
	std::string str = itos(a);

	Metadata *meta = ref->getmeta(true);
	if (meta == NULL || str == meta->getString(name))
		return 0;

	meta->setString(name, str);
	ref->reportMetadataChange(&name);
	return 0;
}

// get_float(self, name)
int MetaDataRef::l_get_float(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	Metadata *meta = ref->getmeta(false);
	if (meta == NULL) {
		lua_pushnumber(L, 0);
		return 1;
	}

	const std::string &str = meta->getString(name);
	lua_pushnumber(L, stof(str));
	return 1;
}

// set_float(self, name, var)
int MetaDataRef::l_set_float(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);
	float a = readParam<float>(L, 3);
	std::string str = ftos(a);

	Metadata *meta = ref->getmeta(true);
	if (meta == NULL || str == meta->getString(name))
		return 0;

	meta->setString(name, str);
	ref->reportMetadataChange(&name);
	return 0;
}

// to_table(self)
int MetaDataRef::l_to_table(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);

	Metadata *meta = ref->getmeta(true);
	if (meta == NULL) {
		lua_pushnil(L);
		return 1;
	}
	lua_newtable(L);

	ref->handleToTable(L, meta);

	return 1;
}

// from_table(self, table)
int MetaDataRef::l_from_table(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	MetaDataRef *ref = checkobject(L, 1);
	int base = 2;

	ref->clearMeta();

	if (!lua_istable(L, base)) {
		// No metadata
		lua_pushboolean(L, true);
		return 1;
	}

	// Create new metadata
	Metadata *meta = ref->getmeta(true);
	if (meta == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}

	bool was_successful = ref->handleFromTable(L, base, meta);
	ref->reportMetadataChange();
	lua_pushboolean(L, was_successful);
	return 1;
}

void MetaDataRef::handleToTable(lua_State *L, Metadata *meta)
{
	lua_newtable(L);
	{
		const StringMap &fields = meta->getStrings();
		for (const auto &field : fields) {
			const std::string &name = field.first;
			const std::string &value = field.second;
			lua_pushlstring(L, name.c_str(), name.size());
			lua_pushlstring(L, value.c_str(), value.size());
			lua_settable(L, -3);
		}
	}
	lua_setfield(L, -2, "fields");
}

bool MetaDataRef::handleFromTable(lua_State *L, int table, Metadata *meta)
{
	// Set fields
	lua_getfield(L, table, "fields");
	if (lua_istable(L, -1)) {
		int fieldstable = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, fieldstable) != 0) {
			// key at index -2 and value at index -1
			std::string name = readParam<std::string>(L, -2);
			size_t cl;
			const char *cs = lua_tolstring(L, -1, &cl);
			meta->setString(name, std::string(cs, cl));
			lua_pop(L, 1); // Remove value, keep key for next iteration
		}
		lua_pop(L, 1);
	}

	return true;
}

// equals(self, other)
int MetaDataRef::l_equals(lua_State *L)
{
	MetaDataRef *ref1 = checkobject(L, 1);
	Metadata *data1 = ref1->getmeta(false);
	MetaDataRef *ref2 = checkobject(L, 2);
	Metadata *data2 = ref2->getmeta(false);
	if (data1 == NULL || data2 == NULL)
		lua_pushboolean(L, data1 == data2);
	else
		lua_pushboolean(L, *data1 == *data2);
	return 1;
}
