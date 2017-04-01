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

#include <string>
#include "database.h"

extern "C" {
#include "sqlite3.h"
}

class Database_SQLite3 : public Database
{
public:
	Database_SQLite3(const std::string &savedir);
	~Database_SQLite3();

	void beginSave();
	void endSave();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	bool initialized() const { return m_initialized; }

private:
	// Open the database
	void openDatabase();
	// Create the database structure
	void createDatabase();
	// Open and initialize the database if needed
	void verifyDatabase();

	void bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int index = 1);

	bool m_initialized;

	std::string m_savedir;

	sqlite3 *m_database;
	sqlite3_stmt *m_stmt_read;
	sqlite3_stmt *m_stmt_write;
	sqlite3_stmt *m_stmt_list;
	sqlite3_stmt *m_stmt_delete;
	sqlite3_stmt *m_stmt_begin;
	sqlite3_stmt *m_stmt_end;

	s64 m_busy_handler_data[2];

	static int busyHandler(void *data, int count);
};

#endif
