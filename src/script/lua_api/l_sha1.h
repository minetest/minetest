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

#ifndef L_SHA1_H_
#define L_SHA1_H_

#include "lua_api/l_base.h"
#include "sha1.h"

class LuaSHA1
{
private:
	static const char className[];
	static const luaL_reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// add_bytes(self, string)
	static int l_add_bytes(lua_State *L);

	// get_digest(self) -> string
	static int l_get_digest(lua_State *L);

	// get_hexdigest(self) -> string
	static int l_get_hexdigest(lua_State *L);
public:
	LuaSHA1();
	~LuaSHA1();

	// Creates an SHA1 Object
	static int create_object(lua_State *L);
	// Not callable from Lua
	static SHA1* checkobject(lua_State *L, int narg);
	static void Register(lua_State *L);

};

#endif /* L_SHA1_H_ */
