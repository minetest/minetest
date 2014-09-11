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
	- Initially only replaces sectors/ and sectors2/

	If map.sqlite does not exist in the save dir
	or the block was not found in the database
	the map will try to load from sectors folder.
	In either case, map.sqlite will be created
	and all future saves will save there.

	Structure of map.sqlite:
	Tables:
		blocks
			(PK) INT pos
			BLOB data
*/


#include "database-sqlite3.h"

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "serialization.h"
#include "main.h"
#include "settings.h"
#include "log.h"
#include "filesys.h"

Database_SQLite3::Database_SQLite3(ServerMap *map, std::string savedir)
{
	m_database = NULL;
	m_database_read = NULL;
	m_database_write = NULL;
	m_database_list = NULL;
	m_savedir = savedir;
	srvmap = map;
}

int Database_SQLite3::Initialized(void)
{
	return m_database ? 1 : 0;
}

void Database_SQLite3::beginSave() {
	verifyDatabase();
	if(sqlite3_exec(m_database, "BEGIN;", NULL, NULL, NULL) != SQLITE_OK)
		errorstream<<"WARNING: beginSave() failed, saving might be slow.";
}

void Database_SQLite3::endSave() {
	verifyDatabase();
	if(sqlite3_exec(m_database, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK)
		errorstream<<"WARNING: endSave() failed, map might not have saved.";
}

void Database_SQLite3::createDirs(std::string path)
{
	if(fs::CreateAllDirs(path) == false)
	{
		infostream<<DTIME<<"Database_SQLite3: Failed to create directory "
				<<"\""<<path<<"\""<<std::endl;
		throw BaseException("Database_SQLite3 failed to create directory");
	}
}

void Database_SQLite3::verifyDatabase() {
	if(m_database)
		return;

	std::string dbp = m_savedir + DIR_DELIM "map.sqlite";
	bool needs_create = false;
	int d;

	// Open the database connection

	createDirs(m_savedir); // ?

	if(!fs::PathExists(dbp))
		needs_create = true;

	d = sqlite3_open_v2(dbp.c_str(), &m_database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if(d != SQLITE_OK) {
		errorstream<<"SQLite3 database failed to open: "<<sqlite3_errmsg(m_database)<<std::endl;
		throw FileNotGoodException("Cannot open database file");
	}

	if(needs_create)
		createDatabase();

	std::string querystr = std::string("PRAGMA synchronous = ")
			 + itos(g_settings->getU16("sqlite_synchronous"));
	d = sqlite3_exec(m_database, querystr.c_str(), NULL, NULL, NULL);
	if(d != SQLITE_OK) {
		errorstream<<"Database pragma set failed: "
				<<sqlite3_errmsg(m_database)<<std::endl;
		throw FileNotGoodException("Cannot set pragma");
	}

	d = sqlite3_prepare(m_database, "SELECT `data` FROM `blocks` WHERE `pos`=? LIMIT 1", -1, &m_database_read, NULL);
	if(d != SQLITE_OK) {
		errorstream<<"SQLite3 read statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
		throw FileNotGoodException("Cannot prepare read statement");
	}
#ifdef __ANDROID__
	d = sqlite3_prepare(m_database, "INSERT INTO `blocks` VALUES(?, ?);", -1, &m_database_write, NULL);
#else
	d = sqlite3_prepare(m_database, "REPLACE INTO `blocks` VALUES(?, ?);", -1, &m_database_write, NULL);
#endif
	if(d != SQLITE_OK) {
		errorstream<<"SQLite3 write statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
		throw FileNotGoodException("Cannot prepare write statement");
	}

#ifdef __ANDROID__
	d = sqlite3_prepare(m_database, "DELETE FROM `blocks` WHERE `pos`=?;", -1, &m_database_delete, NULL);
	if(d != SQLITE_OK) {
		infostream<<"WARNING: SQLite3 database delete statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
		throw FileNotGoodException("Cannot prepare delete statement");
	}
#endif

	d = sqlite3_prepare(m_database, "SELECT `pos` FROM `blocks`", -1, &m_database_list, NULL);
	if(d != SQLITE_OK) {
		infostream<<"SQLite3 list statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
		throw FileNotGoodException("Cannot prepare read statement");
	}

	infostream<<"ServerMap: SQLite3 database opened"<<std::endl;
}

bool Database_SQLite3::saveBlock(v3s16 blockpos, std::string &data)
{
	verifyDatabase();

#ifdef __ANDROID__
	/**
	 * Note: For some unknown reason sqlite3 fails to REPLACE blocks on android,
	 * deleting them and inserting first works.
	 */
	if (sqlite3_bind_int64(m_database_read, 1, getBlockAsInteger(blockpos)) != SQLITE_OK) {
		infostream << "WARNING: Could not bind block position for load: "
			<< sqlite3_errmsg(m_database)<<std::endl;
	}

	if (sqlite3_step(m_database_read) == SQLITE_ROW) {
		if (sqlite3_bind_int64(m_database_delete, 1, getBlockAsInteger(blockpos)) != SQLITE_OK) {
			infostream << "WARNING: Could not bind block position for delete: "
				<< sqlite3_errmsg(m_database)<<std::endl;
		}

		if (sqlite3_step(m_database_delete) != SQLITE_DONE) {
			errorstream << "WARNING: saveBlock: Block failed to delete "
				<< PP(blockpos) << ": " << sqlite3_errmsg(m_database) << std::endl;
			return false;
		}
		sqlite3_reset(m_database_delete);
	}
	sqlite3_reset(m_database_read);
#endif

	if (sqlite3_bind_int64(m_database_write, 1, getBlockAsInteger(blockpos)) != SQLITE_OK) {
		errorstream << "WARNING: saveBlock: Block position failed to bind: "
			<< PP(blockpos) << ": " << sqlite3_errmsg(m_database) << std::endl;
		sqlite3_reset(m_database_write);
		return false;
	}

	if (sqlite3_bind_blob(m_database_write, 2, (void *) data.c_str(), data.size(), NULL) != SQLITE_OK) {
		errorstream << "WARNING: saveBlock: Block data failed to bind: "
			<< PP(blockpos) << ": " << sqlite3_errmsg(m_database) << std::endl;
		sqlite3_reset(m_database_write);
		return false;
	}

	if (sqlite3_step(m_database_write) != SQLITE_DONE) {
		errorstream << "WARNING: saveBlock: Block failed to save "
			<< PP(blockpos) << ": " << sqlite3_errmsg(m_database) << std::endl;
		sqlite3_reset(m_database_write);
		return false;
	}

	sqlite3_reset(m_database_write);

	return true;
}

std::string Database_SQLite3::loadBlock(v3s16 blockpos)
{
	verifyDatabase();

	if (sqlite3_bind_int64(m_database_read, 1, getBlockAsInteger(blockpos)) != SQLITE_OK) {
		errorstream << "Could not bind block position for load: "
			<< sqlite3_errmsg(m_database)<<std::endl;
	}

	if (sqlite3_step(m_database_read) == SQLITE_ROW) {
		const char *data = (const char *) sqlite3_column_blob(m_database_read, 0);
		size_t len = sqlite3_column_bytes(m_database_read, 0);

		std::string s = "";
		if(data)
			s = std::string(data, len);

		sqlite3_step(m_database_read);
		// We should never get more than 1 row, so ok to reset
		sqlite3_reset(m_database_read);

		return s;
	}

	sqlite3_reset(m_database_read);
	return "";
}

void Database_SQLite3::createDatabase()
{
	int e;
	assert(m_database);
	e = sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `blocks` ("
			"`pos` INT NOT NULL PRIMARY KEY,"
			"`data` BLOB"
		");"
	, NULL, NULL, NULL);
	if(e != SQLITE_OK)
		throw FileNotGoodException("Could not create sqlite3 database structure");
	else
		infostream<<"ServerMap: SQLite3 database structure was created";

}

void Database_SQLite3::listAllLoadableBlocks(std::list<v3s16> &dst)
{
	verifyDatabase();

	while(sqlite3_step(m_database_list) == SQLITE_ROW)
	{
		sqlite3_int64 block_i = sqlite3_column_int64(m_database_list, 0);
		v3s16 p = getIntegerAsBlock(block_i);
		//dstream<<"block_i="<<block_i<<" p="<<PP(p)<<std::endl;
		dst.push_back(p);
	}
}


#define FINALIZE_STATEMENT(statement)                                          \
	if ( statement )                                                           \
		rc = sqlite3_finalize(statement);                                      \
	if ( rc != SQLITE_OK )                                                     \
		errorstream << "Database_SQLite3::~Database_SQLite3():"                \
			<< "Failed to finalize: " << #statement << ": rc=" << rc << std::endl;

Database_SQLite3::~Database_SQLite3()
{
	int rc = SQLITE_OK;

	FINALIZE_STATEMENT(m_database_read)
	FINALIZE_STATEMENT(m_database_write)
	FINALIZE_STATEMENT(m_database_list)

	if(m_database)
		rc = sqlite3_close(m_database);

	if (rc != SQLITE_OK) {
		errorstream << "Database_SQLite3::~Database_SQLite3(): "
				<< "Failed to close database: rc=" << rc << std::endl;
	}
}
