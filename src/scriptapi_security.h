/*
Minetest-c55
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 sapier, sapier at gmx dot net

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

#ifndef SCRIPTAPI_SECURITY_HEADER
#define SCRIPTAPI_SECURITY_HEADER

//lua functions
int l_safe_dofile(lua_State *L);
int l_forbidden(lua_State *L);
int l_safe_remove(lua_State *L);
int l_safe_mkdir(lua_State *L);
int l_safe_io_lines(lua_State *L);
int l_safe_io_open(lua_State *L);

//this functions need to ne replaced with safe ones too
int l_safe_rawequal(lua_State *L);
int l_safe_rawget(lua_State *L);
int l_safe_rawset(lua_State *L);
int l_safe_fenv(lua_State *L);
int l_safe_setmetatable(lua_State *L);

//initialization
void InitSecurity(lua_State* L);

void replaceLibFunc(lua_State *L,
						std::string libname,
						std::string funcname,
						int (function)(lua_State*));

bool inAllowedFolder(lua_State *L,std::string filename);
bool isSpecialFile(std::string filename);

#endif
