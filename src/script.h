/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SCRIPT_HEADER
#define SCRIPT_HEADER

#include <exception>
#include <string>

typedef struct lua_State lua_State;

class LuaError : public std::exception
{
public:
	LuaError(lua_State *L, const std::string &s);

	virtual ~LuaError() throw()
	{}
	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
	std::string m_s;
};

lua_State* script_init();
void script_deinit(lua_State *L);
std::string script_get_backtrace(lua_State *L);
void script_error(lua_State *L, const char *fmt, ...);
bool script_load(lua_State *L, const char *path);

#endif

