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
		infostream<<"WARNING: beginSave() failed, saving might be slow.";
}

void Database_SQLite3::endSave() {
	verifyDatabase();
	if(sqlite3_exec(m_database, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK)
		infostream<<"WARNING: endSave() failed, map might not have saved.";
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
	
	{
		std::string dbp = m_savedir + DIR_DELIM + "map.sqlite";
		bool needs_create = false;
		int d;
		
		/*
			Open the database connection
		*/
	
		createDirs(m_savedir); // ?
	
		if(!fs::PathExists(dbp))
			needs_create = true;
	
		d = sqlite3_open_v2(dbp.c_str(), &m_database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if(d != SQLITE_OK) {
			infostream<<"WARNING: SQLite3 database failed to open: "<<sqlite3_errmsg(m_database)<<std::endl;
			throw FileNotGoodException("Cannot open database file");
		}
		
		if(needs_create)
			createDatabase();

		std::string querystr = std::string("PRAGMA synchronous = ")
				 + itos(g_settings->getU16("sqlite_synchronous"));
		d = sqlite3_exec(m_database, querystr.c_str(), NULL, NULL, NULL);
		if(d != SQLITE_OK) {
			infostream<<"WARNING: Database pragma set failed: "
					<<sqlite3_errmsg(m_database)<<std::endl;
			throw FileNotGoodException("Cannot set pragma");
		}

		d = sqlite3_prepare(m_database, "SELECT `data` FROM `blocks` WHERE `pos`=? LIMIT 1", -1, &m_database_read, NULL);
		if(d != SQLITE_OK) {
			infostream<<"WARNING: SQLite3 database read statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
			throw FileNotGoodException("Cannot prepare read statement");
		}
		
		d = sqlite3_prepare(m_database, "REPLACE INTO `blocks` VALUES(?, ?)", -1, &m_database_write, NULL);
		if(d != SQLITE_OK) {
			infostream<<"WARNING: SQLite3 database write statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
			throw FileNotGoodException("Cannot prepare write statement");
		}
		
		d = sqlite3_prepare(m_database, "SELECT `pos` FROM `blocks`", -1, &m_database_list, NULL);
		if(d != SQLITE_OK) {
			infostream<<"WARNING: SQLite3 database list statment failed to prepare: "<<sqlite3_errmsg(m_database)<<std::endl;
			throw FileNotGoodException("Cannot prepare read statement");
		}
		
		infostream<<"ServerMap: SQLite3 database opened"<<std::endl;
	}
}

void Database_SQLite3::saveBlock(MapBlock *block)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Dummy blocks are not written
	*/
	if(block->isDummy())
	{
		/*v3s16 p = block->getPos();
		infostream<<"Database_SQLite3::saveBlock(): WARNING: Not writing dummy block "
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		return;
	}

	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST_WRITE;
	// Get destination
	v3s16 p3d = block->getPos();
	
	
#if 0
	v2s16 p2d(p3d.X, p3d.Z);
	std::string sectordir = getSectorDir(p2d);

	createDirs(sectordir);

	std::string fullpath = sectordir+DIR_DELIM+getBlockFilename(p3d);
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open block data");
#endif
	/*
		[0] u8 serialization version
		[1] data
	*/
	
	verifyDatabase();
	
	std::ostringstream o(std::ios_base::binary);
	
	o.write((char*)&version, 1);
	
	// Write basic data
	block->serialize(o, version, true);
	
	// Write block to database
	
	std::string tmp = o.str();
	const char *bytes = tmp.c_str();
	
	if(sqlite3_bind_int64(m_database_write, 1, getBlockAsInteger(p3d)) != SQLITE_OK)
		infostream<<"WARNING: Block position failed to bind: "<<sqlite3_errmsg(m_database)<<std::endl;
	if(sqlite3_bind_blob(m_database_write, 2, (void *)bytes, o.tellp(), NULL) != SQLITE_OK) // TODO this mught not be the right length
		infostream<<"WARNING: Block data failed to bind: "<<sqlite3_errmsg(m_database)<<std::endl;
	int written = sqlite3_step(m_database_write);
	if(written != SQLITE_DONE)
		infostream<<"WARNING: Block failed to save ("<<p3d.X<<", "<<p3d.Y<<", "<<p3d.Z<<") "
		<<sqlite3_errmsg(m_database)<<std::endl;
	// Make ready for later reuse
	sqlite3_reset(m_database_write);
	
	// We just wrote it to the disk so clear modified flag
	block->resetModified();
}

MapBlock* Database_SQLite3::loadBlock(v3s16 blockpos)
{
	v2s16 p2d(blockpos.X, blockpos.Z);
	verifyDatabase();

	if (sqlite3_bind_int64(m_database_read, 1, getBlockAsInteger(blockpos)) != SQLITE_OK) {
		infostream << "WARNING: Could not bind block position for load: "
			<< sqlite3_errmsg(m_database)<<std::endl;
	}

	if (sqlite3_step(m_database_read) == SQLITE_ROW) {
		/*
			Make sure sector is loaded
		*/
		MapSector *sector = srvmap->createSector(p2d);

		/*
			Load block
		*/
		const char *data = (const char *)sqlite3_column_blob(m_database_read, 0);
		size_t len = sqlite3_column_bytes(m_database_read, 0);
		if (data == NULL || len == 0) {
			errorstream << "Blank block data in database (data == NULL || len"
				" == 0) (" << blockpos.X << "," << blockpos.Y << ","
				<< blockpos.Z << ")" << std::endl;

			if (g_settings->getBool("ignore_world_load_errors")) {
				errorstream << "Ignoring block load error. Duck and cover! "
					<< "(ignore_world_load_errors)" << std::endl;
			} else {
				throw SerializationError("Blank block data in database");
			}
			return NULL;
		}

		std::string datastr(data, len);

		//srvmap->loadBlock(&datastr, blockpos, sector, false);

		try {
			std::istringstream is(datastr, std::ios_base::binary);

			u8 version = SER_FMT_VER_INVALID;
			is.read((char *)&version, 1);

			if (is.fail())
				throw SerializationError("ServerMap::loadBlock(): Failed"
					 " to read MapBlock version");

			MapBlock *block = NULL;
			bool created_new = false;
			block = sector->getBlockNoCreateNoEx(blockpos.Y);
			if (block == NULL)
			{
				block = sector->createBlankBlockNoInsert(blockpos.Y);
				created_new = true;
			}

			// Read basic data
			block->deSerialize(is, version, true);

			// If it's a new block, insert it to the map
			if (created_new)
				sector->insertBlock(block);

			/*
				Save blocks loaded in old format in new format
			*/
			//if(version < SER_FMT_VER_HIGHEST || save_after_load)
			// Only save if asked to; no need to update version
			//if(save_after_load)
			//     	saveBlock(block);

			// We just loaded it from, so it's up-to-date.
			block->resetModified();
		}
		catch (SerializationError &e)
		{
			errorstream << "Invalid block data in database"
				<< " (" << blockpos.X << "," << blockpos.Y << "," << blockpos.Z << ")"
				<< " (SerializationError): " << e.what() << std::endl;

			// TODO: Block should be marked as invalid in memory so that it is
			// not touched but the game can run

			if (g_settings->getBool("ignore_world_load_errors")) {
				errorstream << "Ignoring block load error. Duck and cover! "
					<< "(ignore_world_load_errors)" << std::endl;
			} else {
				throw SerializationError("Invalid block data in database");
				//assert(0);
			}
		}

		sqlite3_step(m_database_read);
		// We should never get more than 1 row, so ok to reset
		sqlite3_reset(m_database_read);

		return srvmap->getBlockNoCreateNoEx(blockpos);  // should not be using this here
	}
	sqlite3_reset(m_database_read);
	return NULL;
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
	if(e == SQLITE_ABORT)
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

Database_SQLite3::~Database_SQLite3()
{
	if(m_database_read)
		sqlite3_finalize(m_database_read);
	if(m_database_write)
		sqlite3_finalize(m_database_write);
	if(m_database)
		sqlite3_close(m_database);
}

