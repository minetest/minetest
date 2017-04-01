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
#include "filesys.h"
#include <string>
#include "settings.h"

extern "C" {
	#include "sqlite3.h"
}

class SQLite3CheckpointThread;

// SQLite3 code that is not specific to the map database interface expected by minetest.
class SQLite3
{
public:
	SQLite3(const std::string &db_path, Settings &conf);
	SQLite3(const SQLite3 &db);
	virtual ~SQLite3();

	bool databaseExists() { return fs::PathExists(m_database_path); }
	void openDatabase();
	void closeDatabase();
	void applySynchronousLevel();	// Per connection setting
	std::string getJournalMode() const { return m_journal_mode; };
	void applyJournalMode();		// Database-wide setting
	void enableBusyHandler();	// Per connection setting
	void checkpointWALForce();	// Fail if WAL can't be restarted
	void checkpointWALFinal();	// Try to truncate / restart, but don't fail
	void checkpointWALPassive();	// checkpoint as much as possible;
					// complain if backlog is growing
	void startWALCheckpointThread();
	void stopWALCheckpointThread();
	void setAutoCheckpoint(bool enable = true);
	void beginTransaction();
	void commitTransaction();

	// This assumes there is at most one database writer instance.
	bool WALCheckpointThreadEnabled() { return m_walCheckpointThread; }

protected:
	std::string getDBJournalMode();

	// making this protected allows subclasses to prepare statements
	sqlite3 *m_database;

private:
	struct BusyHandlerData
	{
		s64 first_time;
		s64 prev_time;
	};
	BusyHandlerData m_busy_handler_data;
	static int busyHandler(void *data, int count);

	std::string m_database_path;
	s16 m_synchronous;
	std::string m_journal_mode;
	int m_last_wal_backlog;

	sqlite3_stmt *m_stmt_begin;
	sqlite3_stmt *m_stmt_commit;

	SQLite3CheckpointThread *m_walCheckpointThread;
};


class Database_SQLite3 : public Database, protected SQLite3
{
public:
	Database_SQLite3(const std::string &savedir, Settings &conf);
	~Database_SQLite3();

	void beginSave() { verifyDatabase(); beginTransaction(); }
	void endSave()
	{
		verifyDatabase();
		commitTransaction();
		if (!WALCheckpointThreadEnabled())
			checkpointWALPassive();
	}

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	bool initialized() const { return m_initialized; }

private:
	// Initialize the database
	void initDatabase();
	// Create the database structure
	void createDatabase();
	// Open and initialize the database if needed
	void verifyDatabase();

	void bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int index=1);

	bool m_initialized;

	std::string m_savedir;

	sqlite3_stmt *m_stmt_read;
	sqlite3_stmt *m_stmt_write;
	sqlite3_stmt *m_stmt_list;
	sqlite3_stmt *m_stmt_delete;
};

#endif

