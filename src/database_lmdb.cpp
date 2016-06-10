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

#if USE_LMDB

#include <vector>
#include <string>
#include "irr_v3d.h"
#include "irrlichttypes.h"
#include "database.h"
#include "database_lmdb.h"
#include "settings.h"
#include "exceptions.h"
#include "lmdb.h"
#include "filesys.h"
#include <errno.h>

#define MAKEKEY(pos)                    \
	MDB_val key;                        \
	s64 key_ = getBlockAsInteger(pos);  \
	key.mv_size = sizeof(s64);          \
	key.mv_data = &key_;

//public:
Database_LMDB::Database_LMDB(const std::string &savedir, Settings &conf)
{
	int result;

	online = false;
	path = std::string (savedir) + DIR_DELIM + "map.mdb";
	configuration = &conf;

	mapSize = conf.exists("lmdb_mapsize") ? conf.getU64("lmdb_mapsize") :
		defaultMapSize;

	if ((result = mdb_env_create(&environment))) {
		except("LMDB: Unknown error occurred creating environment.");
	}

	if ((result = mdb_env_set_mapsize(environment, mapSize))) {
		// Most probably, this means the mapsize is invalid.
		if (mdb_env_set_mapsize(environment, defaultMapSize)) {
			closeDatabase();
			except("LMDB: Can't set mapsize.");
		}
	}

	// Flag justification:  NOMETASYNC allows a commit to occur with only
	// one flush.  The price is durability -- the most recently committed
	// transaction may be lost in the event of an error that prevents the
	// buffers from making it to disk.  Probably not a problem for us.
	// In no case will this result in a corrupted database, since LMDB
	// never overwrites live data.
	if ((result = mdb_env_open(environment, path.c_str(),
			MDB_NOMETASYNC|MDB_NOSUBDIR, 0600))) {
		closeDatabase();
		if ((result == MDB_VERSION_MISMATCH)) {
			except("LMDB: Library version doesn't match the one that "
				"created the environment.");
		}
		else if (result == MDB_INVALID) {
			except("LMDB: Environment headers corrupted.");
		}
		else if (result == ENOENT) {
			except("LMDB: World directory removed before LMDB could open "
				"database.");
		}
		else if (result == EACCES) {
			except("LMDB: Inappropriate permissions on LMDB database.");
		}
		else if (result == EAGAIN) {
			except("LMDB: Database locked by another process.");
		}
		else {
			except("LMDB: Unknown error in env_open.");
		}
	}

	makeTransaction(MDB_RDONLY);

	if ((result = mdb_dbi_open(transaction, NULL, MDB_INTEGERKEY,
			&database))) {
		closeDatabase();
		except("LMDB: Can't open initial database.");
	}
	online = true;
}


Database_LMDB::~Database_LMDB()
{
	closeDatabase();
}


void Database_LMDB::beginSave()
{
	mdb_txn_abort(transaction);
	makeTransaction(0);
}


void Database_LMDB::endSave()
{
	commit();
	makeTransaction(MDB_RDONLY);
}


bool Database_LMDB::saveBlock(const v3s16 &pos, const std::string &data)
{
	int result;
	MDB_val value;
	size_t temp_size = data.length();
	std::string temp_data = data;

	MAKEKEY(pos);

	value.mv_data = const_cast<char *>(temp_data.c_str());
	value.mv_size = temp_size;

	while ((result = mdb_put(transaction, database, &key, &value, 0))) {
		if (result == MDB_TXN_FULL) {
			commit();
			makeTransaction(0);
			continue;
		}
		else if (result == MDB_MAP_FULL) {
			commit();
			mapSize += pow(1024,2);
			mdb_env_set_mapsize(environment, mapSize);
			makeTransaction(0);
			continue;
		}
		else if (result == EACCES) {
			except("LMDB: Can't use saveBlock without beginSave.");
		}
		else if (result == EINVAL) {
			except("LMDB: Invalid parameter saving block.");
		}
		else {
			except("LMDB: Unknown error in saveBlock.");
		}
	}
	return true;
}


void Database_LMDB::loadBlock(const v3s16 &pos, std::string *block)
{
	int result;
	MDB_val value;

	MAKEKEY(pos);

	*block = "";
	if ((result = mdb_get(transaction, database, &key, &value))) {
		if (result == MDB_NOTFOUND) {
			return;
		}
		else if (result == EINVAL) {
			closeDatabase();
			except("LMDB: Invalid parameter loading block.");
		}
		else {
			closeDatabase();
			except("LMDB: Unknown error loading block.");
		}
	}
	*block = std::string((char *) value.mv_data, value.mv_size);
	return;
}


bool Database_LMDB::deleteBlock(const v3s16 &pos)
{
	int result;

	MAKEKEY(pos);

	if ((result = mdb_del(transaction, database, &key, NULL))) {
		if (result == MDB_NOTFOUND) {
			return false;
		}
		closeDatabase();
		if (result == EACCES) {
			except("LMDB: Attempt to delete block in read-only "
				"transaction.");
		}
		else if (result == EINVAL){
			except("LMDB: Invalid parameter in delete.");
		}
		else {
			except("LMDB: Unknown error in delete.");
		}
	}
	return true;
}


void Database_LMDB::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	int result;
	MDB_cursor *cursor;
	MDB_val key, value;
	MDB_stat stats;

	if (mdb_stat(transaction, database, &stats)) {
		except("LMDB: Cannot get stats for database.");
	}

	dst.reserve(stats.ms_entries);

	if ((result = mdb_cursor_open(transaction, database, &cursor))) {
		closeDatabase();
		if (result == EINVAL) {
			except("LMDB: Invalid parameter creating cursor.");
		}
		else {
			except("LMDB: Unknown error creating cursor.");
		}
	}

	result = mdb_cursor_get(cursor, &key, &value, MDB_FIRST);

	while (!result) {
		dst.push_back(getIntegerAsBlock(*(s64 *) key.mv_data));
		result = mdb_cursor_get(cursor, &key, &value, MDB_NEXT);
	}
	if (result == MDB_NOTFOUND) {
		mdb_cursor_close(cursor);
		return;
	}
	else if (result == EINVAL) {
		mdb_cursor_close(cursor);
		closeDatabase();
		except("LMDB: Invalid parameter in cursor get.");
	}
	else {
		mdb_cursor_close(cursor);
		closeDatabase();
		except("LMDB: Unknown error during cursor get.");
	}
}


bool Database_LMDB::initialized()
{
	return online;
}


//private:
void Database_LMDB::closeDatabase()
{
	online = false;
	// These are all void functions and no error conditions should occur.
	mdb_txn_abort(transaction);
	mdb_dbi_close(environment, database);
	mdb_env_close(environment);
}


void Database_LMDB::commit() {
	int result;

	if ((result = mdb_txn_commit(transaction))) {
		closeDatabase();
		if (result == EINVAL) {
			except("LMDB: Invalid parameter in commit.");
		}
		else if (result == ENOSPC) {
			except("LMDB: Disk full attempting to commit.");
		}
		else if (result == EIO) {
			except("LMDB: I/O error writing to database.");
		}
		else if (result == ENOMEM) {
			except("LMDB: Out of memory during commit.");
		}
		else {
			except("LMDB: Unknown error during commit.");
		}
	}
}


void Database_LMDB::except(std::string message){
	throw DatabaseException(message);
}


void Database_LMDB::makeTransaction(unsigned int flags)
{
	int result = 0;

	while ((result = mdb_txn_begin(environment, NULL, flags,
			&transaction))) {
		if (result == MDB_MAP_RESIZED) {
			if (mdb_env_set_mapsize(environment, 0)) {
				closeDatabase();
				except("LMDB: Failed to resize memmap.");
			}
			continue;
		}
		closeDatabase();
		if (result == MDB_PANIC) {
			except("LMDB: Fatal error occurred.");
		}
		else if (result == MDB_READERS_FULL) {
			except("LMDB: Too many reading threads.");
		}
		else if (result == ENOMEM) {
			except("LMDB: Failed to allocate memory.");
		}
		else {
			except("LMDB: Unknown error making transaction.");
		}
	}
}

#endif // USE_LMDB

/* Local Variables: */
/* indent-tabs-mode: t */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* End: */
