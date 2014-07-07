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

#ifndef DATABASE_SQLITE3_HEADER
#define DATABASE_SQLITE3_HEADER

#include "database.h"
#include <string>

extern "C" {
	#include "sqlite3.h"
}

class ServerMap;

class Database_SQLite3 : public Database
{
public:
	Database_SQLite3(ServerMap *map, std::string savedir);
	virtual void beginSave();
	virtual void endSave();

	virtual bool saveBlock(v3s16 blockpos, std::string &data);
	virtual std::string loadBlock(v3s16 blockpos);
	virtual void listAllLoadableBlocks(std::list<v3s16> &dst);
	virtual int Initialized(void);
	~Database_SQLite3();
private:
	ServerMap *srvmap;
	std::string m_savedir;
	sqlite3 *m_database;
	sqlite3_stmt *m_database_read;
	sqlite3_stmt *m_database_write;
#ifdef __ANDROID__
	sqlite3_stmt *m_database_delete;
#endif
	sqlite3_stmt *m_database_list;

	// Create the database structure
	void createDatabase();
	// Verify we can read/write to the database
	void verifyDatabase();
	void createDirs(std::string path);
};

#endif
