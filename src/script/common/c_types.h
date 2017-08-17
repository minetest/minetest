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

extern "C" {
#include "lua.h"
}

#include <iostream>

#include "exceptions.h"

struct EnumString
{
	int num;
	const char *str;
};

class StackUnroller
{
private:
	lua_State *m_lua;
	int m_original_top;
public:
	StackUnroller(lua_State *L):
		m_lua(L),
		m_original_top(-1)
	{
		m_original_top = lua_gettop(m_lua); // store stack height
	}
	~StackUnroller()
	{
		lua_settop(m_lua, m_original_top); // restore stack height
	}
};

class LuaError : public ModError
{
public:
	LuaError(const std::string &s) : ModError(s) {}
};


extern EnumString es_ItemType[];
