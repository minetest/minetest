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

#ifndef DATABASE_LMDB_HEADER
#define DATABASE_LMDB_HEADER

#include <vector>
#include <string>
#include "irr_v3d.h"
#include "irrlichttypes.h"
#include "database.h"
#include "settings.h"
#include "lmdb.h"

class Database_LMDB : public Database
{
public:
	Database_LMDB(const std::string &savedir, Settings &conf);
	~Database_LMDB();

	void beginSave();
	void endSave();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);

	void listAllLoadableBlocks(std::vector<v3s16> &dst);

	bool initialized();
private:
	MDB_env *environment;
	MDB_txn *transaction;
	MDB_dbi database;

	const static size_t defaultMapSize = 2147483648;
	size_t mapSize;

	std::string path;
	Settings *configuration;

	bool online;

	void closeDatabase();
	void commit();
	void except(std::string message);

	void makeTransaction(unsigned int flags);
};

#endif


/* Local Variables: */
/* indent-tabs-mode: t */
/* tab-width: 4 */
/* c-basic-offset: 4 */
/* End: */
