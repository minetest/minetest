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

#ifndef LUA_FILEAPI_H_
#define LUA_FILEAPI_H_

#include <iostream>
#include <fstream>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "server.h"

class FileRef {
public:
	FileRef();
	~FileRef();



	static FileRef *checkobject(lua_State *L, int narg);
	// garbage collector
	static int gc_object(lua_State *L);
	// Creates an FileRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	bool open(std::string path, std::string mode);
	static bool checkFilename(std::string filename,std::string type);
	static std::string getFilename(std::string filename,std::string type,lua_State *L);
	static void Register(lua_State *L);

	std::fstream* m_file;
	bool          m_writable;

	//lua functions
	static int l_listfiles(lua_State *L);
	static int l_open(lua_State *L);
	static int l_delete(lua_State *L);
	static int l_close(lua_State *L);
	static int l_getline(lua_State *L);
	static int l_write(lua_State *L);
	static int l_read(lua_State *L);
	static int l_seek(lua_State *L);

	static const char className[];
	static const luaL_reg methods[];
};

Server* get_server(lua_State *L);

#endif /* LUA_FILEAPI_H_ */
