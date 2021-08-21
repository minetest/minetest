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

#include "database.h"
#include "exceptions.h"
#include "util/basic_macros.h"
#include <cstring>
#include <ctime>
#include <string>

extern "C" {
#include "sqlite3.h"
}

#ifndef SERVER
#include "util/Optional.h"
#include <utility>

enum class MediaCacheClass : u16;
enum class MediaDataFormat : u16;
#endif

class Database_SQLite3 : public Database
{
public:
	virtual ~Database_SQLite3();

	DISABLE_CLASS_COPY(Database_SQLite3);
	DISABLE_CLASS_MOVE(Database_SQLite3);

	void beginSave();
	void endSave();

	bool initialized() const { return m_initialized; }
protected:
	Database_SQLite3(const std::string &savedir, const std::string &dbname);

	// Open and initialize the database if needed
	void verifyDatabase();

	// Convertors
	inline void str_to_sqlite(sqlite3_stmt *s, int iBind, const std::string &str) const
	{
		sqlite3_vrfy(sqlite3_bind_text(s, iBind, str.c_str(), str.size(), NULL));
	}

	inline void str_to_sqlite(sqlite3_stmt *s, int iBind, const char *str) const
	{
		sqlite3_vrfy(sqlite3_bind_text(s, iBind, str, strlen(str), NULL));
	}

	// please clear bindings to s before data is deleted or modified
	inline void blob_str_to_sqlite(sqlite3_stmt *s, int iBind, const std::string &data) const
	{
		sqlite3_vrfy(sqlite3_bind_blob(s, iBind, data.c_str(), data.size(), SQLITE_STATIC));
	}

	inline void int_to_sqlite(sqlite3_stmt *s, int iBind, int val) const
	{
		sqlite3_vrfy(sqlite3_bind_int(s, iBind, val));
	}

	inline void int64_to_sqlite(sqlite3_stmt *s, int iBind, s64 val) const
	{
		sqlite3_vrfy(sqlite3_bind_int64(s, iBind, (sqlite3_int64) val));
	}

	inline void double_to_sqlite(sqlite3_stmt *s, int iBind, double val) const
	{
		sqlite3_vrfy(sqlite3_bind_double(s, iBind, val));
	}

	inline std::string sqlite_to_string(sqlite3_stmt *s, int iCol)
	{
		const char *text = reinterpret_cast<const char*>(sqlite3_column_text(s, iCol));
		return std::string(text ? text : "");
	}

	inline std::string sqlite_to_blob_string(sqlite3_stmt *s, int iCol)
	{
		const char *data = reinterpret_cast<const char *>(sqlite3_column_blob(s, iCol));
		size_t size = static_cast<size_t>(sqlite3_column_bytes(s, iCol));
		return std::string(data ? data : "", size);
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

#ifndef SERVER
struct MediaCacheUpdateFileArg {
	std::string sha1_digest; // in hex
	MediaCacheClass cache_class;
	MediaDataFormat data_format;
	std::string data;
};

class MediaCacheDatabaseSQLite3 final : private Database_SQLite3
{
public:
	MediaCacheDatabaseSQLite3(const std::string &savedir);
	virtual ~MediaCacheDatabaseSQLite3() override;

	/** Searches the db for a file.
	 *
	 * Note: If you have multiple files, prefer findFiles.
	 *
	 * if not found:
	 * * returns false
	 * if found:
	 * * returns true
	 * * sets out_data and out_data_format
	 * * updates the file's timestamp
	 * @param sha1_digest hex-encoded sha1 digest
	 * @param out_data will contain the data afterwards if found. can be nullptr
	 * @param out_data_format will contain the data format afterwards if found.
	 *        can be nullptr
	 * @return whether the file was found
	 */
	bool findFile(const std::string &sha1_digest,
			std::string *out_data = nullptr, MediaDataFormat *out_data_format = nullptr);
	/** Like findFile, but does many files at once.
	 *
	 * @return a vector<Optional<pair<data, data_format>>>. the vectors i-th value
	 *         corresponds to the i-th entry of sha1_digests
	 */
	std::vector<Optional<std::pair<std::string, MediaDataFormat>>> findFiles(
			const std::vector<std::string> &sha1_digests);

	/** Inserts a new file into the cache db.
	 * Replaces old row if existent.
	 *
	 * Note: If you have multiple files, prefer updateFiles.
	 *
	 * @param sha1_digest hex-encoded sha1 digest
	 * @param cache_class the cache-class for the file
	 * @param data_format the data-format of the file-data
	 * @param data the file data
	 */
	void updateFile(const std::string &sha1_digest, MediaCacheClass cache_class,
			MediaDataFormat data_format, const std::string &data);
	void updateFile(const MediaCacheUpdateFileArg &file)
	{
		updateFile(file.sha1_digest, file.cache_class, file.data_format, file.data);
	}
	/** Like updateFile, but does many files at once.
	 *
	 * @param files the files to update
	 */
	void updateFiles(const std::vector<MediaCacheUpdateFileArg> &files);

	/** Adds the files (like updateFiles), then cleans the cache by removing old
	 * files, and may do some other householding actions, like vacuuming.
	 *
	 * @param files new files
	 * @param cache_class_oldest_times for each cache-class the time point in seconds
	 *        sice epoch since which files are kept. older files are deleted
	 */
	void updateFilesAndClean(const std::vector<MediaCacheUpdateFileArg> &files,
			const std::vector<std::time_t> &cache_class_oldest_times);

protected:
	void createDatabase() override;
	void initStatements() override;

private:
	void beginSaveExclusive();
	bool findFileSingle(const std::string &sha1_digest,
			std::string *out_data = nullptr, MediaDataFormat *out_data_format = nullptr);
	void updateFileSingle(const std::string &sha1_digest, MediaCacheClass cache_class,
			MediaDataFormat data_format, const std::string &data);

	sqlite3_stmt *m_stmt_begin_exclusive = nullptr;
	sqlite3_stmt *m_stmt_lookup = nullptr;
	sqlite3_stmt *m_stmt_touch = nullptr;
	sqlite3_stmt *m_stmt_insert = nullptr;
	sqlite3_stmt *m_stmt_expire_olds = nullptr;
	sqlite3_stmt *m_stmt_update_deleted_since_vacuum = nullptr;
	sqlite3_stmt *m_stmt_get_deleted_since_vacuum = nullptr;
	sqlite3_stmt *m_stmt_reset_deleted_since_vacuum = nullptr;
	sqlite3_stmt *m_stmt_delete_twice_expired = nullptr;
	sqlite3_stmt *m_stmt_vacuum = nullptr;
};
#endif
