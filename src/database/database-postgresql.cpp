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
#include "remoteplayer.h"
#include "server/player_sao.h"

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
}

void Database_PostgreSQL::connectToDatabase()
{
	m_conn = PQconnectdb(m_connect_string.c_str());

	if (PQstatus(m_conn) != CONNECTION_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
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

PGresult *Database_PostgreSQL::checkResults(PGresult *result, bool clear)
{
	ExecStatusType statusType = PQresultStatus(result);

	switch (statusType) {
	case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:
		break;
	case PGRES_FATAL_ERROR:
	default:
		throw DatabaseException(
			std::string("PostgreSQL database error: ") +
			PQresultErrorMessage(result));
	}

	if (clear)
		PQclear(result);

	return result;
}

void Database_PostgreSQL::createTableIfNotExists(const std::string &table_name,
		const std::string &definition)
{
	std::string sql_check_table = "SELECT relname FROM pg_class WHERE relname='" +
		table_name + "';";
	PGresult *result = checkResults(PQexec(m_conn, sql_check_table.c_str()), false);

	// If table doesn't exist, create it
	if (!PQntuples(result)) {
		checkResults(PQexec(m_conn, definition.c_str()));
	}

	PQclear(result);
}

void Database_PostgreSQL::beginSave()
{
	verifyDatabase();
	checkResults(PQexec(m_conn, "BEGIN;"));
}

void Database_PostgreSQL::endSave()
{
	checkResults(PQexec(m_conn, "COMMIT;"));
}

void Database_PostgreSQL::rollback()
{
	checkResults(PQexec(m_conn, "ROLLBACK;"));
}

MapDatabasePostgreSQL::MapDatabasePostgreSQL(const std::string &connect_string):
	Database_PostgreSQL(connect_string),
	MapDatabase()
{
	connectToDatabase();
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
	prepareStatement("read_block",
		"SELECT data FROM blocks "
			"WHERE posX = $1::int4 AND posY = $2::int4 AND "
			"posZ = $3::int4");

	if (getPGVersion() < 90500) {
		prepareStatement("write_block_insert",
			"INSERT INTO blocks (posX, posY, posZ, data) SELECT "
				"$1::int4, $2::int4, $3::int4, $4::bytea "
				"WHERE NOT EXISTS (SELECT true FROM blocks "
				"WHERE posX = $1::int4 AND posY = $2::int4 AND "
				"posZ = $3::int4)");

		prepareStatement("write_block_update",
			"UPDATE blocks SET data = $4::bytea "
				"WHERE posX = $1::int4 AND posY = $2::int4 AND "
				"posZ = $3::int4");
	} else {
		prepareStatement("write_block",
			"INSERT INTO blocks (posX, posY, posZ, data) VALUES "
				"($1::int4, $2::int4, $3::int4, $4::bytea) "
				"ON CONFLICT ON CONSTRAINT blocks_pkey DO "
				"UPDATE SET data = $4::bytea");
	}

	prepareStatement("delete_block", "DELETE FROM blocks WHERE "
		"posX = $1::int4 AND posY = $2::int4 AND posZ = $3::int4");

	prepareStatement("list_all_loadable_blocks",
		"SELECT posX, posY, posZ FROM blocks");
}

bool MapDatabasePostgreSQL::saveBlock(const v3s16 &pos, const std::string &data)
{
	// Verify if we don't overflow the platform integer with the mapblock size
	if (data.size() > INT_MAX) {
		errorstream << "Database_PostgreSQL::saveBlock: Data truncation! "
			<< "data.size() over 0xFFFFFFFF (== " << data.size()
			<< ")" << std::endl;
		return false;
	}

	verifyDatabase();

	s32 x, y, z;
	x = htonl(pos.X);
	y = htonl(pos.Y);
	z = htonl(pos.Z);

	const void *args[] = { &x, &y, &z, data.c_str() };
	const int argLen[] = {
		sizeof(x), sizeof(y), sizeof(z), (int)data.size()
	};
	const int argFmt[] = { 1, 1, 1, 1 };

	if (getPGVersion() < 90500) {
		execPrepared("write_block_update", ARRLEN(args), args, argLen, argFmt);
		execPrepared("write_block_insert", ARRLEN(args), args, argLen, argFmt);
	} else {
		execPrepared("write_block", ARRLEN(args), args, argLen, argFmt);
	}
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

	PGresult *results = execPrepared("read_block", ARRLEN(args), args,
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

	execPrepared("delete_block", ARRLEN(args), args, argLen, argFmt);

	return true;
}

void MapDatabasePostgreSQL::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	verifyDatabase();

	PGresult *results = execPrepared("list_all_loadable_blocks", 0,
		NULL, NULL, NULL, false, false);

	int numrows = PQntuples(results);

	for (int row = 0; row < numrows; ++row)
		dst.push_back(pg_to_v3s16(results, row, 0));

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
	if (getPGVersion() < 90500) {
		prepareStatement("create_player",
			"INSERT INTO player(name, pitch, yaw, posX, posY, posZ, hp, breath) VALUES "
				"($1, $2, $3, $4, $5, $6, $7::int, $8::int)");

		prepareStatement("update_player",
			"UPDATE SET pitch = $2, yaw = $3, posX = $4, posY = $5, posZ = $6, hp = $7::int, "
				"breath = $8::int, modification_date = NOW() WHERE name = $1");
	} else {
		prepareStatement("save_player",
			"INSERT INTO player(name, pitch, yaw, posX, posY, posZ, hp, breath) VALUES "
				"($1, $2, $3, $4, $5, $6, $7::int, $8::int)"
				"ON CONFLICT ON CONSTRAINT player_pkey DO UPDATE SET pitch = $2, yaw = $3, "
				"posX = $4, posY = $5, posZ = $6, hp = $7::int, breath = $8::int, "
				"modification_date = NOW()");
	}

	prepareStatement("remove_player", "DELETE FROM player WHERE name = $1");

	prepareStatement("load_player_list", "SELECT name FROM player");

	prepareStatement("remove_player_inventories",
		"DELETE FROM player_inventories WHERE player = $1");

	prepareStatement("remove_player_inventory_items",
		"DELETE FROM player_inventory_items WHERE player = $1");

	prepareStatement("add_player_inventory",
		"INSERT INTO player_inventories (player, inv_id, inv_width, inv_name, inv_size) VALUES "
			"($1, $2::int, $3::int, $4, $5::int)");

	prepareStatement("add_player_inventory_item",
		"INSERT INTO player_inventory_items (player, inv_id, slot_id, item) VALUES "
			"($1, $2::int, $3::int, $4)");

	prepareStatement("load_player_inventories",
		"SELECT inv_id, inv_width, inv_name, inv_size FROM player_inventories "
			"WHERE player = $1 ORDER BY inv_id");

	prepareStatement("load_player_inventory_items",
		"SELECT slot_id, item FROM player_inventory_items WHERE "
			"player = $1 AND inv_id = $2::int");

	prepareStatement("load_player",
		"SELECT pitch, yaw, posX, posY, posZ, hp, breath FROM player WHERE name = $1");

	prepareStatement("remove_player_metadata",
		"DELETE FROM player_metadata WHERE player = $1");

	prepareStatement("save_player_metadata",
		"INSERT INTO player_metadata (player, attr, value) VALUES ($1, $2, $3)");

	prepareStatement("load_player_metadata",
		"SELECT attr, value FROM player_metadata WHERE player = $1");

}

bool PlayerDatabasePostgreSQL::playerDataExists(const std::string &playername)
{
	verifyDatabase();

	const char *values[] = { playername.c_str() };
	PGresult *results = execPrepared("load_player", 1, values, false);

	bool res = (PQntuples(results) > 0);
	PQclear(results);
	return res;
}

void PlayerDatabasePostgreSQL::savePlayer(RemotePlayer *player)
{
	PlayerSAO* sao = player->getPlayerSAO();
	if (!sao)
		return;

	verifyDatabase();

	v3f pos = sao->getBasePosition();
	std::string pitch = ftos(sao->getLookPitch());
	std::string yaw = ftos(sao->getRotation().Y);
	std::string posx = ftos(pos.X);
	std::string posy = ftos(pos.Y);
	std::string posz = ftos(pos.Z);
	std::string hp = itos(sao->getHP());
	std::string breath = itos(sao->getBreath());
	const char *values[] = {
		player->getName(),
		pitch.c_str(),
		yaw.c_str(),
		posx.c_str(), posy.c_str(), posz.c_str(),
		hp.c_str(),
		breath.c_str()
	};

	const char* rmvalues[] = { player->getName() };
	beginSave();

	if (getPGVersion() < 90500) {
		if (!playerDataExists(player->getName()))
			execPrepared("create_player", 8, values, true, false);
		else
			execPrepared("update_player", 8, values, true, false);
	}
	else
		execPrepared("save_player", 8, values, true, false);

	// Write player inventories
	execPrepared("remove_player_inventories", 1, rmvalues);
	execPrepared("remove_player_inventory_items", 1, rmvalues);

	std::vector<const InventoryList*> inventory_lists = sao->getInventory()->getLists();
	for (u16 i = 0; i < inventory_lists.size(); i++) {
		const InventoryList* list = inventory_lists[i];
		const std::string &name = list->getName();
		std::string width = itos(list->getWidth()),
			inv_id = itos(i), lsize = itos(list->getSize());

		const char* inv_values[] = {
			player->getName(),
			inv_id.c_str(),
			width.c_str(),
			name.c_str(),
			lsize.c_str()
		};
		execPrepared("add_player_inventory", 5, inv_values);

		for (u32 j = 0; j < list->getSize(); j++) {
			std::ostringstream os;
			list->getItem(j).serialize(os);
			std::string itemStr = os.str(), slotId = itos(j);

			const char* invitem_values[] = {
				player->getName(),
				inv_id.c_str(),
				slotId.c_str(),
				itemStr.c_str()
			};
			execPrepared("add_player_inventory_item", 4, invitem_values);
		}
	}

	execPrepared("remove_player_metadata", 1, rmvalues);
	const StringMap &attrs = sao->getMeta().getStrings();
	for (const auto &attr : attrs) {
		const char *meta_values[] = {
			player->getName(),
			attr.first.c_str(),
			attr.second.c_str()
		};
		execPrepared("save_player_metadata", 3, meta_values);
	}
	endSave();

	player->onSuccessfulSave();
}

bool PlayerDatabasePostgreSQL::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	sanity_check(sao);
	verifyDatabase();

	const char *values[] = { player->getName() };
	PGresult *results = execPrepared("load_player", 1, values, false, false);

	// Player not found, return not found
	if (!PQntuples(results)) {
		PQclear(results);
		return false;
	}

	sao->setLookPitch(pg_to_float(results, 0, 0));
	sao->setRotation(v3f(0, pg_to_float(results, 0, 1), 0));
	sao->setBasePosition(v3f(
		pg_to_float(results, 0, 2),
		pg_to_float(results, 0, 3),
		pg_to_float(results, 0, 4))
	);
	sao->setHPRaw((u16) pg_to_int(results, 0, 5));
	sao->setBreath((u16) pg_to_int(results, 0, 6), false);

	PQclear(results);

	// Load inventory
	results = execPrepared("load_player_inventories", 1, values, false, false);

	int resultCount = PQntuples(results);

	for (int row = 0; row < resultCount; ++row) {
		InventoryList* invList = player->inventory.
			addList(PQgetvalue(results, row, 2), pg_to_uint(results, row, 3));
		invList->setWidth(pg_to_uint(results, row, 1));

		u32 invId = pg_to_uint(results, row, 0);
		std::string invIdStr = itos(invId);

		const char* values2[] = {
			player->getName(),
			invIdStr.c_str()
		};
		PGresult *results2 = execPrepared("load_player_inventory_items", 2,
			values2, false, false);

		int resultCount2 = PQntuples(results2);
		for (int row2 = 0; row2 < resultCount2; row2++) {
			const std::string itemStr = PQgetvalue(results2, row2, 1);
			if (itemStr.length() > 0) {
				ItemStack stack;
				stack.deSerialize(itemStr);
				invList->changeItem(pg_to_uint(results2, row2, 0), stack);
			}
		}
		PQclear(results2);
	}

	PQclear(results);

	results = execPrepared("load_player_metadata", 1, values, false);

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
	execPrepared("remove_player", 1, values);

	return true;
}

void PlayerDatabasePostgreSQL::listPlayers(std::vector<std::string> &res)
{
	verifyDatabase();

	PGresult *results = execPrepared("load_player_list", 0, NULL, false);

	int numrows = PQntuples(results);
	for (int row = 0; row < numrows; row++)
		res.emplace_back(PQgetvalue(results, row, 0));

	PQclear(results);
}

AuthDatabasePostgreSQL::AuthDatabasePostgreSQL(const std::string &connect_string) :
		Database_PostgreSQL(connect_string), AuthDatabase()
{
	connectToDatabase();
}

void AuthDatabasePostgreSQL::createDatabase()
{
	createTableIfNotExists("auth",
		"CREATE TABLE auth ("
			"id SERIAL,"
			"name TEXT UNIQUE,"
			"password TEXT,"
			"last_login INT NOT NULL DEFAULT 0,"
			"PRIMARY KEY (id)"
		");");

	createTableIfNotExists("user_privileges",
		"CREATE TABLE user_privileges ("
			"id INT,"
			"privilege TEXT,"
			"PRIMARY KEY (id, privilege),"
			"CONSTRAINT fk_id FOREIGN KEY (id) REFERENCES auth (id) ON DELETE CASCADE"
		");");
}

void AuthDatabasePostgreSQL::initStatements()
{
	prepareStatement("auth_read", "SELECT id, name, password, last_login FROM auth WHERE name = $1");
	prepareStatement("auth_write", "UPDATE auth SET name = $1, password = $2, last_login = $3 WHERE id = $4");
	prepareStatement("auth_create", "INSERT INTO auth (name, password, last_login) VALUES ($1, $2, $3) RETURNING id");
	prepareStatement("auth_delete", "DELETE FROM auth WHERE name = $1");

	prepareStatement("auth_list_names", "SELECT name FROM auth ORDER BY name DESC");

	prepareStatement("auth_read_privs", "SELECT privilege FROM user_privileges WHERE id = $1");
	prepareStatement("auth_write_privs", "INSERT INTO user_privileges (id, privilege) VALUES ($1, $2)");
	prepareStatement("auth_delete_privs", "DELETE FROM user_privileges WHERE id = $1");
}

bool AuthDatabasePostgreSQL::getAuth(const std::string &name, AuthEntry &res)
{
	verifyDatabase();

	const char *values[] = { name.c_str() };
	PGresult *result = execPrepared("auth_read", 1, values, false, false);
	int numrows = PQntuples(result);
	if (numrows == 0) {
		PQclear(result);
		return false;
	}

	res.id = pg_to_uint(result, 0, 0);
	res.name = std::string(PQgetvalue(result, 0, 1), PQgetlength(result, 0, 1));
	res.password = std::string(PQgetvalue(result, 0, 2), PQgetlength(result, 0, 2));
	res.last_login = pg_to_int(result, 0, 3);

	PQclear(result);

	std::string playerIdStr = itos(res.id);
	const char *privsValues[] = { playerIdStr.c_str() };
	PGresult *results = execPrepared("auth_read_privs", 1, privsValues, false);

	numrows = PQntuples(results);
	for (int row = 0; row < numrows; row++)
		res.privileges.emplace_back(PQgetvalue(results, row, 0));

	PQclear(results);

	return true;
}

bool AuthDatabasePostgreSQL::saveAuth(const AuthEntry &authEntry)
{
	verifyDatabase();

	beginSave();

	std::string lastLoginStr = itos(authEntry.last_login);
	std::string idStr = itos(authEntry.id);
	const char *values[] = {
		authEntry.name.c_str() ,
		authEntry.password.c_str(),
		lastLoginStr.c_str(),
		idStr.c_str(),
	};
	execPrepared("auth_write", 4, values);

	writePrivileges(authEntry);

	endSave();
	return true;
}

bool AuthDatabasePostgreSQL::createAuth(AuthEntry &authEntry)
{
	verifyDatabase();

	std::string lastLoginStr = itos(authEntry.last_login);
	const char *values[] = {
		authEntry.name.c_str() ,
		authEntry.password.c_str(),
		lastLoginStr.c_str()
	};

	beginSave();

	PGresult *result = execPrepared("auth_create", 3, values, false, false);

	int numrows = PQntuples(result);
	if (numrows == 0) {
		errorstream << "Strange behaviour on auth creation, no ID returned." << std::endl;
		PQclear(result);
		rollback();
		return false;
	}

	authEntry.id = pg_to_uint(result, 0, 0);
	PQclear(result);

	writePrivileges(authEntry);

	endSave();
	return true;
}

bool AuthDatabasePostgreSQL::deleteAuth(const std::string &name)
{
	verifyDatabase();

	const char *values[] = { name.c_str() };
	execPrepared("auth_delete", 1, values);

	// privileges deleted by foreign key on delete cascade
	return true;
}

void AuthDatabasePostgreSQL::listNames(std::vector<std::string> &res)
{
	verifyDatabase();

	PGresult *results = execPrepared("auth_list_names", 0,
		NULL, NULL, NULL, false, false);

	int numrows = PQntuples(results);

	for (int row = 0; row < numrows; ++row)
		res.emplace_back(PQgetvalue(results, row, 0));

	PQclear(results);
}

void AuthDatabasePostgreSQL::reload()
{
	// noop for PgSQL
}

void AuthDatabasePostgreSQL::writePrivileges(const AuthEntry &authEntry)
{
	std::string authIdStr = itos(authEntry.id);
	const char *values[] = { authIdStr.c_str() };
	execPrepared("auth_delete_privs", 1, values);

	for (const std::string &privilege : authEntry.privileges) {
		const char *values[] = { authIdStr.c_str(), privilege.c_str() };
		execPrepared("auth_write_privs", 2, values);
	}
}


#endif // USE_POSTGRESQL
