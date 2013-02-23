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

#ifndef LUA_PSEUDORANDOM_H_
#define LUA_PSEUDORANDOM_H_


extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include "noise.h"

/*
	LuaPseudoRandom
*/


class LuaPseudoRandom
{
private:
	PseudoRandom m_pseudo;

	static const char className[];
	static const luaL_reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// next(self, min=0, max=32767) -> get next value
	static int l_next(lua_State *L);

public:
	LuaPseudoRandom(int seed);

	~LuaPseudoRandom();

	const PseudoRandom& getItem() const;
	PseudoRandom& getItem();

	// LuaPseudoRandom(seed)
	// Creates an LuaPseudoRandom and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaPseudoRandom* checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};




#endif /* LUA_PSEUDORANDOM_H_ */
