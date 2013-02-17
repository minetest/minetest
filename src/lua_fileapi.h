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
#include <vector>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "sqlite3.h"
}

#include "server.h"
#include "settings.h"

class FileRef {
public:
	FileRef();
	~FileRef();

	typedef enum {
		FR_Plain,
		FR_Settings,
		FR_SQLite,
		FR_Invalid
	} FileType;

	typedef enum {
		FR_World,
		FR_User,
		FR_Unknown
	} StorageLoc;

	//file information
	bool          m_writable;
	FileType      m_type;
	StorageLoc    m_storage;

	//static functions
	static FileRef*     checkobject         (lua_State *L, int narg);
	static int          gc_object           (lua_State *L);
	static bool         checkFilename       (std::string filename,StorageLoc type);
	static void         Register            (lua_State *L);
	static void         create_lua_object   (lua_State *L,FileRef* ref);

	//lua functions
	static int l_listfiles                  (lua_State *L);
	static int l_open                       (lua_State *L);
	static int l_delete                     (lua_State *L);
	static int l_close                      (lua_State *L);
	static int l_save                       (lua_State *L);
	static int l_getline                    (lua_State *L);
	static int l_write                      (lua_State *L);
	static int l_read                       (lua_State *L);
	static int l_seekline                   (lua_State *L);

	static int l_setting_set                (lua_State *L);
	static int l_setting_setbool            (lua_State *L);
	static int l_setting_get                (lua_State *L);
	static int l_setting_getbool            (lua_State *L);

	static const char className[];
	static const luaL_reg methods[];

private:
	bool open                               (std::string mode);
	bool open_settings                      ();
	bool open_settings_sqlite               (std::string mode);
	void cleanup_sqlite                     ();
	bool validateFilePermissinons           (std::string name,
												std::string type,
												std::string location);
	bool getFilename                        (std::string filename, lua_State *L);
	bool sql_db_initialize                  ();

	std::fstream* m_file;
	Settings*     m_settings;
	std::string   m_filename;
	sqlite3*      m_database;
	std::vector<unsigned int>   m_linepos;

	sqlite3_stmt* m_stmt_create_table_int;
	sqlite3_stmt* m_stmt_create_table_string;
	sqlite3_stmt* m_stmt_set_string;
	sqlite3_stmt* m_stmt_set_bool;
	sqlite3_stmt* m_stmt_get_string;
	sqlite3_stmt* m_stmt_get_bool;
	sqlite3_stmt* m_stmt_del_string;
	sqlite3_stmt* m_stmt_del_bool;
};
#endif /* LUA_FILEAPI_H_ */
