/*
Minetest
Copyright (C) 2013 sfan5 <sfan5@live.de>

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

#include "lua_api/l_internal.h"
#include "lua_api/l_sha1.h"
#include <stdio.h>

// garbage collector
int LuaSHA1::gc_object(lua_State *L)
{
	SHA1 *o = (SHA1*) (lua_touserdata(L, 1));
	delete o;
	return 0;
}

// add_bytes(self, string)
int LuaSHA1::l_add_bytes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SHA1 *o = checkobject(L, 1);
	o->addBytes(luaL_checkstring(L, 2), lua_objlen(L, 2));
	return 0;
}

// get_digest(self) -> string
int LuaSHA1::l_get_digest(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SHA1 *o = checkobject(L, 1);
	char *dg = (char*) o->getDigest();
	lua_pushlstring(L, dg, 20);
	free(dg);
	return 1;
}

// get_hexdigest(self) -> string
int LuaSHA1::l_get_hexdigest(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	SHA1 *o = checkobject(L, 1);
	unsigned char *dg = o->getDigest();
	char buf[41];
	buf[40] = 0;
	int i;
	for(i = 0; i < 20; i++)
	{
		snprintf((char*) (buf + i*2), 3, "%02x", dg[i]);
	}
	lua_pushlstring(L, buf, 40);
	free(dg);
	return 1;
}

int LuaSHA1::create_object(lua_State *L) {
	NO_MAP_LOCK_REQUIRED;
	SHA1 *o = new SHA1();
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}


SHA1* LuaSHA1::checkobject(lua_State *L, int narg)
{
	NO_MAP_LOCK_REQUIRED;
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(SHA1**)ud;  // unbox pointer
}

void LuaSHA1::Register(lua_State *L)
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

	// Can be created from Lua (LuaSHA1())
	lua_register(L, className, create_object);
}

const char LuaSHA1::className[] = "SHA1";
const luaL_reg LuaSHA1::methods[] = {
	luamethod(LuaSHA1, add_bytes),
	luamethod(LuaSHA1, get_digest),
	luamethod(LuaSHA1, get_hexdigest),
	{0,0}
};

