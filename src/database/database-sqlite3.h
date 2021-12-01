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

	inline s64 sqlite_to_int64(sqlite3_stmt *s, int iCol)
	{
		return (s64) sqlite3_column_int64(s, iCol);
	}

	inline u64 sqlite_to_uint64(sqlite3_stmt *s, int iCol)
	{
		return (u64) sqlite3_column_int64(s, iCol);
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

	sqlite3 *m_database = nullptr;
private:
	// Open the database
	void openDatabase();

	bool m_initialized = false;

	std::string m_savedir = "";
	std::string m_dbname = "";

	sqlite3_stmt *m_stmt_begin = nullptr;
	sqlite3_stmt *m_stmt_end = nullptr;

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
	sqlite3_stmt *m_stmt_read = nullptr;
	sqlite3_stmt *m_stmt_write = nullptr;
	sqlite3_stmt *m_stmt_list = nullptr;
	sqlite3_stmt *m_stmt_delete = nullptr;
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
	sqlite3_stmt *m_stmt_player_load = nullptr;
	sqlite3_stmt *m_stmt_player_add = nullptr;
	sqlite3_stmt *m_stmt_player_update = nullptr;
	sqlite3_stmt *m_stmt_player_remove = nullptr;
	sqlite3_stmt *m_stmt_player_list = nullptr;
	sqlite3_stmt *m_stmt_player_load_inventory = nullptr;
	sqlite3_stmt *m_stmt_player_load_inventory_items = nullptr;
	sqlite3_stmt *m_stmt_player_add_inventory = nullptr;
	sqlite3_stmt *m_stmt_player_add_inventory_items = nullptr;
	sqlite3_stmt *m_stmt_player_remove_inventory = nullptr;
	sqlite3_stmt *m_stmt_player_remove_inventory_items = nullptr;
	sqlite3_stmt *m_stmt_player_metadata_load = nullptr;
	sqlite3_stmt *m_stmt_player_metadata_remove = nullptr;
	sqlite3_stmt *m_stmt_player_metadata_add = nullptr;
};

class AuthDatabaseSQLite3 : private Database_SQLite3, public AuthDatabase
{
public:
	AuthDatabaseSQLite3(const std::string &savedir);
	virtual ~AuthDatabaseSQLite3();

	virtual bool getAuth(const std::string &name, AuthEntry &res);
	virtual bool saveAuth(const AuthEntry &authEntry);
	virtual bool createAuth(AuthEntry &authEntry);
	virtual bool deleteAuth(const std::string &name);
	virtual void listNames(std::vector<std::string> &res);
	virtual void reload();

protected:
	virtual void createDatabase();
	virtual void initStatements();

private:
	virtual void writePrivileges(const AuthEntry &authEntry);

	sqlite3_stmt *m_stmt_read = nullptr;
	sqlite3_stmt *m_stmt_write = nullptr;
	sqlite3_stmt *m_stmt_create = nullptr;
	sqlite3_stmt *m_stmt_delete = nullptr;
	sqlite3_stmt *m_stmt_list_names = nullptr;
	sqlite3_stmt *m_stmt_read_privs = nullptr;
	sqlite3_stmt *m_stmt_write_privs = nullptr;
	sqlite3_stmt *m_stmt_delete_privs = nullptr;
	sqlite3_stmt *m_stmt_last_insert_rowid = nullptr;
};

class ModMetadataDatabaseSQLite3 : private Database_SQLite3, public ModMetadataDatabase
{
public:
	ModMetadataDatabaseSQLite3(const std::string &savedir);
	virtual ~ModMetadataDatabaseSQLite3();

	virtual bool getModEntries(const std::string &modname, StringMap *storage);
	virtual bool setModEntry(const std::string &modname,
		const std::string &key, const std::string &value);
	virtual bool removeModEntry(const std::string &modname, const std::string &key);
	virtual void listMods(std::vector<std::string> *res);

	virtual void beginSave() { Database_SQLite3::beginSave(); }
	virtual void endSave() { Database_SQLite3::endSave(); }

protected:
	virtual void createDatabase();
	virtual void initStatements();

private:
	sqlite3_stmt *m_stmt_get = nullptr;
	sqlite3_stmt *m_stmt_set = nullptr;
	sqlite3_stmt *m_stmt_remove = nullptr;
};
