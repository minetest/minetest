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

class Settings;

class Database_SQLite3 : public Database
{
public:
	Database_SQLite3(const std::string &savedir);
	~Database_SQLite3();

	void beginSave();
	void endSave();

	bool saveBlock(const v3s16 &pos, const std::string &data)
		{ return saveBlock(pos, data.data(), data.size()); }
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);

private:
	// Open the database
	void openDatabase();
	// Create the database structure
	void createDatabase(bool set_ver=true);
	// Pepares statements that depend on the database having an up-to-date schema
	void prepareStatements();
	// Check of the database is an old version and needs to be updated
	void checkMigrate();
	// Update database from version 0 to version 1 (pos split)
	bool migrate(bool started);

	bool saveBlock(const v3s16 &pos, const char *data, size_t data_len,
			s64 id = -1);

	bool deleteBlock(s64 id);

	void bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int start = 1);

	std::string m_savedir;

	sqlite3 *m_database;
	sqlite3_stmt *m_stmt_read;
	sqlite3_stmt *m_stmt_insert;
	sqlite3_stmt *m_stmt_insert_pos;
	sqlite3_stmt *m_stmt_update;
	sqlite3_stmt *m_stmt_list;
	sqlite3_stmt *m_stmt_get_id;
	sqlite3_stmt *m_stmt_delete;
	sqlite3_stmt *m_stmt_delete_pos;
	sqlite3_stmt *m_stmt_begin;
	sqlite3_stmt *m_stmt_end;

	s64 m_busy_handler_data[2];

	static int busyHandler(void *data, int count);
};

#endif
