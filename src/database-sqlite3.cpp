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

/*
SQLite format specification:
	blocks:
		(PK) INT id
		BLOB data
*/


#include "database-sqlite3.h"

#include "log.h"
#include "filesys.h"
#include "exceptions.h"
#include "main.h"
#include "settings.h"
#include "util/string.h"

#include <cassert>


#define SQLRES(s, r) \
	if ((s) != (r)) { \
		throw FileNotGoodException(std::string(\
					"SQLite3 database error (" \
					__FILE__ ":" TOSTRING(__LINE__) \
					"): ") +\
				sqlite3_errmsg(m_database)); \
	}
#define SQLOK(s) SQLRES(s, SQLITE_OK)

#define PREPARE_STATEMENT(name, query) \
	SQLOK(sqlite3_prepare_v2(m_database, query, -1, &m_stmt_##name, NULL))

#define FINALIZE_STATEMENT(statement) \
	if (sqlite3_finalize(statement) != SQLITE_OK) { \
		throw FileNotGoodException(std::string( \
			"SQLite3: Failed to finalize " #statement ": ") + \
			 sqlite3_errmsg(m_database)); \
	}


Database_SQLite3::Database_SQLite3(const std::string &savedir) :
	m_initialized(false),
	m_savedir(savedir),
	m_database(NULL),
	m_stmt_read(NULL),
	m_stmt_write(NULL),
	m_stmt_list(NULL),
	m_stmt_delete(NULL)
{
}

void Database_SQLite3::beginSave() {
	verifyDatabase();
	SQLRES(sqlite3_step(m_stmt_begin), SQLITE_DONE);
	sqlite3_reset(m_stmt_begin);
}

void Database_SQLite3::endSave() {
	verifyDatabase();
	SQLRES(sqlite3_step(m_stmt_end), SQLITE_DONE);
	sqlite3_reset(m_stmt_end);
}

void Database_SQLite3::openDatabase()
{
	if (m_database) return;

	std::string dbp = m_savedir + DIR_DELIM + "map.sqlite";

	// Open the database connection

	if (!fs::CreateAllDirs(m_savedir)) {
		infostream << "Database_SQLite3: Failed to create directory \""
			<< m_savedir << "\"" << std::endl;
		throw FileNotGoodException("Failed to create database "
				"save directory");
	}

	bool needs_create = !fs::PathExists(dbp);

	if (sqlite3_open_v2(dbp.c_str(), &m_database,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL) != SQLITE_OK) {
		errorstream << "SQLite3 database failed to open: "
			<< sqlite3_errmsg(m_database) << std::endl;
		throw FileNotGoodException("Cannot open database file");
	}

	if (needs_create) {
		createDatabase();
	}

	std::string query_str = std::string("PRAGMA synchronous = ")
			 + itos(g_settings->getU16("sqlite_synchronous"));
	SQLOK(sqlite3_exec(m_database, query_str.c_str(), NULL, NULL, NULL));
}

void Database_SQLite3::verifyDatabase()
{
	if (m_initialized) return;

	openDatabase();

	PREPARE_STATEMENT(begin, "BEGIN");
	PREPARE_STATEMENT(end, "COMMIT");
	PREPARE_STATEMENT(read, "SELECT `data` FROM `blocks` WHERE `pos` = ? LIMIT 1");
#ifdef __ANDROID__
	PREPARE_STATEMENT(write,  "INSERT INTO `blocks` (`pos`, `data`) VALUES (?, ?)");
#else
	PREPARE_STATEMENT(write, "REPLACE INTO `blocks` (`pos`, `data`) VALUES (?, ?)");
#endif
	PREPARE_STATEMENT(delete, "DELETE FROM `blocks` WHERE `pos` = ?");
	PREPARE_STATEMENT(list, "SELECT `pos` FROM `blocks`");

	m_initialized = true;

	verbosestream << "ServerMap: SQLite3 database opened." << std::endl;
}

inline void Database_SQLite3::bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int index)
{
	SQLOK(sqlite3_bind_int64(stmt, index, getBlockAsInteger(pos)));
}

bool Database_SQLite3::deleteBlock(const v3s16 &pos)
{
	verifyDatabase();

	bindPos(m_stmt_delete, pos);

	bool good = sqlite3_step(m_stmt_delete) == SQLITE_DONE;
	sqlite3_reset(m_stmt_delete);

	if (!good) {
		errorstream << "WARNING: deleteBlock: Block failed to delete "
			<< PP(pos) << ": " << sqlite3_errmsg(m_database) << std::endl;
	}
	return good;
}

bool Database_SQLite3::saveBlock(const v3s16 &pos, const std::string &data)
{
	verifyDatabase();

#ifdef __ANDROID__
	/**
	 * Note: For some unknown reason SQLite3 fails to REPLACE blocks on Android,
	 * deleting them and then inserting works.
	 */
	bindPos(m_stmt_read, pos);

	if (sqlite3_step(m_stmt_read) == SQLITE_ROW) {
		deleteBlock(pos);
	}
	sqlite3_reset(m_stmt_read);
#endif

	bindPos(m_stmt_write, pos);
	SQLOK(sqlite3_bind_blob(m_stmt_write, 2, data.data(), data.size(), NULL));

	SQLRES(sqlite3_step(m_stmt_write), SQLITE_DONE)
	sqlite3_reset(m_stmt_write);

	return true;
}

std::string Database_SQLite3::loadBlock(const v3s16 &pos)
{
	verifyDatabase();

	bindPos(m_stmt_read, pos);

	if (sqlite3_step(m_stmt_read) != SQLITE_ROW) {
		sqlite3_reset(m_stmt_read);
		return "";
	}
	const char *data = (const char *) sqlite3_column_blob(m_stmt_read, 0);
	size_t len = sqlite3_column_bytes(m_stmt_read, 0);

	std::string s;
	if (data)
		s = std::string(data, len);

	sqlite3_step(m_stmt_read);
	// We should never get more than 1 row, so ok to reset
	sqlite3_reset(m_stmt_read);

	return s;
}

void Database_SQLite3::createDatabase()
{
	assert(m_database);
	SQLOK(sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `blocks` (\n"
		"	`pos` INT PRIMARY KEY,\n"
		"	`data` BLOB\n"
		");\n",
		NULL, NULL, NULL));

}

void Database_SQLite3::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	verifyDatabase();

	while (sqlite3_step(m_stmt_list) == SQLITE_ROW) {
		dst.push_back(getIntegerAsBlock(sqlite3_column_int64(m_stmt_list, 0)));
	}
	sqlite3_reset(m_stmt_list);
}

Database_SQLite3::~Database_SQLite3()
{
	FINALIZE_STATEMENT(m_stmt_read)
	FINALIZE_STATEMENT(m_stmt_write)
	FINALIZE_STATEMENT(m_stmt_list)
	FINALIZE_STATEMENT(m_stmt_begin)
	FINALIZE_STATEMENT(m_stmt_end)
	FINALIZE_STATEMENT(m_stmt_delete)

	if (sqlite3_close(m_database) != SQLITE_OK) {
		errorstream << "Database_SQLite3::~Database_SQLite3(): "
				<< "Failed to close database: "
				<< sqlite3_errmsg(m_database) << std::endl;
	}
}

