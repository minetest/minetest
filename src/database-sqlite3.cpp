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
		int PK x
		int PK y
		int PK z
		BLOB data
*/


#include "database-sqlite3.h"

#include "log.h"
#include "filesys.h"
#include "exceptions.h"
#include "settings.h"
#include "porting.h"
#include "util/string.h"

#include <cassert>
#include <iostream>


#define LATEST_DB_VERSION 1

// When to print messages when the database is being held locked by another process
// Note: I've seen occasional delays of over 250ms while running minetestmapper.
#define BUSY_INFO_TRESHOLD	100	// Print first informational message after 100ms.
#define BUSY_WARNING_TRESHOLD	250	// Print warning message after 250ms. Lag is increased.
#define BUSY_ERROR_TRESHOLD	1000	// Print error message after 1000ms. Significant lag.
#define BUSY_FATAL_TRESHOLD	3000	// Allow SQLITE_BUSY to be returned, which will cause a minetest crash.
#define BUSY_ERROR_INTERVAL	10000	// Safety net: report again every 10 seconds


#define SQLRES(s, r, m) \
	if ((s) != (r)) { \
		throw DatabaseException(std::string(m) + ": " +\
				sqlite3_errmsg(m_database)); \
	}
#define SQLOK(s, m) SQLRES(s, SQLITE_OK, m)

#define PREPARE_STATEMENT(stmt, query) \
	SQLOK(sqlite3_prepare_v2(m_database, query, -1, &(stmt), NULL),\
		"Failed to prepare query '" query "'")

#define SQLOK_ERRSTREAM(s, m)                           \
	if ((s) != SQLITE_OK) {                             \
		errorstream << (m) << ": "                      \
			<< sqlite3_errmsg(m_database) << std::endl; \
	}

#define FINALIZE_STATEMENT(statement) SQLOK_ERRSTREAM(sqlite3_finalize(statement), \
	"Failed to finalize " #statement)


Database_SQLite3::Database_SQLite3(const std::string &savedir) :
	m_savedir(savedir),
	m_database(NULL),
	m_stmt_read(NULL),
	m_stmt_insert(NULL),
	m_stmt_insert_pos(NULL),
	m_stmt_update(NULL),
	m_stmt_list(NULL),
	m_stmt_get_id(NULL),
	m_stmt_delete(NULL),
	m_stmt_delete_pos(NULL),
	m_stmt_begin(NULL),
	m_stmt_end(NULL)
{
	openDatabase();
}

int Database_SQLite3::busyHandler(void *data, int count)
{
	s64 &first_time = reinterpret_cast<s64 *>(data)[0];
	s64 &prev_time = reinterpret_cast<s64 *>(data)[1];
	s64 cur_time = getTimeMs();

	if (count == 0) {
		first_time = cur_time;
		prev_time = first_time;
	} else {
		while (cur_time < prev_time)
			cur_time += s64(1)<<32;
	}

	if (cur_time - first_time < BUSY_INFO_TRESHOLD) {
		; // do nothing
	} else if (cur_time - first_time >= BUSY_INFO_TRESHOLD &&
			prev_time - first_time < BUSY_INFO_TRESHOLD) {
		infostream << "SQLite3 database has been locked for "
			<< cur_time - first_time << " ms." << std::endl;
	} else if (cur_time - first_time >= BUSY_WARNING_TRESHOLD &&
			prev_time - first_time < BUSY_WARNING_TRESHOLD) {
		warningstream << "SQLite3 database has been locked for "
			<< cur_time - first_time << " ms." << std::endl;
	} else if (cur_time - first_time >= BUSY_ERROR_TRESHOLD &&
			prev_time - first_time < BUSY_ERROR_TRESHOLD) {
		errorstream << "SQLite3 database has been locked for "
			<< cur_time - first_time << " ms; this causes lag." << std::endl;
	} else if (cur_time - first_time >= BUSY_FATAL_TRESHOLD &&
			prev_time - first_time < BUSY_FATAL_TRESHOLD) {
		errorstream << "SQLite3 database has been locked for "
			<< cur_time - first_time << " ms - giving up!" << std::endl;
	} else if ((cur_time - first_time) / BUSY_ERROR_INTERVAL !=
			(prev_time - first_time) / BUSY_ERROR_INTERVAL) {
		// Safety net: keep reporting every BUSY_ERROR_INTERVAL
		errorstream << "SQLite3 database has been locked for "
			<< (cur_time - first_time) / 1000 << " seconds!" << std::endl;
	}

	prev_time = cur_time;

	// Make sqlite transaction fail if delay exceeds BUSY_FATAL_TRESHOLD
	return cur_time - first_time < BUSY_FATAL_TRESHOLD;
}

void Database_SQLite3::checkMigrate()
{
	sqlite3_stmt *stmt_get_version = NULL;
	PREPARE_STATEMENT(stmt_get_version, "PRAGMA user_version");

	if (sqlite3_step(stmt_get_version) != SQLITE_ROW) {
		throw FileNotGoodException("Fetching SQLite3 database "
			"version failed!");
	}

	int version = sqlite3_column_int(stmt_get_version, 0);
	FINALIZE_STATEMENT(stmt_get_version);

	switch (version) {
	case 0: {
		std::string flag_path = m_savedir + DIR_DELIM + "map.sqlite.migrating";
		bool migrationStarted = fs::PathExists(flag_path);
		if (!migrationStarted) {
			std::ofstream flag(flag_path.c_str());
		}
		if (!migrate(migrationStarted))
			throw FileNotGoodException("Migration paused");
		std::cerr << "Map schema successfully migrated!" << std::endl;
		fs::DeleteSingleFileOrEmptyDirectory(flag_path);
	}
	case LATEST_DB_VERSION:
		// Up-to-date
		break;
	default:
		throw FileNotGoodException("Unsupported database version " +
			std::to_string(version) + "!");
	}
}

bool Database_SQLite3::migrate(bool started)
{
	std::cerr << "Migrating map schema..." << std::endl;

	beginSave();

	// TODO: Check table instead of relying on flag file
	if (!started) {
		SQLOK(sqlite3_exec(m_database,
			"ALTER TABLE `blocks` RENAME TO `old_blocks`;",
			NULL, NULL, NULL), "Failed to rename table for migration");
		createDatabase(false);
	}

	prepareStatements();

	sqlite3_stmt *stmt_get = NULL;
	sqlite3_stmt *stmt_delete = NULL;

	PREPARE_STATEMENT(stmt_get, "SELECT `pos`, `data` FROM `old_blocks`");
	PREPARE_STATEMENT(stmt_delete, "DELETE FROM `old_blocks` WHERE `pos` = ?");

	bool &kill = *porting::signal_handler_killstatus();
	time_t last_time = time(NULL);
	char spinner[] = "|/-\\";
	unsigned short spinidx = 0;
	unsigned long num_converted = 0;

	while (sqlite3_step(stmt_get) == SQLITE_ROW) {
		s64 posHash = sqlite3_column_int64(stmt_get, 0);
		v3s16 pos = getIntegerAsBlock(posHash);
		const char *data = static_cast<const char *>(sqlite3_column_blob(stmt_get, 1));
		size_t data_len = sqlite3_column_bytes(stmt_get, 1);

		saveBlock(pos, data, data_len, 0);

		// Delete block from old table now to allow migration resumption
		SQLOK(sqlite3_bind_int64(stmt_delete, 1, posHash),
			"Failed to bind old migrated block pos for delete");
		SQLRES(sqlite3_step(stmt_delete), SQLITE_DONE,
			"Failed to delete old migrated block");
		sqlite3_reset(stmt_delete);

		num_converted++;
		if (time(NULL) - last_time > 1) {
			endSave();
			spinidx = (spinidx + 1) & 3;
			std::cerr << spinner[spinidx] << " Converted "
				<< num_converted << " blocks...\r"
				<< std::flush;
			beginSave();
			last_time = time(NULL);
		}
		if (kill)
			goto finish;
	}

	SQLOK(sqlite3_exec(m_database,
		"DROP TABLE `old_blocks`;\n"
		"PRAGMA user_version = 1;\n",
		NULL, NULL, NULL), "Failed to delete old block table "
			"and update database version");

finish:
	endSave();

	FINALIZE_STATEMENT(stmt_get);
	FINALIZE_STATEMENT(stmt_delete);

	std::cerr << std::endl;

	return !kill;
}

void Database_SQLite3::beginSave()
{
	SQLRES(sqlite3_step(m_stmt_begin), SQLITE_DONE,
		"Failed to start SQLite3 transaction");
	sqlite3_reset(m_stmt_begin);
}

void Database_SQLite3::endSave()
{
	SQLRES(sqlite3_step(m_stmt_end), SQLITE_DONE,
		"Failed to commit SQLite3 transaction");
	sqlite3_reset(m_stmt_end);
}

void Database_SQLite3::openDatabase()
{
	if (m_database)
		return;

	std::string dbp = m_savedir + DIR_DELIM + "map.sqlite";

	// Open the database connection

	if (!fs::CreateAllDirs(m_savedir)) {
		infostream << "Database_SQLite3: Failed to create directory \""
			<< m_savedir << "\"" << std::endl;
		throw FileNotGoodException("Failed to create database "
				"save directory");
	}

	bool needs_create = !fs::PathExists(dbp);

	SQLOK(sqlite3_open_v2(dbp.c_str(), &m_database,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL),
		std::string("Failed to open SQLite3 database file ") + dbp);

	SQLOK(sqlite3_busy_handler(m_database, Database_SQLite3::busyHandler,
		m_busy_handler_data), "Failed to set SQLite3 busy handler");

	if (needs_create)
		createDatabase();

	std::string query_str = std::string("PRAGMA synchronous = ")
			 + itos(g_settings->getU16("sqlite_synchronous"));
	SQLOK(sqlite3_exec(m_database, query_str.c_str(), NULL, NULL, NULL),
		"Failed to modify sqlite3 synchronous mode");

	PREPARE_STATEMENT(m_stmt_begin, "BEGIN");
	PREPARE_STATEMENT(m_stmt_end, "COMMIT");

	checkMigrate();

	prepareStatements();

	verbosestream << "ServerMap: SQLite3 database opened." << std::endl;
}

void Database_SQLite3::prepareStatements()
{
	if (m_stmt_read != NULL)
		return;

	PREPARE_STATEMENT(m_stmt_read, "SELECT `data` FROM "
			"`blocks` JOIN `block_pos` USING (`id`) "
			"WHERE `x1` = ? AND `y1` = ? AND `z1` = ?");

	PREPARE_STATEMENT(m_stmt_insert, "INSERT INTO `blocks` (`data`) VALUES (?)");

	PREPARE_STATEMENT(m_stmt_update, "UPDATE `blocks` "
			"SET `data` = ? WHERE `id` = ?");

	PREPARE_STATEMENT(m_stmt_insert_pos, "INSERT INTO `block_pos` "
			"(`id`, `x1`, `x2`, `y1`, `y2`, `z1`, `z2`) "
			"VALUES (?1, ?2, ?2, ?3, ?3, ?4, ?4)");

	PREPARE_STATEMENT(m_stmt_get_id, "SELECT `id` FROM `block_pos` "
			"WHERE `x1` = ? AND `y1` = ? AND `z1` = ?");

	PREPARE_STATEMENT(m_stmt_delete, "DELETE FROM `blocks` WHERE `id` = ?");
	PREPARE_STATEMENT(m_stmt_delete_pos, "DELETE FROM `block_pos` WHERE `id` = ?");

	PREPARE_STATEMENT(m_stmt_list, "SELECT `x1`, `y1`, `z1` FROM `block_pos`");
}

inline void Database_SQLite3::bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int start)
{
	SQLOK(sqlite3_bind_int(stmt, start,     pos.X), "Failed to bind block X coordinate");
	SQLOK(sqlite3_bind_int(stmt, start + 1, pos.Y), "Failed to bind block Y coordinate");
	SQLOK(sqlite3_bind_int(stmt, start + 2, pos.Z), "Failed to bind block Z coordinate");
}

bool Database_SQLite3::deleteBlock(const v3s16 &pos)
{
	bindPos(m_stmt_get_id, pos);

	if (sqlite3_step(m_stmt_get_id) != SQLITE_ROW) {
		sqlite3_reset(m_stmt_get_id);
		warningstream << "deleteBlock: Block failed to get ID for "
			<< PP(pos) << ": " << sqlite3_errmsg(m_database) << std::endl;
		return false;
	}
	s64 id = sqlite3_column_int64(m_stmt_get_id, 0);
	sqlite3_reset(m_stmt_get_id);

	return deleteBlock(id);
}

bool Database_SQLite3::deleteBlock(s64 id)
{
	bool good;
	SQLOK(sqlite3_bind_int64(m_stmt_delete, 1, id),
			"Failed to bind block ID for delete");
	good = sqlite3_step(m_stmt_delete) == SQLITE_DONE;
	sqlite3_reset(m_stmt_delete);
	if (!good) {
		warningstream << "deleteBlock: Block failed to delete "
			<< id << ": " << sqlite3_errmsg(m_database) << std::endl;
		return false;
	}

	SQLOK(sqlite3_bind_int64(m_stmt_delete_pos, 1, id),
			"Failed to bind block ID for delete");
	good = sqlite3_step(m_stmt_delete_pos) == SQLITE_DONE;
	sqlite3_reset(m_stmt_delete_pos);
	if (!good) {
		warningstream << "deleteBlock: Block failed to delete position "
			<< id << ": " << sqlite3_errmsg(m_database) << std::endl;
		return false;
	}

	return true;
}

bool Database_SQLite3::saveBlock(const v3s16 &pos, const char *data,
		size_t data_len, s64 id)
{
	if (id == -1) {
		bindPos(m_stmt_get_id, pos);

		if (sqlite3_step(m_stmt_get_id) == SQLITE_ROW) {
			id = sqlite3_column_int64(m_stmt_get_id, 0);
		} else {
			id = 0;
		}
		sqlite3_reset(m_stmt_get_id);
	}

	sqlite3_stmt *stmt_do = id == 0 ? m_stmt_insert : m_stmt_update;

	if (id != 0) {
		SQLOK(sqlite3_bind_int64(stmt_do, 2, id),
			"Failed to bind block ID for save");
	}
	SQLOK(sqlite3_bind_blob(stmt_do, 1, data, data_len, NULL),
		"Failed to bind block data");
	SQLRES(sqlite3_step(stmt_do), SQLITE_DONE, "Failed to save block")
	sqlite3_reset(stmt_do);


	if (id == 0) {
		s64 id = sqlite3_last_insert_rowid(m_database);
		SQLOK(sqlite3_bind_int64(m_stmt_insert_pos, 1, id),
				"Failed to bind block ID for pos save");
		bindPos(m_stmt_insert_pos, pos, 2);
		SQLRES(sqlite3_step(m_stmt_insert_pos), SQLITE_DONE, "Failed to save block")
		sqlite3_reset(m_stmt_insert_pos);
	}

	return true;
}

void Database_SQLite3::loadBlock(const v3s16 &pos, std::string *block)
{
	bindPos(m_stmt_read, pos);

	if (sqlite3_step(m_stmt_read) != SQLITE_ROW) {
		sqlite3_reset(m_stmt_read);
		return;
	}

	const char *data = (const char *) sqlite3_column_blob(m_stmt_read, 0);
	size_t len = sqlite3_column_bytes(m_stmt_read, 0);

	*block = (data) ? std::string(data, len) : "";

	sqlite3_step(m_stmt_read);
	// We should never get more than 1 row, so ok to reset
	sqlite3_reset(m_stmt_read);
}

void Database_SQLite3::createDatabase(bool set_ver)
{
	assert(m_database); // Pre-condition
	if (set_ver) {
		std::string q = "PRAGMA user_version = " +
			std::to_string(LATEST_DB_VERSION) + ';';
		SQLOK(sqlite3_exec(m_database, q.c_str(), NULL, NULL, NULL),
			"Failed to initialize database version");
	}
	SQLOK(sqlite3_exec(m_database,
		"CREATE VIRTUAL TABLE `block_pos` USING `rtree_i32`\n"
		"	(`id`, `x1`, `x2`, `y1`, `y2`, `z1`, `z2`);\n"
		"CREATE TABLE IF NOT EXISTS `blocks` (\n"
		"	`id` INTEGER PRIMARY KEY,\n"
		"	`data` BLOB\n"
		");\n",
		NULL, NULL, NULL),
		"Failed to create database table");
}

void Database_SQLite3::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	while (sqlite3_step(m_stmt_list) == SQLITE_ROW) {
		v3s16 p;
		p.X = sqlite3_column_int(m_stmt_list, 0);
		p.Y = sqlite3_column_int(m_stmt_list, 1);
		p.Z = sqlite3_column_int(m_stmt_list, 2);
		dst.push_back(p);
	}
	sqlite3_reset(m_stmt_list);
}

Database_SQLite3::~Database_SQLite3()
{
	FINALIZE_STATEMENT(m_stmt_read)
	FINALIZE_STATEMENT(m_stmt_insert)
	FINALIZE_STATEMENT(m_stmt_update)
	FINALIZE_STATEMENT(m_stmt_insert_pos)
	FINALIZE_STATEMENT(m_stmt_list)
	FINALIZE_STATEMENT(m_stmt_get_id)
	FINALIZE_STATEMENT(m_stmt_delete)
	FINALIZE_STATEMENT(m_stmt_delete_pos)
	FINALIZE_STATEMENT(m_stmt_begin)
	FINALIZE_STATEMENT(m_stmt_end)

	SQLOK_ERRSTREAM(sqlite3_close(m_database), "Failed to close database");
}

