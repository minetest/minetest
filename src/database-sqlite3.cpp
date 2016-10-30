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
#include "settings.h"
#include "porting.h"
#include "util/string.h"
#include "threading/thread.h"
#include "threading/semaphore.h"

#include <cassert>

// When to print messages when the database is being held locked by another process
// Note: I've seen occasional delays of over 250ms while running minetestmapper.
#define BUSY_INFO_TRESHOLD	100	// Print first informational message after 100ms.
#define BUSY_WARNING_TRESHOLD	250	// Print warning message after 250ms. Lag is increased.
#define BUSY_ERROR_TRESHOLD	1000	// Print error message after 1000ms. Significant lag.
#define BUSY_FATAL_TRESHOLD	10000	// Allow SQLITE_BUSY to be returned, which will cause a minetest crash.
#define BUSY_ERROR_INTERVAL	2000	// Report again regularly while the situation lasts


#define SQLRES(s, r, m) \
	if ((s) != (r)) { \
		throw DatabaseException(std::string(m) + ": " +\
				sqlite3_errmsg(m_database)); \
	}
#define SQLOK(s, m) SQLRES(s, SQLITE_OK, m)

#define PREPARE_STATEMENT(name, query) \
	SQLOK(sqlite3_prepare_v2(m_database, query, -1, &m_stmt_##name, NULL),\
		"Failed to prepare query '" query "'")

#define SQLOK_ERRSTREAM(s, m)                           \
	if ((s) != SQLITE_OK) {                             \
		errorstream << (m) << ": "                      \
			<< sqlite3_errmsg(m_database) << std::endl; \
	}

#define FINALIZE_STATEMENT(statement)				\
	if (statement)						\
		SQLOK_ERRSTREAM(sqlite3_finalize(statement),	\
			"Failed to finalize " #statement)


class SQLite3CheckpointThread : public Thread
{
public:
	SQLite3CheckpointThread(const SQLite3 &db) :
		Thread("SQLite3-Checkpoint"),
		m_db(db)
		{}
	bool shutdown(int timeout);

protected:
	void *run();

private:
	SQLite3 m_db;
	Semaphore m_shutdown_request;
	Semaphore m_shutdown_acknowledge;
};


// ====================== Generic SQLite3 class implementation ========================


SQLite3::SQLite3(const std::string &db_path, Settings &conf) :
	m_database(NULL),
	m_database_path(db_path),
	m_synchronous(2),
	m_journal_mode("delete"),
	m_last_wal_backlog(0),
	m_stmt_begin(NULL),
	m_stmt_commit(NULL),
	m_walCheckpointThread(NULL)
{
	bool db_exists = databaseExists();
	bool set_world_mt;

	// Determine journalling mode to use
	set_world_mt = false;
	if (conf.getNoEx("sqlite_journal_mode", m_journal_mode)) {
		m_journal_mode = lowercase(m_journal_mode);
		if (m_journal_mode != "delete" && m_journal_mode != "truncate"
				&& m_journal_mode != "persist" && m_journal_mode != "wal") {
			throw DatabaseException("SQLite3: Invalid value for configuration"
					" parameter 'sqlite_journal_mode' in world.mt;"
					" expected: delete|truncate|persist|wal");
		}
	} else if (db_exists) {
		// Backward compatibility: don't change existing mode.
		errorstream << "**** Sqlite3: sqlite_journal_mode mode is not set in world.mt."
			<< std::endl
			<< "****          Please set it, and consider switching to WAL mode, which"
			<< " is much faster" << std::endl
			<< "****          (See minetest.conf.example for more information)"
			<< std::endl;
		m_journal_mode = "";
	} else if (g_settings->getNoEx("sqlite_journal_mode", m_journal_mode)) {
		m_journal_mode = lowercase(m_journal_mode);
		if (m_journal_mode != "delete" && m_journal_mode != "truncate"
				&& m_journal_mode != "persist" && m_journal_mode != "wal") {
			throw DatabaseException("SQLite3: Invalid value for configuration"
					" parameter 'sqlite_journal_mode' in minetest.conf;"
					" expected: delete|truncate|persist|wal");
		}
		set_world_mt = true;
	} else {
		set_world_mt = true;
	}
	if (set_world_mt) {
		if (!conf.set("sqlite_journal_mode", m_journal_mode)) {
			throw DatabaseException("SQLite3: Failed to set sqlite_journal_mode"
				" in world.mt");
		}
	}

	// Determine synchronous mode to use
	set_world_mt = false;
	if (conf.exists("sqlite_synchronous")) {
		if (!conf.getS16NoEx("sqlite_synchronous", m_synchronous)
				|| m_synchronous < -1 || m_synchronous > 3) {
			throw DatabaseException("SQLite3: Invalid value for sqlite_synchronous"
				" in world.mt; expected -1 .. 3");
		}
	} else if (g_settings->exists("sqlite_synchronous")) {
		if (!g_settings->getS16NoEx("sqlite_synchronous", m_synchronous)
				|| m_synchronous < -1 || m_synchronous > 3) {
			throw DatabaseException("SQLite3: Invalid value for sqlite_synchronous"
				" in minetest.conf; expected -1 .. 3");
		}
		set_world_mt = true;
	} else {
		set_world_mt = true;
	}
	if (set_world_mt) {
		if (!conf.setS16("sqlite_synchronous", m_synchronous)) {
			throw DatabaseException("SQLite3: Failed to set sqlite_synchronous"
				" in world.mt");
		}
	}
}


SQLite3::SQLite3(const SQLite3 &db) :
	m_database(NULL),
	m_database_path(db.m_database_path),
	m_synchronous(db.m_synchronous),
	m_journal_mode(db.m_journal_mode),
	m_last_wal_backlog(0),
	m_stmt_begin(NULL),
	m_stmt_commit(NULL),
	m_walCheckpointThread(NULL)
{
}


SQLite3::~SQLite3()
{
	closeDatabase();
}


void SQLite3::openDatabase()
{
	SQLOK(sqlite3_open_v2(m_database_path.c_str(), &m_database,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL),
		std::string("SQLite3: Failed to open SQLite3 database file ")
			+ m_database_path);

	PREPARE_STATEMENT(begin, "BEGIN");
	PREPARE_STATEMENT(commit, "COMMIT");
}


void SQLite3::closeDatabase()
{
	if (m_database) {
		if (m_walCheckpointThread) {
			stopWALCheckpointThread();
			checkpointWALFinal();
		}
		FINALIZE_STATEMENT(m_stmt_begin);
		FINALIZE_STATEMENT(m_stmt_commit);
		SQLOK_ERRSTREAM(sqlite3_close(m_database), "SQLite3: Failed to close database");
		m_database = NULL;
	}
}


void SQLite3::setAutoCheckpoint(bool enable)
{
	if (enable) {
		// 1000 is the default value.
		SQLOK_ERRSTREAM(sqlite3_wal_autocheckpoint(m_database, 1000),
			"SQLite3: Failed to set auto-checkpoint interval to 1000")
	} else {
		SQLOK_ERRSTREAM(sqlite3_wal_autocheckpoint(m_database, 0),
			"SQLite3: Failed to disable auto-checkpointing")
	}
}


void SQLite3::checkpointWALPassive()
{
	int total_wal_frames;
	int checkpointed_frames;
	SQLOK(sqlite3_wal_checkpoint_v2(m_database, NULL,
			SQLITE_CHECKPOINT_PASSIVE,
			&total_wal_frames, &checkpointed_frames),
		"SQLite3: failed to checkpoint database (Passive)");
	if (total_wal_frames >= 0 && checkpointed_frames >= 0) {
		int new_wal_backlog = total_wal_frames - checkpointed_frames;
		// Normally, sqlite checkpoints every 1000 pages.
		// If the backlog is growing, complain every time it has crossed
		// a multiple of 10000
		if (m_last_wal_backlog / 10000 < new_wal_backlog / 10000) {
			(new_wal_backlog >= 100000 ? errorstream : warningstream)
				<< "SQLite3: WAL backlog exceeds "
				<< (new_wal_backlog / 10000) * 10000
				<< " pages - is another process using the database ?"
				<< std::endl;
		}
		m_last_wal_backlog = new_wal_backlog;
	}
}


void SQLite3::checkpointWALForce()
{
	int rv = sqlite3_wal_checkpoint_v2(m_database, NULL,
		SQLITE_CHECKPOINT_RESTART, NULL, NULL);
	if (rv == SQLITE_BUSY) {
		throw DatabaseException("SQLite3: The database is in use by another process"
			" (checkpointing failed with SQLITE_BUSY)");
	} else {
		SQLOK(rv, "SQLite3: Failed to checkpoint database (Restart)");
	}
}


void SQLite3::checkpointWALFinal()
{
	// Try to checkpoint the entire WAL (usually done before exit), so that
	// all data is in the database file, and no blocks are left in the wal file.
	// Hopefully, this also causes the wal file to be removed.
	// SQLITE_CHECKPOINT_TRUNCATE is only supported since sqlite 3.8 (jan. 2015)
#ifdef SQLITE_CHECKPOINT_TRUNCATE
	int rv = sqlite3_wal_checkpoint_v2(m_database, NULL,
		SQLITE_CHECKPOINT_TRUNCATE, NULL, NULL);
#else
	int rv = sqlite3_wal_checkpoint_v2(m_database, NULL,
		SQLITE_CHECKPOINT_RESTART, NULL, NULL);
#endif
	if (rv == SQLITE_BUSY) {
		warningstream << "SQLite3: Failed to checkpoint database:"
			<< " the database is in use by another process"
			<< " (but all data is safe!)"
			<< std::endl;
	} else {
#ifdef SQLITE_CHECKPOINT_TRUNCATE
		SQLOK(rv, "SQLite3: Failed to checkpoint database (Truncate)")
#else
		SQLOK(rv, "SQLite3: Failed to checkpoint database (Restart)")
#endif
	}
}


int SQLite3::busyHandler(void *data, int count)
{
	BusyHandlerData *bhd = reinterpret_cast<BusyHandlerData *>(data);
	s64 cur_time = getTimeMs();

	if (count == 0) {
		bhd->first_time = cur_time;
		bhd->prev_time = bhd->first_time;
	} else {
		while (cur_time < bhd->prev_time)
			cur_time += s64(1)<<32;
	}

	if (cur_time - bhd->first_time < BUSY_INFO_TRESHOLD) {
		; // do nothing
	} else if (cur_time - bhd->first_time >= BUSY_INFO_TRESHOLD &&
			bhd->prev_time - bhd->first_time < BUSY_INFO_TRESHOLD) {
		infostream << "SQLite3 database has been locked for "
			<< cur_time - bhd->first_time << " ms." << std::endl;
	} else if (cur_time - bhd->first_time >= BUSY_WARNING_TRESHOLD &&
			bhd->prev_time - bhd->first_time < BUSY_WARNING_TRESHOLD) {
		warningstream << "SQLite3 database has been locked for "
			<< cur_time - bhd->first_time << " ms." << std::endl;
	} else if (cur_time - bhd->first_time >= BUSY_ERROR_TRESHOLD &&
			bhd->prev_time - bhd->first_time < BUSY_ERROR_TRESHOLD) {
		errorstream << "SQLite3 database has been locked for "
			<< cur_time - bhd->first_time << " ms; this causes lag." << std::endl;
	} else if (cur_time - bhd->first_time >= BUSY_FATAL_TRESHOLD &&
			bhd->prev_time - bhd->first_time < BUSY_FATAL_TRESHOLD) {
		errorstream << "SQLite3 database has been locked for "
			<< (cur_time - bhd->first_time) / 1000 << " seconds - giving up!" << std::endl;
	} else if ((cur_time - bhd->first_time) / BUSY_ERROR_INTERVAL !=
			(bhd->prev_time - bhd->first_time) / BUSY_ERROR_INTERVAL) {
		// Safety net: keep reporting every BUSY_ERROR_INTERVAL
		errorstream << "SQLite3 database has been locked for "
			<< (cur_time - bhd->first_time) / 1000 << " seconds!" << std::endl;
	}

	bhd->prev_time = cur_time;

	// Make sqlite transaction fail if delay exceeds BUSY_FATAL_TRESHOLD
	return cur_time - bhd->first_time < BUSY_FATAL_TRESHOLD;
}


void SQLite3::beginTransaction() {
	SQLRES(sqlite3_step(m_stmt_begin), SQLITE_DONE,
		"Failed to start SQLite3 transaction");
	sqlite3_reset(m_stmt_begin);
}


void SQLite3::commitTransaction() {
	SQLRES(sqlite3_step(m_stmt_commit), SQLITE_DONE,
		"Failed to commit SQLite3 transaction");
	sqlite3_reset(m_stmt_commit);
}


std::string SQLite3::getDBJournalMode()
{
	sqlite3_stmt *stmt_journal;

	SQLOK(sqlite3_prepare_v2(m_database, "PRAGMA journal_mode;",
			-1, &stmt_journal, NULL),
		"Failed to prepare get-journal-mode query")
	if (sqlite3_step(stmt_journal) != SQLITE_ROW)
		throw DatabaseException("SQLite3: Failed to get journal mode from database");
	std::string mode = (const char *) sqlite3_column_text(stmt_journal, 0);
	mode = lowercase(mode);
	SQLOK_ERRSTREAM(sqlite3_finalize(stmt_journal),
		"SQLite3: Failed to finalize get-journal-mode statement")
	return mode;
}


void SQLite3::applyJournalMode()
{
	sqlite3_stmt *stmt_journal;

	// Determine previous mode for user feedback
	std::string previous_mode = getDBJournalMode();
	if (m_journal_mode == "") {
		m_journal_mode = previous_mode;
		infostream << "SQLite3: sqlite_journal_mode is not set in world.mt."
			<< " Detected mode from database: " << previous_mode
			<< std::endl;
	}

	std::string new_mode = previous_mode;
	if (previous_mode != m_journal_mode) {
		std::string sql_code = "PRAGMA journal_mode = ";
		sql_code += m_journal_mode + ";";
		SQLOK(sqlite3_prepare_v2(m_database, sql_code.c_str(), -1, &stmt_journal, NULL),
			"SQLite3: Failed to prepare set-journal-mode query")
		if (sqlite3_step(stmt_journal) != SQLITE_ROW)
			throw DatabaseException("SQLite3: Failed to execute journal mode statement");
		new_mode = lowercase((const char *)sqlite3_column_text(stmt_journal, 0));
		SQLOK_ERRSTREAM(sqlite3_finalize(stmt_journal),
			"SQLite3: Failed to finalize set-journal-mode statement")

		if (new_mode != m_journal_mode) {
			warningstream << "SQLite3: failed to set journal mode '"
				<< m_journal_mode << "' on database. Mode is now: '"
				<< new_mode << "'" << std::endl;
		}
		if (previous_mode != new_mode) {
			infostream << "SQLite3: changed database journal mode from '"
				<< previous_mode << "' to '" << new_mode << "'"
				<< std::endl;
		}
		m_journal_mode = new_mode;
	} else {
		infostream << "SQLite3: database journal mode is '"
			<< previous_mode << "' (not changed)" << std::endl;
	}

	if (m_journal_mode == "wal") {
		SQLOK_ERRSTREAM(sqlite3_wal_autocheckpoint(m_database, 0),
			"Failed to disable auto-checkpointing for WAL mode");
	}
}


void SQLite3::applySynchronousLevel()
{
	if (m_synchronous == -1)
		m_synchronous = (m_journal_mode == "wal") ? 1 : 2;
	std::string query_str = std::string("PRAGMA synchronous = ")
			 + itos(m_synchronous);
	SQLOK(sqlite3_exec(m_database, query_str.c_str(), NULL, NULL, NULL),
		"SQLite3: Failed to modify sqlite3 synchronous level");
	infostream << "SQLite3: database synchronous level set to: "
		<< m_synchronous << std::endl;
}


void SQLite3::startWALCheckpointThread()
{
	if (!m_database)
		throw DatabaseException("SQLite3: Internal error: trying to start"
			" WAL thread, but database is not yet open");

	if (m_journal_mode != "wal" || getDBJournalMode() != "wal")
		throw DatabaseException("SQLite3: Internal error: trying to start"
			" WAL thread, but journal mode is not 'wal'");

	if (m_walCheckpointThread)
		throw DatabaseException("SQLite3: Internal error: trying to start"
			" WAL thread, but it is already running");

	setAutoCheckpoint(false);
	m_walCheckpointThread = new SQLite3CheckpointThread(*this);
	m_walCheckpointThread->start();
}

void SQLite3::stopWALCheckpointThread()
{
	if (m_walCheckpointThread) {
		if (!m_walCheckpointThread->shutdown(5000)) {
			errorstream << "SQLite3: WAL thread fails to terminate - killing it"
				<< std::endl;
			m_walCheckpointThread->stop();
		}
		m_walCheckpointThread->wait();
		delete m_walCheckpointThread;
		m_walCheckpointThread = NULL;
	}
}


void SQLite3::enableBusyHandler(void)
{
	SQLOK(sqlite3_busy_handler(m_database, SQLite3::busyHandler,
		&m_busy_handler_data), "SQLite3: Failed to set busy handler");
}


// ====================== SQLite3 WAL-sync class implementation ========================


bool SQLite3CheckpointThread::shutdown(int timeout)
{
	m_shutdown_request.post();
	return m_shutdown_acknowledge.wait(timeout);
}


void *SQLite3CheckpointThread::run()
{
	m_db.openDatabase();
	m_db.applySynchronousLevel();

	int checkpoint_interval = g_settings->getU16("sqlite_wal_checkpoint_interval");
	if (checkpoint_interval < 1) {
		errorstream <<  "Invalid value for sqlite_wal_checkpoint_interval ("
			<< checkpoint_interval << ") - minimum value is 1" << std::endl;
		g_settings->setU16("sqlite_wal_checkpoint_interval", 1);
		checkpoint_interval = 1;
	}
	checkpoint_interval *= 1000;

	infostream << "SQLite3; WAL checkpoint thread active" << std::endl;

	do {
		m_db.checkpointWALPassive();
	} while (!m_shutdown_request.wait(checkpoint_interval));

	m_db.closeDatabase();
	infostream << "SQLite3: WAL checkpoint thread finished" << std::endl;
	m_shutdown_acknowledge.post();

	return NULL;
}


// ====================== SQLite3 minetest database class implementation ========================


Database_SQLite3::Database_SQLite3(const std::string &savedir, Settings &conf) :
	SQLite3(savedir + DIR_DELIM + "map.sqlite", conf),
	m_initialized(false),
	m_savedir(savedir),
	m_stmt_read(NULL),
	m_stmt_write(NULL),
	m_stmt_list(NULL),
	m_stmt_delete(NULL)
{
}


void Database_SQLite3::initDatabase()
{
	if (m_database) return;

	// Open the database connection

	if (!fs::CreateAllDirs(m_savedir)) {
		infostream << "Database_SQLite3: Failed to create directory \""
			<< m_savedir << "\"" << std::endl;
		throw FileNotGoodException("Failed to create database "
				"save directory");
	}

	bool needs_create = !databaseExists();

	openDatabase();
	enableBusyHandler();

	// Journal mode must be set before setting synchronous operation, as
	// the effective journal mode is not yet known
	applyJournalMode();
	applySynchronousLevel();

	if (getJournalMode() == "wal") {
		setAutoCheckpoint(false);
		checkpointWALForce();
		// The ability to disable the checkpoint thread is intended for
		// benchmarking and testing only. Therefore this configuration
		// setting is not officially documented.
		if (!g_settings->getFlag("disable_wal_checkpoint_thread"))
			startWALCheckpointThread();
	}

	if (needs_create)
		createDatabase();
}

void Database_SQLite3::verifyDatabase()
{
	if (m_initialized) return;

	initDatabase();

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
	SQLOK(sqlite3_bind_int64(stmt, index, getBlockAsInteger(pos)),
		"Internal error: failed to bind query at " __FILE__ ":" TOSTRING(__LINE__));
}

bool Database_SQLite3::deleteBlock(const v3s16 &pos)
{
	verifyDatabase();

	bindPos(m_stmt_delete, pos);

	bool good = sqlite3_step(m_stmt_delete) == SQLITE_DONE;
	sqlite3_reset(m_stmt_delete);

	if (!good) {
		warningstream << "deleteBlock: Block failed to delete "
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
	SQLOK(sqlite3_bind_blob(m_stmt_write, 2, data.data(), data.size(), NULL),
		"Internal error: failed to bind query at " __FILE__ ":" TOSTRING(__LINE__));

	SQLRES(sqlite3_step(m_stmt_write), SQLITE_DONE, "Failed to save block")
	sqlite3_reset(m_stmt_write);

	return true;
}

void Database_SQLite3::loadBlock(const v3s16 &pos, std::string *block)
{
	verifyDatabase();

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

void Database_SQLite3::createDatabase()
{
	assert(m_database); // Pre-condition
	SQLOK(sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `blocks` (\n"
		"	`pos` INT PRIMARY KEY,\n"
		"	`data` BLOB\n"
		");\n",
		NULL, NULL, NULL),
		"Failed to create database table");
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
	FINALIZE_STATEMENT(m_stmt_delete)
}

