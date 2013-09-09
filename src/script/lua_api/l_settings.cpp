/*
Minetest
Copyright (C) 2013 PilzAdam <pilzadam@minetest.net>

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

#include "lua_api/l_settings.h"
#include "lua_api/l_internal.h"
#include "settings.h"
#include "log.h"

// garbage collector
int LuaSettings::gc_object(lua_State* L)
{
	LuaSettings* o = *(LuaSettings **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// get(self, key) -> value
int LuaSettings::l_get(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	std::string key = std::string(luaL_checkstring(L, 2));
	if (o->m_settings->exists(key)) {
		std::string value = o->m_settings->get(key);
		lua_pushstring(L, value.c_str());
	} else {
		lua_pushnil(L);
	}

	return 1;
}

// get_bool(self, key) -> boolean
int LuaSettings::l_get_bool(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	std::string key = std::string(luaL_checkstring(L, 2));
	if (o->m_settings->exists(key)) {
		bool value = o->m_settings->getBool(key);
		lua_pushboolean(L, value);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

// set(self, key, value)
int LuaSettings::l_set(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	std::string key = std::string(luaL_checkstring(L, 2));
	const char* value = luaL_checkstring(L, 3);

	o->m_settings->set(key, value);

	return 1;
}

// remove(self, key) -> success
int LuaSettings::l_remove(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	std::string key = std::string(luaL_checkstring(L, 2));

	bool success = o->m_settings->remove(key);
	lua_pushboolean(L, success);

	return 1;
}

// get_names(self) -> {key1, ...}
int LuaSettings::l_get_names(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	std::vector<std::string> keys = o->m_settings->getNames();

	lua_newtable(L);
	for (unsigned int i=0; i < keys.size(); i++)
	{
		lua_pushstring(L, keys[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

// write(self) -> success
int LuaSettings::l_write(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	bool success = o->m_settings->updateConfigFile(o->m_filename.c_str());
	lua_pushboolean(L, success);

	return 1;
}

// to_table(self) -> {[key1]=value1,...}
int LuaSettings::l_to_table(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaSettings* o = checkobject(L, 1);

	std::vector<std::string> keys = o->m_settings->getNames();

	lua_newtable(L);
	for (unsigned int i=0; i < keys.size(); i++)
	{
		lua_pushstring(L, o->m_settings->get(keys[i]).c_str());
		lua_setfield(L, -2, keys[i].c_str());
	}

	return 1;
}

LuaSettings::LuaSettings(const char* filename)
{
	m_filename = std::string(filename);

	m_settings = new Settings();
	m_settings->readConfigFile(m_filename.c_str());
}

LuaSettings::~LuaSettings()
{
	delete m_settings;
}

void LuaSettings::Register(lua_State* L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_openlib(L, 0, methods, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable

	// Can be created from Lua (Settings(filename))
	lua_register(L, className, create_object);
}

// LuaSettings(filename)
// Creates an LuaSettings and leaves it on top of stack
int LuaSettings::create_object(lua_State* L)
{
	NO_MAP_LOCK_REQUIRED;
	const char* filename = luaL_checkstring(L, 1);
	LuaSettings* o = new LuaSettings(filename);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

LuaSettings* LuaSettings::checkobject(lua_State* L, int narg)
{
	NO_MAP_LOCK_REQUIRED;
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(LuaSettings**)ud;  // unbox pointer
}

const char LuaSettings::className[] = "Settings";
const luaL_reg LuaSettings::methods[] = {
	luamethod(LuaSettings, get),
	luamethod(LuaSettings, get_bool),
	luamethod(LuaSettings, set),
	luamethod(LuaSettings, remove),
	luamethod(LuaSettings, get_names),
	luamethod(LuaSettings, write),
	luamethod(LuaSettings, to_table),
	{0,0}
};
