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

#include <cstring>
#include <string>
#include "database.h"
#include "exceptions.h"

extern "C" {
#include "sqlite3.h"
}

class Database_SQLite3 : public Database
{
public:
	virtual ~Database_SQLite3();

	void beginSave();
	void endSave();

	bool initialized() const { return m_initialized; }
protected:
	Database_SQLite3(const std::string &savedir, const std::string &dbname);

	// Open and initialize the database if needed
	void verifyDatabase();

	// Convertors
	inline void str_to_sqlite(sqlite3_stmt *s, int iCol, const std::string &str) const
	{
		sqlite3_vrfy(sqlite3_bind_text(s, iCol, str.c_str(), str.size(), NULL));
	}

	inline void str_to_sqlite(sqlite3_stmt *s, int iCol, const char *str) const
	{
		sqlite3_vrfy(sqlite3_bind_text(s, iCol, str, strlen(str), NULL));
	}

	inline void int_to_sqlite(sqlite3_stmt *s, int iCol, int val) const
	{
		sqlite3_vrfy(sqlite3_bind_int(s, iCol, val));
	}

	inline void int64_to_sqlite(sqlite3_stmt *s, int iCol, s64 val) const
	{
		sqlite3_vrfy(sqlite3_bind_int64(s, iCol, (sqlite3_int64) val));
	}

	inline void double_to_sqlite(sqlite3_stmt *s, int iCol, double val) const
	{
		sqlite3_vrfy(sqlite3_bind_double(s, iCol, val));
	}

	inline std::string sqlite_to_string(sqlite3_stmt *s, int iCol)
	{
		const char* text = reinterpret_cast<const char*>(sqlite3_column_text(s, iCol));
		return std::string(text ? text : "");
	}

	inline s32 sqlite_to_int(sqlite3_stmt *s, int iCol)
	{
		return sqlite3_column_int(s, iCol);
	}

	inline u32 sqlite_to_uint(sqlite3_stmt *s, int iCol)
	{
		return (u32) sqlite3_column_int(s, iCol);
	}

	inline float sqlite_to_float(sqlite3_stmt *s, int iCol)
	{
		return (float) sqlite3_column_double(s, iCol);
	}

	inline const v3f sqlite_to_v3f(sqlite3_stmt *s, int iCol)
	{
		return v3f(sqlite_to_float(s, iCol), sqlite_to_float(s, iCol + 1),
				sqlite_to_float(s, iCol + 2));
	}

	// Query verifiers helpers
	inline void sqlite3_vrfy(int s, const std::string &m = "", int r = SQLITE_OK) const
	{
		if (s != r)
			throw DatabaseException(m + ": " + sqlite3_errmsg(m_database));
	}

	inline void sqlite3_vrfy(const int s, const int r, const std::string &m = "") const
	{
		sqlite3_vrfy(s, m, r);
	}

	// Create the database structure
	virtual void createDatabase() = 0;
	virtual void initStatements() = 0;

	sqlite3 *m_database;
private:
	// Open the database
	void openDatabase();

	bool m_initialized;

	std::string m_savedir;
	std::string m_dbname;

	sqlite3_stmt *m_stmt_begin;
	sqlite3_stmt *m_stmt_end;

	s64 m_busy_handler_data[2];

	static int busyHandler(void *data, int count);
};

class MapDatabaseSQLite3 : private Database_SQLite3, public MapDatabase
{
public:
	MapDatabaseSQLite3(const std::string &savedir);
	virtual ~MapDatabaseSQLite3();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);

	void beginSave() { Database_SQLite3::beginSave(); }
	void endSave() { Database_SQLite3::endSave(); }
protected:
	virtual void createDatabase();
	virtual void initStatements();

private:
	void bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int index = 1);

	// Map
	sqlite3_stmt *m_stmt_read;
	sqlite3_stmt *m_stmt_write;
	sqlite3_stmt *m_stmt_list;
	sqlite3_stmt *m_stmt_delete;
};

class PlayerDatabaseSQLite3 : private Database_SQLite3, public PlayerDatabase
{
public:
	PlayerDatabaseSQLite3(const std::string &savedir);
	virtual ~PlayerDatabaseSQLite3();

	void savePlayer(RemotePlayer *player);
	bool loadPlayer(RemotePlayer *player, PlayerSAO *sao);
	bool removePlayer(const std::string &name);
	void listPlayers(std::vector<std::string> &res);

protected:
	virtual void createDatabase();
	virtual void initStatements();

private:
	bool playerDataExists(const std::string &name);

	// Players
	sqlite3_stmt *m_stmt_player_load;
	sqlite3_stmt *m_stmt_player_add;
	sqlite3_stmt *m_stmt_player_update;
	sqlite3_stmt *m_stmt_player_remove;
	sqlite3_stmt *m_stmt_player_list;
	sqlite3_stmt *m_stmt_player_load_inventory;
	sqlite3_stmt *m_stmt_player_load_inventory_items;
	sqlite3_stmt *m_stmt_player_add_inventory;
	sqlite3_stmt *m_stmt_player_add_inventory_items;
	sqlite3_stmt *m_stmt_player_remove_inventory;
	sqlite3_stmt *m_stmt_player_remove_inventory_items;
	sqlite3_stmt *m_stmt_player_metadata_load;
	sqlite3_stmt *m_stmt_player_metadata_remove;
	sqlite3_stmt *m_stmt_player_metadata_add;
};

#endif
