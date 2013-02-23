/*
Minetest-c55
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

#include "lua_pseudorandom.h"

#include "log.h"
#include "script.h"

/*
	LuaPseudoRandom
*/

//TODO move this macro to a more generic place
#define method(class, name) {#name, class::l_##name}

// garbage collector
int LuaPseudoRandom::gc_object(lua_State *L)
{
	LuaPseudoRandom *o = *(LuaPseudoRandom **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// next(self, min=0, max=32767) -> get next value
int LuaPseudoRandom::l_next(lua_State *L)
{
	LuaPseudoRandom *o = checkobject(L, 1);
	int min = 0;
	int max = 32767;
	lua_settop(L, 3); // Fill 2 and 3 with nil if they don't exist
	if(!lua_isnil(L, 2))
		min = luaL_checkinteger(L, 2);
	if(!lua_isnil(L, 3))
		max = luaL_checkinteger(L, 3);
	if(max < min){
		errorstream<<"PseudoRandom.next(): max="<<max<<" min="<<min<<std::endl;
		throw LuaError(L, "PseudoRandom.next(): max < min");
	}
	if(max - min != 32767 && max - min > 32767/5)
		throw LuaError(L, "PseudoRandom.next() max-min is not 32767 and is > 32768/5. This is disallowed due to the bad random distribution the implementation would otherwise make.");
	PseudoRandom &pseudo = o->m_pseudo;
	int val = pseudo.next();
	val = (val % (max-min+1)) + min;
	lua_pushinteger(L, val);
	return 1;
}


LuaPseudoRandom::LuaPseudoRandom(int seed):
	m_pseudo(seed)
{
}

LuaPseudoRandom::~LuaPseudoRandom()
{
}

const PseudoRandom& LuaPseudoRandom::getItem() const
{
	return m_pseudo;
}
PseudoRandom& LuaPseudoRandom::getItem()
{
	return m_pseudo;
}

// LuaPseudoRandom(seed)
// Creates an LuaPseudoRandom and leaves it on top of stack
int LuaPseudoRandom::create_object(lua_State *L)
{
	int seed = luaL_checknumber(L, 1);
	LuaPseudoRandom *o = new LuaPseudoRandom(seed);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

LuaPseudoRandom* LuaPseudoRandom::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(LuaPseudoRandom**)ud;  // unbox pointer
}

void LuaPseudoRandom::Register(lua_State *L)
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

	// Can be created from Lua (LuaPseudoRandom(seed))
	lua_register(L, className, create_object);
}

const char LuaPseudoRandom::className[] = "PseudoRandom";
const luaL_reg LuaPseudoRandom::methods[] = {
	method(LuaPseudoRandom, next),
	{0,0}
};





