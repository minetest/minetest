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


#include "lua_fileapi.h"
#include "porting.h"
#include "filesys.h"
#include "log.h"
#include "main.h"
#include <vector>

extern Server* get_server(lua_State *L);

#define method(class, name) {#name, class::l_##name}

const char FileRef::className[] = "FileRef";
const luaL_reg FileRef::methods[] = {
	method(FileRef, close),
	method(FileRef, save),
	method(FileRef, getline),
	method(FileRef, write),
	method(FileRef, read),
	method(FileRef, seekline),
	method(FileRef, setting_set),
	method(FileRef, setting_setbool),
	method(FileRef, setting_get),
	method(FileRef, setting_getbool),
	{0,0}
};

/******************************************************************************/
FileRef::FileRef() :
	m_writable(false),
	m_type(FR_Invalid),
	m_storage(FR_Unknown),
	m_file(0),
	m_settings(0),
	m_filename(""),
	m_database(0),
	m_stmt_create_table_int(0),
	m_stmt_create_table_string(0),
	m_stmt_set_string(0),
	m_stmt_set_bool(0),
	m_stmt_get_string(0),
	m_stmt_get_bool(0),
	m_stmt_del_string(0),
	m_stmt_del_bool(0)

{
}

/******************************************************************************/
FileRef::~FileRef() {
	if (m_file != 0)
		delete m_file;
	if (m_settings != 0)
		delete m_settings;
	if (m_database != 0)
		cleanup_sqlite();
}

/******************************************************************************/
void FileRef::cleanup_sqlite() {
	sqlite3_finalize(m_stmt_set_string);
	sqlite3_finalize(m_stmt_set_bool);
	sqlite3_finalize(m_stmt_get_string);
	sqlite3_finalize(m_stmt_get_bool);
	sqlite3_finalize(m_stmt_del_string);
	sqlite3_finalize(m_stmt_del_bool);
	m_stmt_set_string = 0;
	m_stmt_set_bool = 0;
	m_stmt_get_string = 0;
	m_stmt_get_bool = 0;
	m_stmt_del_string = 0;
	m_stmt_del_bool = 0;
	sqlite3_close(m_database);
	m_database = 0;
}

/******************************************************************************/
FileRef* FileRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, FileRef::className);
	if(!ud) luaL_typerror(L, narg, FileRef::className);
	return *(FileRef**)ud;  // unbox pointer
}


/******************************************************************************/
int FileRef::gc_object(lua_State *L) {
	FileRef *o = *(FileRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}


/******************************************************************************/
bool FileRef::open(std::string mode) {
	if (mode == "t") {
		this->m_file = new std::fstream(m_filename.c_str(),
				std::ios_base::trunc | std::ios::out | std::ios::in);
		this->m_writable = true;
	}
	else if (mode == "w") {
		this->m_file = new std::fstream(m_filename.c_str(),
				std::ios_base::ate | std::ios::out | std::ios::in);
		this->m_writable = true;
	}
	else {
		//open read only by default
		this->m_file = new std::fstream(m_filename.c_str(), std::ios_base::in);
		this->m_writable = false;
	}
	if (this->m_file != 0) {
		return true;
	}
	return false;
}

/******************************************************************************/
void FileRef::create_lua_object(lua_State *L,FileRef* ref) {
	*(void **)(lua_newuserdata(L,sizeof(void *))) = ref;
	luaL_getmetatable(L,className);
	lua_setmetatable(L,-2);
}

/******************************************************************************/
bool FileRef::open_settings() {
	this->m_settings = new Settings();

	return this->m_settings->readConfigFile(m_filename.c_str());
}

/******************************************************************************/

#define MY_SQLITE_ERROR if ( (sqlite_retval != SQLITE_OK) && \
							 (sqlite_retval != SQLITE_DONE)) { \
								errorstream << "Unable to do sqlite operation "\
									<< sqlite3_errmsg(m_database) << std::endl;\
										return false; \
						} else { \
							sqlite_retval = 0; \
						}

bool FileRef::sql_db_initialize() {
	int sqlite_retval = 0;

	//prepare basic statements
	sqlite_retval |= sqlite3_prepare_v2(m_database,
			"CREATE TABLE IF NOT EXISTS integers('key' TEXT NOT NULL, 'value' INTEGER);",
			-1,
			&m_stmt_create_table_int, 0);
	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"CREATE TABLE IF NOT EXISTS strings ('key' TEXT NOT NULL, 'value' TEXT);",
				-1,
				&m_stmt_create_table_string, 0);
	MY_SQLITE_ERROR

	//create db tables
	sqlite_retval |= sqlite3_step(m_stmt_create_table_int);
	sqlite3_finalize(m_stmt_create_table_int);
	m_stmt_create_table_int = 0;

	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_step(m_stmt_create_table_string);
	sqlite3_finalize(m_stmt_create_table_string);
	m_stmt_create_table_string = 0;
	MY_SQLITE_ERROR

	//prepare runtime statements
	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"INSERT INTO strings VALUES (?,?);",
				-1,
				&m_stmt_set_string, 0);
	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"INSERT INTO integers VALUES (?,?);",
				-1,
				&m_stmt_set_bool, 0);
	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"DELETE FROM strings WHERE key=?;",
				-1,
				&m_stmt_del_string, 0);
	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"DELETE FROM integers WHERE key=?;",
				-1,
				&m_stmt_del_bool, 0);
	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"SELECT value FROM strings WHERE key=?;",
				-1,
				&m_stmt_get_string, 0);
	MY_SQLITE_ERROR

	sqlite_retval |= sqlite3_prepare_v2(m_database,
				"SELECT value FROM integers WHERE key=?;",
				-1,
				&m_stmt_get_bool, 0);
	MY_SQLITE_ERROR

	return true;
}

/******************************************************************************/
bool FileRef::open_settings_sqlite(std::string mode) {

	unsigned int openmode = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	m_writable = true;

	if (mode == "t") {
		if (!fs::DeleteSingleFileOrEmptyDirectory(m_filename)) {
			return false;
		}
	}
	if (mode == "r") {
		openmode    = SQLITE_OPEN_READONLY;
		m_writable = false;
	}

	if (sqlite3_open_v2(m_filename.c_str(),
						&this->m_database,
						openmode,
						0) == SQLITE_OK) {
		if (this->sql_db_initialize()) {
			return true;
		}
	}
	return false;
}

/******************************************************************************/
bool FileRef::getFilename(std::string filename, lua_State *L) {
	std::string complete_path = "";
	if (m_storage == FR_World) {
		complete_path = get_server(L)->getWorldPath();
		complete_path += DIR_DELIM;
		complete_path += filename;
	} else if (m_storage == FR_User ) {
		complete_path = porting::path_user;
		complete_path += DIR_DELIM;
		complete_path += filename;
	}

	if (complete_path == "" ) {
		errorstream <<
				"Invalid file type specified on opening file" << std::endl;
		return false;
	}

	m_filename = complete_path;
	return true;
}
/******************************************************************************/
bool FileRef::checkFilename       (std::string filename,StorageLoc loc) {

	if (loc == FR_World) {
		if (filename == "auth.txt") return false;
		if (filename == "env_meta.txt") return false;
		if (filename == "ipban.txt") return false;
		if (filename == "map_meta.txt") return false;
		if (filename == "map.sqlite") return false;
		if (filename == "players") return false;
		if (filename == "rollback.txt") return false;
		if (filename == "world.mt") return false;
	}
	else if (loc == FR_User) {
		if (filename == "minetest.conf") return false;
	}
	else {
		return false;
	}
	return true;
}

/******************************************************************************/
bool FileRef::validateFilePermissinons(std::string filename,
										std::string type,
										std::string location) {
	//dir delim is not allowed in filenames at all
	if (filename.find(DIR_DELIM) != std::string::npos)
		return false;

	if ((type ==  "plain"))
		m_type = FR_Plain;

	if ((type ==  "settings"))
		m_type = FR_Settings;

	if ((type ==  "sqlite"))
		m_type = FR_SQLite;

	//check for special files
	if (location == "world") {
		m_storage = FR_World;
		return checkFilename(filename,m_storage);
	}
	else if (location == "user") {
		m_storage = FR_User;
		return checkFilename(filename,m_storage);
	}

	errorstream <<
		"Invalid file type or location for checking filename"
		" ... access DENIED" << std::endl;

	m_storage = FR_Unknown;
	return false;
}

/******************************************************************************/
void FileRef::Register(lua_State *L)
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

		sqlite3_initialize();
	}

/******************************************************************************/
int FileRef::l_listfiles(lua_State *L) {
	std::string type = luaL_checkstring(L, 1);
	if (g_settings->getBool("security_mod_allow_file_listing")) {
		StorageLoc loc = FR_Unknown;
		std::string path = "";
		if (type == "world") {
			loc = FR_World;
			path = get_server(L)->getWorldPath();
		} else if (type == "user") {
			loc = FR_User;
			path = porting::path_user;
		} else {
			lua_pushnil(L);
			return 1;
		}

		std::vector<fs::DirListNode> content = fs::GetDirListing(path);
		int index = 1;

		//create table
		lua_newtable( L );

		for (std::vector<fs::DirListNode>::iterator iter= content.begin();
				iter != content.end();
				iter ++) {

			if ((!iter->dir) &&
				(checkFilename(iter->name,loc)) &&
				(iter->name.compare(0,7,"hidden_") != 0)) {
				lua_pushnumber( L, index );
				lua_pushstring( L, iter->name.c_str());
				lua_settable ( L, -3);
				index ++;
			}
		}
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

/******************************************************************************/
int FileRef::l_delete(lua_State *L) {
	std::string filename = luaL_checkstring(L, 1);
	std::string type = luaL_checkstring(L, 2);

	StorageLoc loc = FR_Unknown;
	std::string path = "";
	if (type == "world") {
		loc = FR_World;
		path = get_server(L)->getWorldPath();
	} else if (type == "user") {
		loc = FR_User;
		path = porting::path_user;
	} else {
		lua_pushnil(L);
		return 1;
	}

	if (checkFilename(path,loc)) {
		if (fs::DeleteSingleFileOrEmptyDirectory(path)) {
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

#define FILEAPI_OPEN_RETURN_NIL  \
	delete ref; \
	lua_pushnil(L); \
	return 1;

/******************************************************************************/
int FileRef::l_open(lua_State *L) {
	std::string filename = luaL_checkstring(L, 1);
	std::string location = luaL_checkstring(L, 2);
	std::string type     = luaL_checkstring(L, 3);
	std::string mode = "d";

	if (((type == "plain") || (type == "sqlite")) && (lua_isnil(L,4)))
		mode = luaL_checkstring(L, 4);

	FileRef* ref = new FileRef();

	if (!ref->validateFilePermissinons(filename,type,location)) {
		FILEAPI_OPEN_RETURN_NIL
	}

	if(!ref->getFilename(filename,L)) {
		FILEAPI_OPEN_RETURN_NIL
	}

	if (ref->m_type == FR_Plain) {
		if (!ref->open(mode)) {
			//TODO add error message
			errorstream << "FileRef: unable to open file" << std::endl;
			if (ref->m_file != 0) {
				delete ref->m_file;
				ref->m_file = 0;
			}
			FILEAPI_OPEN_RETURN_NIL
		}
	}
	else if (ref->m_type == FR_Settings) {
		if (!ref->open_settings()) {
			errorstream << "FileRef: unable to open settings file" << std::endl;
			if (ref->m_settings != 0) {
				delete ref->m_settings;
				ref->m_settings = 0;
			}
			FILEAPI_OPEN_RETURN_NIL
		}
	}
	else if (ref->m_type == FR_SQLite) {
		if(!ref->open_settings_sqlite(mode)) {
			//TODO add error message
			errorstream << "FileRef:unable to open sqlite file" << std::endl;
			if (ref->m_database != 0) {
				sqlite3_close(ref->m_database);
				ref->m_database = 0;
			}
			FILEAPI_OPEN_RETURN_NIL
		}
	}

	FileRef::create_lua_object(L,ref);
	return 1;
}

#undef FILEAPI_OPEN_RETURN_NIL

/******************************************************************************/
int FileRef::l_close(lua_State *L) {
	FileRef* file = checkobject(L, 1);

	if (file->m_type == FR_Plain) {
			file->m_file->close();
			delete file->m_file;
			file->m_file = 0;
		}
	else if (file->m_type == FR_Settings) {
		file->m_settings->updateConfigFile(file->m_filename.c_str());
		delete file->m_settings;
		file->m_settings = 0;
	}
	else if (file->m_type == FR_SQLite){
		file->cleanup_sqlite();
	}
	return 0;
}

/******************************************************************************/
int FileRef::l_getline(lua_State *L) {
	FileRef* file = checkobject(L, 1);

	if ((file->m_type == FR_Plain) &&
		(file->m_file->is_open())){
		std::string line;
		getline(*(file->m_file),line);
		lua_pushstring(L, line.c_str());
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

/******************************************************************************/
int FileRef::l_seekline(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	unsigned int seekto = luaL_checkint(L,2);

	if ((file->m_type == FR_Plain) &&
		(file->m_file->is_open())){

		//do we already know position?
		if (file->m_linepos.size() > seekto) {
			file->m_file->seekg(file->m_linepos[seekto]);
			lua_pushboolean(L, true);
			return 1;
		}

		unsigned int current_line = 0;
		file->m_file->seekg(0);
		if (!file->m_linepos.empty()) {
			current_line = file->m_linepos.size() - 1;
			file->m_file->seekg(file->m_linepos[current_line]);
		}
		else {
			file->m_linepos.push_back(0);
		}

		while( ( current_line < seekto ) && (!file->m_file->eof())) {
			std::string throwaway;
			getline(*(file->m_file),throwaway);
			file->m_linepos.push_back(file->m_file->tellg());
			current_line++;
		}

		if (current_line == seekto) {
			lua_pushboolean(L, true);
			return 1;
		}
	}

	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/
int FileRef::l_write(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	std::string data = luaL_checkstring(L, 2);

	if ((file->m_type != FR_Plain) ||
			(!file->m_writable)) {
		lua_pushboolean(L, false);
		return 1;
	}

	if ((file->m_file->is_open()) &&
		(data != "")){
		if (!file->m_file->eof()) {
			file->m_linepos.clear();
		}
		file->m_file->write(data.c_str(),data.size());
		lua_pushboolean(L, true);
	}
	else {
		errorstream << "file not writable or no data:" << data << std::endl;
		lua_pushboolean(L, false);
	}
	return 1;
}

/******************************************************************************/
int FileRef::l_read(lua_State *L) {
	FileRef* file = checkobject(L, 1);

	if ((file->m_type == FR_Plain) &&
		(file->m_file->is_open())){
		std::string retval = "";
		std::string toappend = "";

		while ( file->m_file->good() ) {
			getline (*(file->m_file),toappend);
			retval += toappend;
			retval += "\n";
		}
		lua_pushstring(L, retval.c_str());
		return 1;
	}

	lua_pushnil(L);
	return 1;
}
/******************************************************************************/
int FileRef::l_setting_getbool(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	std::string key = luaL_checkstring(L, 2);

	if (file->m_type == FR_Settings) {
			try{
				bool value = file->m_settings->getBool(key.c_str());
				lua_pushboolean(L, value);
				return 1;
			} catch(SettingNotFoundException &e){
			}
	}
	else if (file->m_type == FR_SQLite) {
		bool retval = true;
		sqlite3_clear_bindings(file->m_stmt_get_bool);
		retval &= (sqlite3_bind_text(file->m_stmt_get_bool,
							1,
							key.c_str(),
							-1,
							SQLITE_TRANSIENT) == SQLITE_OK);
		if (retval) {
			int result = 0;
			result = sqlite3_step(file->m_stmt_get_bool);
			if (result == SQLITE_ROW) {
				bool data =
						(bool)
						sqlite3_column_int(file->m_stmt_get_bool, 0);
				lua_pushboolean(L,data);
				sqlite3_reset(file->m_stmt_get_bool);
				return 1;
			}
			else {
				errorstream << "SQLITE Config: didn't get row but "
						<< result << " sqlerror was: "
						<< sqlite3_errmsg(file->m_database) << std::endl;
			}
		}
		else {
			errorstream << "SQLITE Config: unable to get data "
					<< sqlite3_errmsg(file->m_database) << std::endl;
		}
	}
	lua_pushnil(L);
	return 1;
}

/******************************************************************************/
int FileRef::l_setting_get(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	std::string key = luaL_checkstring(L, 2);

	if (file->m_type == FR_Settings) {
		lua_pushstring(L,file->m_settings->get(key).c_str());
		return 1;
	}
	else if (file->m_type == FR_SQLite) {
		bool retval = true;
		sqlite3_clear_bindings(file->m_stmt_get_string);
		retval &= (sqlite3_bind_text(file->m_stmt_get_string,
								1,
								key.c_str(),
								-1,
								SQLITE_TRANSIENT) == SQLITE_OK);
		if (retval) {
			int result = 0;

			result = sqlite3_step(file->m_stmt_get_string);

			if (result == SQLITE_ROW) {
				const char* data =
						(const char*)
						sqlite3_column_text(file->m_stmt_get_string, 0);
				lua_pushstring(L,data);
				sqlite3_reset(file->m_stmt_get_string);
				return 1;
			}
			else {
				errorstream << "SQLITE Config: didn't get row but "
						<< result << " sqlerror was: "
						<< sqlite3_errmsg(file->m_database) << std::endl;
			}
		}
		else {
			errorstream << "SQLITE Config: unable to get data "
					<< sqlite3_errmsg(file->m_database) << std::endl;
		}
	}
	lua_pushnil(L);
	return 1;
}
/******************************************************************************/
int FileRef::l_setting_setbool(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	std::string key = luaL_checkstring(L, 2);
	bool value = lua_toboolean(L,3);

	if (file->m_type == FR_Settings) {
		file->m_settings->setBool(key,value);
		lua_pushboolean(L, true);
		return 1;
	}
	else if ((file->m_type == FR_SQLite) && (file->m_writable)){
		bool retval = true;
		sqlite3_clear_bindings(file->m_stmt_set_bool);

		retval &= (sqlite3_bind_int(file->m_stmt_set_bool,
									2,
									value) == SQLITE_OK);

		retval &= (sqlite3_bind_text(file->m_stmt_set_bool,
						1,
						key.c_str(),
						-1,
						SQLITE_TRANSIENT) == SQLITE_OK);

		if (retval) {
			retval &= (sqlite3_step(file->m_stmt_set_bool) == SQLITE_DONE);
		}
		else {
			errorstream << "SQLITE Config: unable to set value " << key
						<< " to " << value << " "
						<< sqlite3_errmsg(file->m_database) << std::endl;
		}
		sqlite3_reset(file->m_stmt_set_bool);
		lua_pushboolean(L, retval);
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/
int FileRef::l_setting_set(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	std::string key = luaL_checkstring(L, 2);

	//allow resetting setting by passing nil to set function
	std::string value = "";
	const char* text = lua_tostring(L, 3);
	if (text != 0) {
		value = std::string(text);
	}

	if (file->m_type == FR_Settings) {
		file->m_settings->set(key,value);
		lua_pushboolean(L, true);
		return 1;
	}
	else if ((file->m_type == FR_SQLite) && (file->m_writable)){
		bool retval = true;
		sqlite3_stmt* tocall;

		//really set value
		if (value != "") {
			tocall = file->m_stmt_set_string;
			sqlite3_clear_bindings(file->m_stmt_set_string);

			retval &= (sqlite3_bind_text(file->m_stmt_set_string,
						2,
						value.c_str(),
						-1,
						SQLITE_TRANSIENT) == SQLITE_OK);

		}
		//delete from database
		else {
			tocall = file->m_stmt_del_string;
			sqlite3_clear_bindings(file->m_stmt_del_string);
		}

		retval &= (sqlite3_bind_text(tocall,
						1,
						key.c_str(),
						-1,
						SQLITE_TRANSIENT) == SQLITE_OK);

		if (retval) {
			retval &= (sqlite3_step(tocall) == SQLITE_DONE);
		}
		else {
			errorstream << "SQLITE Config: unable to set value " << key
						<< " to " << value << " "
						<< sqlite3_errmsg(file->m_database) << std::endl;
		}
		sqlite3_reset(tocall);
		lua_pushboolean(L, retval);
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/
int FileRef::l_save(lua_State *L) {
	FileRef* file = checkobject(L, 1);
	bool retval = false;

	if (file->m_type == FR_Settings) {
		retval = file->m_settings->updateConfigFile(file->m_filename.c_str());
	}

	if ((file->m_type == FR_Plain) && (file->m_writable)) {
		file->m_file->flush();
		retval = true;
	}

	if ((file->m_type == FR_SQLite) && (file->m_writable)) {
		file->m_file->flush();
		retval = true;
	}

	lua_pushboolean(L, retval);
	return 1;
}
