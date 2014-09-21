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

#include "config.h"

#if USE_LEVELDB

#include "database-leveldb.h"
#include "leveldb/db.h"

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "serialization.h"
#include "main.h"
#include "settings.h"
#include "log.h"
#include "filesys.h"

#define ENSURE_STATUS_OK(s) \
	if (!(s).ok()) { \
		throw FileNotGoodException(std::string("LevelDB error: ") + (s).ToString()); \
	}

Database_LevelDB::Database_LevelDB(ServerMap *map, std::string savedir)
{
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, savedir + DIR_DELIM + "map.db", &m_database);
	ENSURE_STATUS_OK(status);
	srvmap = map;
}

int Database_LevelDB::Initialized(void)
{
	return 1;
}

void Database_LevelDB::beginSave() {}
void Database_LevelDB::endSave() {}

bool Database_LevelDB::saveBlock(v3s16 blockpos, std::string &data)
{
	leveldb::Status status = m_database->Put(leveldb::WriteOptions(),
			i64tos(getBlockAsInteger(blockpos)), data);
	if (!status.ok()) {
		errorstream << "WARNING: saveBlock: LevelDB error saving block "
			<< PP(blockpos) << ": " << status.ToString() << std::endl;
		return false;
	}

	return true;
}

std::string Database_LevelDB::loadBlock(v3s16 blockpos)
{
	std::string datastr;
	leveldb::Status status = m_database->Get(leveldb::ReadOptions(),
		i64tos(getBlockAsInteger(blockpos)), &datastr);

	if(status.ok())
		return datastr;
	else
		return "";
}

void Database_LevelDB::listAllLoadableBlocks(std::list<v3s16> &dst)
{
	leveldb::Iterator* it = m_database->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		dst.push_back(getIntegerAsBlock(stoi64(it->key().ToString())));
	}
	ENSURE_STATUS_OK(it->status());  // Check for any errors found during the scan
	delete it;
}

Database_LevelDB::~Database_LevelDB()
{
	delete m_database;
}
#endif
