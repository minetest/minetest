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
#include "content_sao.h"
#include "remoteplayer.h"

#include <cassert>

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

#define PREPARE_STATEMENT(name, query) \
	SQLOK(sqlite3_prepare_v2(m_database, query, -1, &m_stmt_##name, NULL),\
		"Failed to prepare query '" query "'")

#define SQLOK_ERRSTREAM(s, m)                           \
	if ((s) != SQLITE_OK) {                             \
		errorstream << (m) << ": "                      \
			<< sqlite3_errmsg(m_database) << std::endl; \
	}

#define FINALIZE_STATEMENT(statement) SQLOK_ERRSTREAM(sqlite3_finalize(statement), \
	"Failed to finalize " #statement)

int Database_SQLite3::busyHandler(void *data, int count)
{
	s64 &first_time = reinterpret_cast<s64 *>(data)[0];
	s64 &prev_time = reinterpret_cast<s64 *>(data)[1];
	s64 cur_time = porting::getTimeMs();

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


Database_SQLite3::Database_SQLite3(const std::string &savedir, const std::string &dbname) :
	m_savedir(savedir),
	m_dbname(dbname)
{
}

void Database_SQLite3::beginSave()
{
	verifyDatabase();
	SQLRES(sqlite3_step(m_stmt_begin), SQLITE_DONE,
		"Failed to start SQLite3 transaction");
	sqlite3_reset(m_stmt_begin);
}

void Database_SQLite3::endSave()
{
	verifyDatabase();
	SQLRES(sqlite3_step(m_stmt_end), SQLITE_DONE,
		"Failed to commit SQLite3 transaction");
	sqlite3_reset(m_stmt_end);
}

void Database_SQLite3::openDatabase()
{
	if (m_database) return;

	std::string dbp = m_savedir + DIR_DELIM + m_dbname + ".sqlite";

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

	if (needs_create) {
		createDatabase();
	}

	std::string query_str = std::string("PRAGMA synchronous = ")
			 + itos(g_settings->getU16("sqlite_synchronous"));
	SQLOK(sqlite3_exec(m_database, query_str.c_str(), NULL, NULL, NULL),
		"Failed to modify sqlite3 synchronous mode");
	SQLOK(sqlite3_exec(m_database, "PRAGMA foreign_keys = ON", NULL, NULL, NULL),
		"Failed to enable sqlite3 foreign key support");
}

void Database_SQLite3::verifyDatabase()
{
	if (m_initialized) return;

	openDatabase();

	PREPARE_STATEMENT(begin, "BEGIN;");
	PREPARE_STATEMENT(end, "COMMIT;");

	initStatements();

	m_initialized = true;
}

Database_SQLite3::~Database_SQLite3()
{
	FINALIZE_STATEMENT(m_stmt_begin)
	FINALIZE_STATEMENT(m_stmt_end)

	SQLOK_ERRSTREAM(sqlite3_close(m_database), "Failed to close database");
}

/*
 * Map database
 */

MapDatabaseSQLite3::MapDatabaseSQLite3(const std::string &savedir):
	Database_SQLite3(savedir, "map"),
	MapDatabase()
{
}

MapDatabaseSQLite3::~MapDatabaseSQLite3()
{
	FINALIZE_STATEMENT(m_stmt_read)
	FINALIZE_STATEMENT(m_stmt_write)
	FINALIZE_STATEMENT(m_stmt_list)
	FINALIZE_STATEMENT(m_stmt_delete)
}


void MapDatabaseSQLite3::createDatabase()
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

void MapDatabaseSQLite3::initStatements()
{
	PREPARE_STATEMENT(read, "SELECT `data` FROM `blocks` WHERE `pos` = ? LIMIT 1");
#ifdef __ANDROID__
	PREPARE_STATEMENT(write,  "INSERT INTO `blocks` (`pos`, `data`) VALUES (?, ?)");
#else
	PREPARE_STATEMENT(write, "REPLACE INTO `blocks` (`pos`, `data`) VALUES (?, ?)");
#endif
	PREPARE_STATEMENT(delete, "DELETE FROM `blocks` WHERE `pos` = ?");
	PREPARE_STATEMENT(list, "SELECT `pos` FROM `blocks`");

	verbosestream << "ServerMap: SQLite3 database opened." << std::endl;
}

inline void MapDatabaseSQLite3::bindPos(sqlite3_stmt *stmt, const v3s16 &pos, int index)
{
	SQLOK(sqlite3_bind_int64(stmt, index, getBlockAsInteger(pos)),
		"Internal error: failed to bind query at " __FILE__ ":" TOSTRING(__LINE__));
}

bool MapDatabaseSQLite3::deleteBlock(const v3s16 &pos)
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

bool MapDatabaseSQLite3::saveBlock(const v3s16 &pos, const std::string &data)
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

void MapDatabaseSQLite3::loadBlock(const v3s16 &pos, std::string *block)
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

void MapDatabaseSQLite3::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	verifyDatabase();

	while (sqlite3_step(m_stmt_list) == SQLITE_ROW)
		dst.push_back(getIntegerAsBlock(sqlite3_column_int64(m_stmt_list, 0)));

	sqlite3_reset(m_stmt_list);
}

/*
 * Player Database
 */

PlayerDatabaseSQLite3::PlayerDatabaseSQLite3(const std::string &savedir):
	Database_SQLite3(savedir, "players"),
	PlayerDatabase()
{
}

PlayerDatabaseSQLite3::~PlayerDatabaseSQLite3()
{
	FINALIZE_STATEMENT(m_stmt_player_load)
	FINALIZE_STATEMENT(m_stmt_player_add)
	FINALIZE_STATEMENT(m_stmt_player_update)
	FINALIZE_STATEMENT(m_stmt_player_remove)
	FINALIZE_STATEMENT(m_stmt_player_list)
	FINALIZE_STATEMENT(m_stmt_player_add_inventory)
	FINALIZE_STATEMENT(m_stmt_player_add_inventory_items)
	FINALIZE_STATEMENT(m_stmt_player_remove_inventory)
	FINALIZE_STATEMENT(m_stmt_player_remove_inventory_items)
	FINALIZE_STATEMENT(m_stmt_player_load_inventory)
	FINALIZE_STATEMENT(m_stmt_player_load_inventory_items)
	FINALIZE_STATEMENT(m_stmt_player_metadata_load)
	FINALIZE_STATEMENT(m_stmt_player_metadata_add)
	FINALIZE_STATEMENT(m_stmt_player_metadata_remove)
};


void PlayerDatabaseSQLite3::createDatabase()
{
	assert(m_database); // Pre-condition

	SQLOK(sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `player` ("
			"`name` VARCHAR(50) NOT NULL,"
			"`pitch` NUMERIC(11, 4) NOT NULL,"
			"`yaw` NUMERIC(11, 4) NOT NULL,"
			"`posX` NUMERIC(11, 4) NOT NULL,"
			"`posY` NUMERIC(11, 4) NOT NULL,"
			"`posZ` NUMERIC(11, 4) NOT NULL,"
			"`hp` INT NOT NULL,"
			"`breath` INT NOT NULL,"
			"`creation_date` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
			"`modification_date` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
			"PRIMARY KEY (`name`));",
		NULL, NULL, NULL),
		"Failed to create player table");

	SQLOK(sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `player_metadata` ("
			"    `player` VARCHAR(50) NOT NULL,"
			"    `metadata` VARCHAR(256) NOT NULL,"
			"    `value` TEXT,"
			"    PRIMARY KEY(`player`, `metadata`),"
			"    FOREIGN KEY (`player`) REFERENCES player (`name`) ON DELETE CASCADE );",
		NULL, NULL, NULL),
		"Failed to create player metadata table");

	SQLOK(sqlite3_exec(m_database,
		"CREATE TABLE IF NOT EXISTS `player_inventories` ("
			"   `player` VARCHAR(50) NOT NULL,"
			"	`inv_id` INT NOT NULL,"
			"	`inv_width` INT NOT NULL,"
			"	`inv_name` TEXT NOT NULL DEFAULT '',"
			"	`inv_size` INT NOT NULL,"
			"	PRIMARY KEY(player, inv_id),"
			"   FOREIGN KEY (`player`) REFERENCES player (`name`) ON DELETE CASCADE );",
		NULL, NULL, NULL),
		"Failed to create player inventory table");

	SQLOK(sqlite3_exec(m_database,
		"CREATE TABLE `player_inventory_items` ("
			"   `player` VARCHAR(50) NOT NULL,"
			"	`inv_id` INT NOT NULL,"
			"	`slot_id` INT NOT NULL,"
			"	`item` TEXT NOT NULL DEFAULT '',"
			"	PRIMARY KEY(player, inv_id, slot_id),"
			"   FOREIGN KEY (`player`) REFERENCES player (`name`) ON DELETE CASCADE );",
		NULL, NULL, NULL),
		"Failed to create player inventory items table");
}

void PlayerDatabaseSQLite3::initStatements()
{
	PREPARE_STATEMENT(player_load, "SELECT `pitch`, `yaw`, `posX`, `posY`, `posZ`, `hp`, "
		"`breath`"
		"FROM `player` WHERE `name` = ?")
	PREPARE_STATEMENT(player_add, "INSERT INTO `player` (`name`, `pitch`, `yaw`, `posX`, "
		"`posY`, `posZ`, `hp`, `breath`) VALUES (?, ?, ?, ?, ?, ?, ?, ?)")
	PREPARE_STATEMENT(player_update, "UPDATE `player` SET `pitch` = ?, `yaw` = ?, "
			"`posX` = ?, `posY` = ?, `posZ` = ?, `hp` = ?, `breath` = ?, "
			"`modification_date` = CURRENT_TIMESTAMP WHERE `name` = ?")
	PREPARE_STATEMENT(player_remove, "DELETE FROM `player` WHERE `name` = ?")
	PREPARE_STATEMENT(player_list, "SELECT `name` FROM `player`")

	PREPARE_STATEMENT(player_add_inventory, "INSERT INTO `player_inventories` "
		"(`player`, `inv_id`, `inv_width`, `inv_name`, `inv_size`) VALUES (?, ?, ?, ?, ?)")
	PREPARE_STATEMENT(player_add_inventory_items, "INSERT INTO `player_inventory_items` "
		"(`player`, `inv_id`, `slot_id`, `item`) VALUES (?, ?, ?, ?)")
	PREPARE_STATEMENT(player_remove_inventory, "DELETE FROM `player_inventories` "
		"WHERE `player` = ?")
	PREPARE_STATEMENT(player_remove_inventory_items, "DELETE FROM `player_inventory_items` "
		"WHERE `player` = ?")
	PREPARE_STATEMENT(player_load_inventory, "SELECT `inv_id`, `inv_width`, `inv_name`, "
		"`inv_size` FROM `player_inventories` WHERE `player` = ? ORDER BY inv_id")
	PREPARE_STATEMENT(player_load_inventory_items, "SELECT `slot_id`, `item` "
		"FROM `player_inventory_items` WHERE `player` = ? AND `inv_id` = ?")

	PREPARE_STATEMENT(player_metadata_load, "SELECT `metadata`, `value` FROM "
		"`player_metadata` WHERE `player` = ?")
	PREPARE_STATEMENT(player_metadata_add, "INSERT INTO `player_metadata` "
		"(`player`, `metadata`, `value`) VALUES (?, ?, ?)")
	PREPARE_STATEMENT(player_metadata_remove, "DELETE FROM `player_metadata` "
		"WHERE `player` = ?")
	verbosestream << "ServerEnvironment: SQLite3 database opened (players)." << std::endl;
}

bool PlayerDatabaseSQLite3::playerDataExists(const std::string &name)
{
	verifyDatabase();
	str_to_sqlite(m_stmt_player_load, 1, name);
	bool res = (sqlite3_step(m_stmt_player_load) == SQLITE_ROW);
	sqlite3_reset(m_stmt_player_load);
	return res;
}

void PlayerDatabaseSQLite3::savePlayer(RemotePlayer *player)
{
	PlayerSAO* sao = player->getPlayerSAO();
	sanity_check(sao);

	const v3f &pos = sao->getBasePosition();
	// Begin save in brace is mandatory
	if (!playerDataExists(player->getName())) {
		beginSave();
		str_to_sqlite(m_stmt_player_add, 1, player->getName());
		double_to_sqlite(m_stmt_player_add, 2, sao->getLookPitch());
		double_to_sqlite(m_stmt_player_add, 3, sao->getYaw());
		double_to_sqlite(m_stmt_player_add, 4, pos.X);
		double_to_sqlite(m_stmt_player_add, 5, pos.Y);
		double_to_sqlite(m_stmt_player_add, 6, pos.Z);
		int64_to_sqlite(m_stmt_player_add, 7, sao->getHP());
		int64_to_sqlite(m_stmt_player_add, 8, sao->getBreath());

		sqlite3_vrfy(sqlite3_step(m_stmt_player_add), SQLITE_DONE);
		sqlite3_reset(m_stmt_player_add);
	} else {
		beginSave();
		double_to_sqlite(m_stmt_player_update, 1, sao->getLookPitch());
		double_to_sqlite(m_stmt_player_update, 2, sao->getYaw());
		double_to_sqlite(m_stmt_player_update, 3, pos.X);
		double_to_sqlite(m_stmt_player_update, 4, pos.Y);
		double_to_sqlite(m_stmt_player_update, 5, pos.Z);
		int64_to_sqlite(m_stmt_player_update, 6, sao->getHP());
		int64_to_sqlite(m_stmt_player_update, 7, sao->getBreath());
		str_to_sqlite(m_stmt_player_update, 8, player->getName());

		sqlite3_vrfy(sqlite3_step(m_stmt_player_update), SQLITE_DONE);
		sqlite3_reset(m_stmt_player_update);
	}

	// Write player inventories
	str_to_sqlite(m_stmt_player_remove_inventory, 1, player->getName());
	sqlite3_vrfy(sqlite3_step(m_stmt_player_remove_inventory), SQLITE_DONE);
	sqlite3_reset(m_stmt_player_remove_inventory);

	str_to_sqlite(m_stmt_player_remove_inventory_items, 1, player->getName());
	sqlite3_vrfy(sqlite3_step(m_stmt_player_remove_inventory_items), SQLITE_DONE);
	sqlite3_reset(m_stmt_player_remove_inventory_items);

	std::vector<const InventoryList*> inventory_lists = sao->getInventory()->getLists();
	for (u16 i = 0; i < inventory_lists.size(); i++) {
		const InventoryList* list = inventory_lists[i];

		str_to_sqlite(m_stmt_player_add_inventory, 1, player->getName());
		int_to_sqlite(m_stmt_player_add_inventory, 2, i);
		int_to_sqlite(m_stmt_player_add_inventory, 3, list->getWidth());
		str_to_sqlite(m_stmt_player_add_inventory, 4, list->getName());
		int_to_sqlite(m_stmt_player_add_inventory, 5, list->getSize());
		sqlite3_vrfy(sqlite3_step(m_stmt_player_add_inventory), SQLITE_DONE);
		sqlite3_reset(m_stmt_player_add_inventory);

		for (u32 j = 0; j < list->getSize(); j++) {
			std::ostringstream os;
			list->getItem(j).serialize(os);
			std::string itemStr = os.str();

			str_to_sqlite(m_stmt_player_add_inventory_items, 1, player->getName());
			int_to_sqlite(m_stmt_player_add_inventory_items, 2, i);
			int_to_sqlite(m_stmt_player_add_inventory_items, 3, j);
			str_to_sqlite(m_stmt_player_add_inventory_items, 4, itemStr);
			sqlite3_vrfy(sqlite3_step(m_stmt_player_add_inventory_items), SQLITE_DONE);
			sqlite3_reset(m_stmt_player_add_inventory_items);
		}
	}

	str_to_sqlite(m_stmt_player_metadata_remove, 1, player->getName());
	sqlite3_vrfy(sqlite3_step(m_stmt_player_metadata_remove), SQLITE_DONE);
	sqlite3_reset(m_stmt_player_metadata_remove);

	const StringMap &attrs = sao->getMeta().getStrings();
	for (const auto &attr : attrs) {
		str_to_sqlite(m_stmt_player_metadata_add, 1, player->getName());
		str_to_sqlite(m_stmt_player_metadata_add, 2, attr.first);
		str_to_sqlite(m_stmt_player_metadata_add, 3, attr.second);
		sqlite3_vrfy(sqlite3_step(m_stmt_player_metadata_add), SQLITE_DONE);
		sqlite3_reset(m_stmt_player_metadata_add);
	}
	sao->getMeta().setModified(false);

	endSave();
}

bool PlayerDatabaseSQLite3::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	verifyDatabase();

	str_to_sqlite(m_stmt_player_load, 1, player->getName());
	if (sqlite3_step(m_stmt_player_load) != SQLITE_ROW) {
		sqlite3_reset(m_stmt_player_load);
		return false;
	}
	sao->setLookPitch(sqlite_to_float(m_stmt_player_load, 0));
	sao->setPlayerYaw(sqlite_to_float(m_stmt_player_load, 1));
	sao->setBasePosition(sqlite_to_v3f(m_stmt_player_load, 2));
	sao->setHPRaw((s16) MYMIN(sqlite_to_int(m_stmt_player_load, 5), S16_MAX));
	sao->setBreath((u16) MYMIN(sqlite_to_int(m_stmt_player_load, 6), U16_MAX), false);
	sqlite3_reset(m_stmt_player_load);

	// Load inventory
	str_to_sqlite(m_stmt_player_load_inventory, 1, player->getName());
	while (sqlite3_step(m_stmt_player_load_inventory) == SQLITE_ROW) {
		InventoryList *invList = player->inventory.addList(
			sqlite_to_string(m_stmt_player_load_inventory, 2),
			sqlite_to_uint(m_stmt_player_load_inventory, 3));
		invList->setWidth(sqlite_to_uint(m_stmt_player_load_inventory, 1));

		u32 invId = sqlite_to_uint(m_stmt_player_load_inventory, 0);

		str_to_sqlite(m_stmt_player_load_inventory_items, 1, player->getName());
		int_to_sqlite(m_stmt_player_load_inventory_items, 2, invId);
		while (sqlite3_step(m_stmt_player_load_inventory_items) == SQLITE_ROW) {
			const std::string itemStr = sqlite_to_string(m_stmt_player_load_inventory_items, 1);
			if (itemStr.length() > 0) {
				ItemStack stack;
				stack.deSerialize(itemStr);
				invList->changeItem(sqlite_to_uint(m_stmt_player_load_inventory_items, 0), stack);
			}
		}
		sqlite3_reset(m_stmt_player_load_inventory_items);
	}

	sqlite3_reset(m_stmt_player_load_inventory);

	str_to_sqlite(m_stmt_player_metadata_load, 1, sao->getPlayer()->getName());
	while (sqlite3_step(m_stmt_player_metadata_load) == SQLITE_ROW) {
		std::string attr = sqlite_to_string(m_stmt_player_metadata_load, 0);
		std::string value = sqlite_to_string(m_stmt_player_metadata_load, 1);

		sao->getMeta().setString(attr, value);
	}
	sao->getMeta().setModified(false);
	sqlite3_reset(m_stmt_player_metadata_load);
	return true;
}

bool PlayerDatabaseSQLite3::removePlayer(const std::string &name)
{
	if (!playerDataExists(name))
		return false;

	str_to_sqlite(m_stmt_player_remove, 1, name);
	sqlite3_vrfy(sqlite3_step(m_stmt_player_remove), SQLITE_DONE);
	sqlite3_reset(m_stmt_player_remove);
	return true;
}

void PlayerDatabaseSQLite3::listPlayers(std::vector<std::string> &res)
{
	verifyDatabase();

	while (sqlite3_step(m_stmt_player_list) == SQLITE_ROW)
		res.push_back(sqlite_to_string(m_stmt_player_list, 0));

	sqlite3_reset(m_stmt_player_list);
}
