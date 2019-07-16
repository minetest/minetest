/*
Copyright (C) 2016 Loic Blot <loic.blot@unix-experience.fr>

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

#if USE_POSTGRESQL

#include "database-postgresql.h"

#ifdef _WIN32
        // Without this some of the network functions are not found on mingw
        #ifndef _WIN32_WINNT
                #define _WIN32_WINNT 0x0501
        #endif
        #include <windows.h>
        #include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "debug.h"
#include "exceptions.h"
#include "settings.h"
#include "content_sao.h"
#include "remoteplayer.h"

Database_PostgreSQL::Database_PostgreSQL(const std::string &connect_string) :
	m_connect_string(connect_string)
{
	if (m_connect_string.empty()) {
		throw SettingNotFoundException(
			"Set pgsql_connection string in world.mt to "
			"use the postgresql backend\n"
			"Notes:\n"
			"pgsql_connection has the following form: \n"
			"\tpgsql_connection = host=127.0.0.1 port=5432 user=mt_user "
			"password=mt_password dbname=minetest_world\n"
			"mt_user should have CREATE TABLE, INSERT, SELECT, UPDATE and "
			"DELETE rights on the database.\n"
			"Don't create mt_user as a SUPERUSER!");
	}
}

Database_PostgreSQL::~Database_PostgreSQL()
{
  PQfinish(m_conn);
  PQfinish(m_async_conn);
}

void Database_PostgreSQL::connectToDatabase()
{
	m_conn = PQconnectdb(m_connect_string.c_str());

	if (PQstatus(m_conn) != CONNECTION_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}

	m_async_conn = PQconnectdb(m_connect_string.c_str());

	if (PQstatus(m_async_conn) != CONNECTION_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL async database error: ") +
			PQerrorMessage(m_async_conn));
	}

	m_pgversion = PQserverVersion(m_conn);

	/*
	* We are using UPSERT feature from PostgreSQL 9.5
	* to have the better performance where possible.
	*/
	if (m_pgversion < 90500) {
		warningstream << "Your PostgreSQL server lacks UPSERT "
			<< "support. Use version 9.5 or better if possible."
			<< std::endl;
	}

	infostream << "PostgreSQL Database: Version " << m_pgversion
			<< " Connection made." << std::endl;

	createDatabase();
	initStatements();
}

void Database_PostgreSQL::verifyDatabase()
{
	if (PQstatus(m_conn) == CONNECTION_OK)
		return;

	PQreset(m_conn);
	ping();
}

void Database_PostgreSQL::ping()
{
	if (PQping(m_connect_string.c_str()) != PQPING_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}
}

bool Database_PostgreSQL::initialized() const
{
	return (PQstatus(m_conn) == CONNECTION_OK);
}

void Database_PostgreSQL::createTableIfNotExists(const std::string &table_name,
		const std::string &definition)
{
	std::string sql_check_table = "SELECT relname FROM pg_class WHERE relname='" +
		table_name + "';";
	PGresult *result = PGUtil::checkResults(PQexec(m_conn, sql_check_table.c_str()), false);

	// If table doesn't exist, create it
	if (!PQntuples(result)) {
		PGUtil::checkResults(PQexec(m_conn, definition.c_str()));
	}

	PQclear(result);
}

void Database_PostgreSQL::beginSave()
{
	verifyDatabase();
	PGUtil::checkResults(PQexec(m_conn, "BEGIN;"));
}

void Database_PostgreSQL::endSave()
{
	PGUtil::checkResults(PQexec(m_conn, "COMMIT;"));
}

MapDatabasePostgreSQL::MapDatabasePostgreSQL(const std::string &connect_string):
	Database_PostgreSQL(connect_string),
	MapDatabase()
{
	connectToDatabase();

	save_queue = new MapSaveQueue(m_async_conn);
	save_queue->start();
}

MapDatabasePostgreSQL::~MapDatabasePostgreSQL(){
	save_queue->stop();
	save_queue->wait();

	delete save_queue;
}

void MapDatabasePostgreSQL::createDatabase()
{
	createTableIfNotExists("blocks",
		"CREATE TABLE blocks ("
			"posX INT NOT NULL,"
			"posY INT NOT NULL,"
			"posZ INT NOT NULL,"
			"data BYTEA,"
			"PRIMARY KEY (posX,posY,posZ)"
			");"
	);

	infostream << "PostgreSQL: Map Database was initialized." << std::endl;
}

void MapDatabasePostgreSQL::initStatements()
{
	PGUtil::prepareStatement(m_conn, "read_block",
		"SELECT data FROM blocks "
			"WHERE posX = $1::int4 AND posY = $2::int4 AND "
			"posZ = $3::int4");

	PGUtil::prepareStatement(m_conn, "delete_block", "DELETE FROM blocks WHERE "
		"posX = $1::int4 AND posY = $2::int4 AND posZ = $3::int4");

	PGUtil::prepareStatement(m_conn, "list_all_loadable_blocks",
		"SELECT posX, posY, posZ FROM blocks");
}

bool MapDatabasePostgreSQL::saveBlock(const v3s16 &pos, const std::string &data)
{
	save_queue->enqueue(pos, data);
	return true;
}

void MapDatabasePostgreSQL::loadBlock(const v3s16 &pos, std::string *block)
{
	verifyDatabase();

	s32 x, y, z;
	x = htonl(pos.X);
	y = htonl(pos.Y);
	z = htonl(pos.Z);

	const void *args[] = { &x, &y, &z };
	const int argLen[] = { sizeof(x), sizeof(y), sizeof(z) };
	const int argFmt[] = { 1, 1, 1 };

	PGresult *results = PGUtil::execPrepared(m_conn, "read_block", ARRLEN(args), args,
		argLen, argFmt, false);

	*block = "";

	if (PQntuples(results))
		*block = std::string(PQgetvalue(results, 0, 0), PQgetlength(results, 0, 0));

	PQclear(results);
}

bool MapDatabasePostgreSQL::deleteBlock(const v3s16 &pos)
{
	verifyDatabase();

	s32 x, y, z;
	x = htonl(pos.X);
	y = htonl(pos.Y);
	z = htonl(pos.Z);

	const void *args[] = { &x, &y, &z };
	const int argLen[] = { sizeof(x), sizeof(y), sizeof(z) };
	const int argFmt[] = { 1, 1, 1 };

	PGUtil::execPrepared(m_conn, "delete_block", ARRLEN(args), args, argLen, argFmt);

	return true;
}

void MapDatabasePostgreSQL::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	verifyDatabase();

	PGresult *results = PGUtil::execPrepared(m_conn, "list_all_loadable_blocks", 0,
		NULL, NULL, NULL, false, false);

	int numrows = PQntuples(results);

	for (int row = 0; row < numrows; ++row)
		dst.push_back(PGUtil::pg_to_v3s16(results, 0, 0));

	PQclear(results);
}

/*
 * Player Database
 */
PlayerDatabasePostgreSQL::PlayerDatabasePostgreSQL(const std::string &connect_string):
	Database_PostgreSQL(connect_string),
	PlayerDatabase()
{
	connectToDatabase();
	save_queue = new PlayerSaveQueue(m_async_conn);
	save_queue->start();
}

PlayerDatabasePostgreSQL::~PlayerDatabasePostgreSQL(){
}

void PlayerDatabasePostgreSQL::createDatabase()
{
	createTableIfNotExists("player",
		"CREATE TABLE player ("
			"name VARCHAR(60) NOT NULL,"
			"pitch NUMERIC(15, 7) NOT NULL,"
			"yaw NUMERIC(15, 7) NOT NULL,"
			"posX NUMERIC(15, 7) NOT NULL,"
			"posY NUMERIC(15, 7) NOT NULL,"
			"posZ NUMERIC(15, 7) NOT NULL,"
			"hp INT NOT NULL,"
			"breath INT NOT NULL,"
			"creation_date TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT NOW(),"
			"modification_date TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT NOW(),"
			"PRIMARY KEY (name)"
			");"
	);

	createTableIfNotExists("player_inventories",
		"CREATE TABLE player_inventories ("
			"player VARCHAR(60) NOT NULL,"
			"inv_id INT NOT NULL,"
			"inv_width INT NOT NULL,"
			"inv_name TEXT NOT NULL DEFAULT '',"
			"inv_size INT NOT NULL,"
			"PRIMARY KEY(player, inv_id),"
			"CONSTRAINT player_inventories_fkey FOREIGN KEY (player) REFERENCES "
			"player (name) ON DELETE CASCADE"
			");"
	);

	createTableIfNotExists("player_inventory_items",
		"CREATE TABLE player_inventory_items ("
			"player VARCHAR(60) NOT NULL,"
			"inv_id INT NOT NULL,"
			"slot_id INT NOT NULL,"
			"item TEXT NOT NULL DEFAULT '',"
			"PRIMARY KEY(player, inv_id, slot_id),"
			"CONSTRAINT player_inventory_items_fkey FOREIGN KEY (player) REFERENCES "
			"player (name) ON DELETE CASCADE"
			");"
	);

	createTableIfNotExists("player_metadata",
		"CREATE TABLE player_metadata ("
			"player VARCHAR(60) NOT NULL,"
			"attr VARCHAR(256) NOT NULL,"
			"value TEXT,"
			"PRIMARY KEY(player, attr),"
			"CONSTRAINT player_metadata_fkey FOREIGN KEY (player) REFERENCES "
			"player (name) ON DELETE CASCADE"
			");"
	);

	infostream << "PostgreSQL: Player Database was inited." << std::endl;
}

void PlayerDatabasePostgreSQL::initStatements()
{
	PGUtil::prepareStatement(m_conn, "remove_player", "DELETE FROM player WHERE name = $1");

	PGUtil::prepareStatement(m_conn, "load_player_list", "SELECT name FROM player");

	PGUtil::prepareStatement(m_conn, "load_player_inventories",
		"SELECT inv_id, inv_width, inv_name, inv_size FROM player_inventories "
			"WHERE player = $1 ORDER BY inv_id");

	PGUtil::prepareStatement(m_conn, "load_player_inventory_items",
		"SELECT slot_id, item FROM player_inventory_items WHERE "
			"player = $1 AND inv_id = $2::int");

	PGUtil::prepareStatement(m_conn, "load_player",
		"SELECT pitch, yaw, posX, posY, posZ, hp, breath FROM player WHERE name = $1");

	PGUtil::prepareStatement(m_conn, "load_player_metadata",
		"SELECT attr, value FROM player_metadata WHERE player = $1");


}

bool PlayerDatabasePostgreSQL::playerDataExists(const std::string &playername)
{
	verifyDatabase();

	const char *values[] = { playername.c_str() };
	PGresult *results = PGUtil::execPrepared(m_conn, "load_player", 1, values, false);

	bool res = (PQntuples(results) > 0);
	PQclear(results);
	return res;
}

void PlayerDatabasePostgreSQL::savePlayer(RemotePlayer *player) {

	PlayerSAO* sao = player->getPlayerSAO();
	if (!sao)
		return;

	save_queue->enqueue(player);

	player->onSuccessfulSave();
}

bool PlayerDatabasePostgreSQL::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	sanity_check(sao);
	verifyDatabase();

	const char *values[] = { player->getName() };
	PGresult *results = PGUtil::execPrepared(m_conn, "load_player", 1, values, false, false);

	// Player not found, return not found
	if (!PQntuples(results)) {
		PQclear(results);
		return false;
	}

	sao->setLookPitch(PGUtil::pg_to_float(results, 0, 0));
	sao->setRotation(v3f(0, PGUtil::pg_to_float(results, 0, 1), 0));
	sao->setBasePosition(v3f(
		PGUtil::pg_to_float(results, 0, 2),
		PGUtil::pg_to_float(results, 0, 3),
		PGUtil::pg_to_float(results, 0, 4))
	);
	sao->setHPRaw((u16) PGUtil::pg_to_int(results, 0, 5));
	sao->setBreath((u16) PGUtil::pg_to_int(results, 0, 6), false);

	PQclear(results);

	// Load inventory
	results = PGUtil::execPrepared(m_conn, "load_player_inventories", 1, values, false, false);

	int resultCount = PQntuples(results);

	for (int row = 0; row < resultCount; ++row) {
		InventoryList* invList = player->inventory.
			addList(PQgetvalue(results, row, 2), PGUtil::pg_to_uint(results, row, 3));
		invList->setWidth(PGUtil::pg_to_uint(results, row, 1));

		u32 invId = PGUtil::pg_to_uint(results, row, 0);
		std::string invIdStr = itos(invId);

		const char* values2[] = {
			player->getName(),
			invIdStr.c_str()
		};
		PGresult *results2 = PGUtil::execPrepared(m_conn, "load_player_inventory_items", 2,
			values2, false, false);

		int resultCount2 = PQntuples(results2);
		for (int row2 = 0; row2 < resultCount2; row2++) {
			const std::string itemStr = PQgetvalue(results2, row2, 1);
			if (itemStr.length() > 0) {
				ItemStack stack;
				stack.deSerialize(itemStr);
				invList->changeItem(PGUtil::pg_to_uint(results2, row2, 0), stack);
			}
		}
		PQclear(results2);
	}

	PQclear(results);

	results = PGUtil::execPrepared(m_conn, "load_player_metadata", 1, values, false);

	int numrows = PQntuples(results);
	for (int row = 0; row < numrows; row++) {
		sao->getMeta().setString(PQgetvalue(results, row, 0), PQgetvalue(results, row, 1));
	}
	sao->getMeta().setModified(false);

	PQclear(results);

	return true;
}

bool PlayerDatabasePostgreSQL::removePlayer(const std::string &name)
{
	if (!playerDataExists(name))
		return false;

	verifyDatabase();

	const char *values[] = { name.c_str() };
	PGUtil::execPrepared(m_conn, "remove_player", 1, values);

	return true;
}

void PlayerDatabasePostgreSQL::listPlayers(std::vector<std::string> &res)
{
	verifyDatabase();

	PGresult *results = PGUtil::execPrepared(m_conn, "load_player_list", 0, NULL, false);

	int numrows = PQntuples(results);
	for (int row = 0; row < numrows; row++)
		res.emplace_back(PQgetvalue(results, row, 0));

	PQclear(results);
}

#endif // USE_POSTGRESQL
