/*
 * MariaDB Database Backend for Minetest
 * Copyright 2023 CaffeinatedBlocks (caffeinatedblocks@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#if USE_MARIADB

#include <cstdlib>
#include <istream>
#include <netinet/in.h>
#include "database-mariadb.h"
#include "debug.h"
#include "exceptions.h"
#include "player.h"
#include "remoteplayer.h"
#include "server/player_sao.h"
#include "settings.h"
#include "util/string.h"


/**
 * 
 * 
 * Database_MariaDB -- the default MariaDB backend implementation
 * 
 * 
 */


Database_MariaDB::Database_MariaDB(const std::string &connect_string, const char *type) {

	if (connect_string.empty()) {
		
		std::ostringstream oss;

		oss << "[MariaDB] Error: connection string is empty or undefined.\n\n"
		    << "Set mariadb" << type << "_connection string in world.mt to use the MariaDB backend.\n\n"
		    << "Notes:\n\n"
		    << "1. mariadb" << type << "_connection format is as follows:\n\n"
		    << "   \"hostname=127.0.0.1 port=3306 user=minetest password=minetest dbname=minetest\n\n"
		    << "   Hint: is better to use 127.0.0.1 in place of localhost to avoid unnecessary DNS lookup\n\n"
		    << "2. The user must be created using the MariaDB client prior to using it with Minetest. Example:\n\n"
		    << "   $ mysql -sse \"CREATE USER 'minetest'@'%' IDENTIFIED BY 'PaSSw0rd';\"\n\n"
		    << "3. The database must also be created using the MariaDB client prior to using it with Minetest. Example:\n\n"
		    << "   $ mysql -sse \"CREATE DATABASE minetest CHARACTER SET = 'utf8mb4' COLLATE = 'utf8mb4_unicode_ci';\"\n\n"
		    << "4. The user must be granted privileges on that database using the MariaDB client prior to using it with Minetest. Example:\n\n"
		    << "   $ mysql -sse \"GRANT ALTER, CREATE, DELETE, DROP, INDEX, INSERT, SELECT, UPDATE ON minetest.* TO 'minetest'@'%';\"\n\n"
		    << "5. To enable automatic database creation, you can skip step 3, and instead of step 4, grant global privileges like so:\n\n"
		    << "   mysql -sse \"GRANT ALTER, CREATE, DELETE, DROP, INDEX, INSERT, SELECT, UPDATE ON *.* TO 'minetest'@'%';\"\n\n";

		throw SettingNotFoundException(oss.str());

	}

	params = parseConnectionString(connect_string);

	std::string connect_string_key = "mariadb" + (std::string) type;

	if (0 == params.count("hostname")) {
		errorstream << "[MariaDB] Error: required parameter 'hostname' is undefined (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (params.at("hostname").empty()) {
		errorstream << "[MariaDB] Error: hostname cannot be empty (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (0 == params.count("port")) {
		errorstream << "[MariaDB] Error: required parameter 'port' is undefined (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (params.at("port").empty()) {
		errorstream << "[MariaDB] Error: port cannot be empty (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (0 == params.count("user")) {
		errorstream << "[MariaDB] Error: required parameter 'user' is undefined (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (params.at("user").empty()) {
		errorstream << "[MariaDB] Error: user cannot be empty (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (0 == params.count("password")) {
		errorstream << "[MariaDB] Error: required parameter 'password' is undefined (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (params.at("password").empty()) {
		errorstream << "[MariaDB] Error: password cannot be empty (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (0 == params.count("dbname")) {
		errorstream << "[MariaDB] Error: required parameter 'dbname' is undefined (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

	if (params.at("dbname").empty()) {
		errorstream << "[MariaDB] Error: dbname cannot be empty (check " + connect_string_key + " in world.mt)" << std::endl;
		throw DatabaseException("MariaDB error");
	}

}


Database_MariaDB::~Database_MariaDB() {
	if (conn && !conn->isClosed()) {
		conn->close();
	}
}


void Database_MariaDB::beginTransaction() {
	try {
		stmtBeginTransaction->execute();
	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void Database_MariaDB::checkConnection() {
	if (!conn) {
		errorstream << "[MariaDB] Error: no connection" << std::endl;
		throw DatabaseException("MariaDB Error");
	}
	if (!conn->isValid()) {
		conn->reset();
	}
}


void Database_MariaDB::commit() {
	try {
		stmtCommit->execute();
	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void Database_MariaDB::connect() {
	try {

		// configure
		sql::Driver* driver(sql::mariadb::get_driver_instance());
		sql::SQLString url("jdbc:mariadb://" + params.at("hostname") + ":" + params.at("port"));
		sql::Properties properties({
			{ "autoReconnect",	"true" },
			{ "user",       	params.at("user") },
			{ "password",   	params.at("password") }
		});

		// connect
		try {
			conn = std::unique_ptr<sql::Connection>(driver->connect(url, properties));
		} catch (sql::SQLException &e) {
			errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
			throw DatabaseException("MariaDB Error");
		}
		if (!conn) {
			errorstream << "[MariaDB] Error: could not connect to database" << std::endl;
			throw DatabaseException("MariaDB Error");
		}

		// prepare statements
		stmtBeginTransaction.reset(prepareStatement(
			"START TRANSACTION"
		));

		stmtCommit.reset(prepareStatement(
			"COMMIT"
		));

		stmtLastInsertId.reset(prepareStatement(
			"SELECT LAST_INSERT_ID()"
		));

		stmtRollback.reset(prepareStatement(
			"ROLLBACK"
		));

		// create database if it does not exist
		this->createDatabase(params.at("dbname"));

		// select database
		try {
			conn->setSchema(params.at("dbname"));
		} catch (sql::SQLException &e) {
			errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
			throw DatabaseException("MariaDB Error");
		}

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void Database_MariaDB::createDatabase(std::string db_name) {
	try {

		std::unique_ptr<sql::Statement> stmt(conn->createStatement());
		stmt->execute("CREATE DATABASE IF NOT EXISTS " + params.at("dbname") + " CHARACTER SET = 'utf8mb4' COLLATE = 'utf8mb4_unicode_ci'");
		stmt->close();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void Database_MariaDB::createTable(std::string table_name, std::string table_spec) {
	try {

		std::unique_ptr<sql::Statement> stmt(conn->createStatement());
		stmt->executeUpdate("CREATE TABLE IF NOT EXISTS " + table_name + " (" + table_spec + ") CHARACTER SET = 'utf8mb4' COLLATE = 'utf8mb4_unicode_ci' ENGINE=InnoDB");
		stmt->close();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


std::int32_t Database_MariaDB::lastInsertId() {
	try {
		
		std::unique_ptr<sql::ResultSet> resLastInsertId(stmtLastInsertId->executeQuery());

		resLastInsertId->next();
		std::int32_t id(resLastInsertId->getInt(1));

		resLastInsertId->close();
		
		return id;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


std::map<std::string, std::string> Database_MariaDB::parseConnectionString(std::string input) {
	
	input += " ";
	std::map<std::string, std::string> params;
	size_t pos, last_pos = 0;
	
	while (std::string::npos != (pos = input.find(' ', last_pos))) {

		std::string part = input.substr(last_pos, pos - last_pos);
		std::size_t pos2 = part.find('=');
		
		if (std::string::npos != pos2) {

			params[part.substr(0, pos2)] = part.substr(pos2 + 1);
		
		}
		
		last_pos = pos + 1;

	}

	return params;
}


sql::PreparedStatement* Database_MariaDB::prepareStatement(const std::string query) {
	try {

		sql::PreparedStatement* stmt(conn->prepareStatement(query));

		return stmt;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void Database_MariaDB::rollback() {
	try {

		stmtRollback->execute();
	
	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


/**
 * 
 * 
 * MapDatabaseMariaDB -- The MariaDB Map Database Backend
 * 
 * 
 */


MapDatabaseMariaDB::MapDatabaseMariaDB(const std::string &connect_string) : Database_MariaDB(connect_string, ""), MapDatabase() {

	// connect to database
	connect();

	// create table if it does not exist
	createTable(
		"blocks",
		"posX INT, "
		"posY INT, "
		"posZ INT, "
		"data BLOB, "
		"PRIMARY KEY (posX, posY, posZ)"
	);

	try {

		// prepare statements
		stmtDeleteBlock.reset(prepareStatement(
			"DELETE FROM blocks WHERE posX = ? and posY = ? and posZ = ?"
		));

		stmtLoadBlock.reset(prepareStatement(
			"SELECT data FROM blocks WHERE posX = ? and posY = ? and posZ = ?"
		));

		stmtListAllLoadableBlocks.reset(prepareStatement(
			"SELECT posX, posY, posZ FROM blocks"
		));

		stmtSaveBlock.reset(prepareStatement(
			"INSERT INTO blocks (posX, posY, posZ, data) VALUES (?, ?, ?, ?) "
			"ON DUPLICATE KEY UPDATE data = VALUE(data)"
		));

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool MapDatabaseMariaDB::deleteBlock(const v3s16 &pos) {

	checkConnection();
	
	try {

		stmtDeleteBlock->setInt(1, pos.X);
		stmtDeleteBlock->setInt(2, pos.Y);
		stmtDeleteBlock->setInt(3, pos.Z);
		stmtDeleteBlock->executeUpdate();
		stmtDeleteBlock->clearParameters();

		std::int32_t rows_affected = stmtDeleteBlock->getUpdateCount();

		return (rows_affected > 0);

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void MapDatabaseMariaDB::listAllLoadableBlocks(std::vector<v3s16> &dst) {

	checkConnection();

	try {

		std::unique_ptr<sql::ResultSet> resBlocks(stmtListAllLoadableBlocks->executeQuery());

		while (resBlocks->next()) {
		
			v3s16 row(resBlocks->getInt("x"), resBlocks->getInt("y"), resBlocks->getInt("z"));
			dst.push_back(row);
		
		}
		
		resBlocks->close();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void MapDatabaseMariaDB::loadBlock(const v3s16 &pos, std::string *block) {

	checkConnection();

	try {

		stmtLoadBlock->setInt(1, htonl(pos.X));
		stmtLoadBlock->setInt(2, htonl(pos.Y));
		stmtLoadBlock->setInt(3, htonl(pos.Z));
		
		std::unique_ptr<sql::ResultSet> resBlock(stmtLoadBlock->executeQuery());

		if (resBlock->next()) {

			std::stringstream in;
			in << resBlock->getBlob("data")->rdbuf();
			//std::string data = std::move(in).str();
			//*block = data;
			*block = std::move(in).str();

		} else {

			block->clear();

		}

		resBlock->close();
		stmtLoadBlock->clearParameters();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool MapDatabaseMariaDB::saveBlock(const v3s16 &pos, const std::string &data) {

	checkConnection();

	try {

		size_t max_size = 4294967295;
		if (data.size() > max_size) {

			errorstream << "[MariaDB] Error: refusing to save block at "
					    << "(x = " << pos.X << ", y = " << pos.Y << ", z = " << pos.Z << ") "
						<< "because data would be truncated: " << data.size() << " > " << max_size << std::endl;
			return false;

		}

		std::istringstream _data(data);

		stmtSaveBlock->setInt(1, pos.X);
		stmtSaveBlock->setInt(2, pos.Y);
		stmtSaveBlock->setInt(3, pos.Z);
		stmtSaveBlock->setBlob(4, &_data);
		stmtSaveBlock->executeUpdate();
		stmtSaveBlock->clearParameters();

		std::int32_t rows_affected = stmtSaveBlock->getUpdateCount();

		return (rows_affected > 0);

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


/*
 *
 * PlayerDatabaseMariaDB -- The MariaDB Player Database Backend
 * 
 */


PlayerDatabaseMariaDB::PlayerDatabaseMariaDB(const std::string &connect_string)
: Database_MariaDB(connect_string, "_player"), PlayerDatabase() {
	
	// connect to database
	connect();

	// note that size of player/name column is
	// determined by PLAYERNAME_SIZE from player.h

	// create tables if they do not exist
	createTable("player",
		"name VARCHAR(" + std::to_string(PLAYERNAME_SIZE) + ") NOT NULL, "
		"pitch DECIMAL(15, 7) NOT NULL, "
		"yaw DECIMAL(15, 7) NOT NULL, "
		"posX DECIMAL(15, 7) NOT NULL, "
		"posY DECIMAL(15, 7) NOT NULL, "
		"posZ DECIMAL(15, 7) NOT NULL, "
		"hp INT NOT NULL, "
		"breath INT NOT NULL, "
		"creation_date DATETIME NOT NULL DEFAULT NOW(), "
		"modification_date DATETIME NOT NULL DEFAULT NOW(), "
		"PRIMARY KEY (name)"
	);

	createTable("player_inventories",
		"player VARCHAR(" + std::to_string(PLAYERNAME_SIZE) + ") NOT NULL, "
		"inv_id INT NOT NULL, "
		"inv_width INT NOT NULL, "
		"inv_name TEXT NOT NULL DEFAULT '', "
		"inv_size INT NOT NULL, "
		"PRIMARY KEY(player, inv_id), "
		"INDEX idx_player (player), "
		"CONSTRAINT player_inventories_fkey FOREIGN KEY (player) REFERENCES player (name) ON DELETE CASCADE"
	);

	createTable("player_inventory_items",
		"player VARCHAR(" + std::to_string(PLAYERNAME_SIZE) + "), "
		"inv_id INT NOT NULL, "
		"slot_id INT NOT NULL, "
		"item TEXT NOT NULL DEFAULT '', "
		"PRIMARY KEY(player, inv_id, slot_id), "
		"INDEX idx_player(player), "
		"CONSTRAINT player_inventory_items_fkey FOREIGN KEY (player) REFERENCES player (name) ON DELETE CASCADE"
	);

	createTable("player_metadata",
		"player VARCHAR(" + std::to_string(PLAYERNAME_SIZE) + ") NOT NULL, "
		"attr TEXT NOT NULL, "
		"value TEXT NOT NULL DEFAULT '', "
		"PRIMARY KEY(player, attr(192)), "
		"INDEX idx_player (player), "
		"CONSTRAINT player_metadata_fkey FOREIGN KEY (player) REFERENCES player (name) ON DELETE CASCADE"
	);

	try {

		// prepare statements
		stmtAddPlayerInventory.reset(prepareStatement(
			"INSERT INTO player_inventories (player, inv_id, inv_width, inv_name, inv_size) "
			"VALUES (?, ?, ?, ?, ?)"
		));

		stmtAddPlayerInventoryItem.reset(prepareStatement(
			"INSERT INTO player_inventory_items (player, inv_id, slot_id, item) VALUES "
			"(?, ?, ?, ?)"
		));

		stmtCreatePlayer.reset(prepareStatement(
			"INSERT INTO player(name, pitch, yaw, posX, posY, posZ, hp, breath) "
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
		));

		stmtLoadPlayer.reset(prepareStatement(
			"SELECT pitch, yaw, posX, posY, posZ, hp, breath FROM player WHERE name = ?"
		));

		stmtLoadPlayerInventories.reset(prepareStatement(
			"SELECT inv_id, inv_width, inv_name, inv_size "
			"FROM player_inventories "
			"WHERE player = ? ORDER BY inv_id"
		));

		stmtLoadPlayerInventoryItems.reset(prepareStatement(
			"SELECT slot_id, item "
			"FROM player_inventory_items "
			"WHERE player = ? AND inv_id = ?"
		));

		stmtLoadPlayerList.reset(prepareStatement(
			"SELECT name FROM player"
		));

		stmtLoadPlayerMetadata.reset(prepareStatement(
			"SELECT attr, value FROM player_metadata WHERE player = ?"
		));

		stmtPlayerExists.reset(prepareStatement(
			"SELECT EXISTS (SELECT name FROM player WHERE name = ?)"
		));

		stmtRemovePlayer.reset(prepareStatement(
			"DELETE FROM player WHERE name = ?"
		));

		stmtRemovePlayerInventories.reset(prepareStatement(
			"DELETE FROM player_inventories WHERE player = ?"
		));

		stmtRemovePlayerInventoryItems.reset(prepareStatement(
			"DELETE FROM player_inventory_items WHERE player = ?"
		));

		stmtRemovePlayerMetadata.reset(prepareStatement(
			"DELETE FROM player_metadata WHERE player = ?"
		));

		stmtSavePlayer.reset(prepareStatement(
			"INSERT "
			"INTO player (name, pitch, yaw, posX, posY, posZ, hp, breath) "
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
			"ON DUPLICATE KEY UPDATE "
			"pitch = VALUE(pitch), "
			"yaw = VALUE(yaw), "
			"posX = VALUE(posX), "
			"posY = VALUE(posY), "
			"posZ = VALUE(posZ), "
			"hp = VALUE(hp), "
			"breath = VALUE(breath)"
		));

		stmtSavePlayerMetadata.reset(prepareStatement(
			"INSERT INTO player_metadata (player, attr, value) VALUES (?, ?, ?)"
		));

		stmtUpdatePlayer.reset(prepareStatement(
			"UPDATE player SET "
			"pitch = ?, "
			"yaw = ?, "
			"posX = ?, "
			"posY = ?, "
			"posZ = ?, "
			"hp = ?, "
			"breath = ?, "
			"modification_date = NOW() "
			"WHERE name = ?"
		));

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool PlayerDatabaseMariaDB::loadPlayer(RemotePlayer *player, PlayerSAO *sao) {

	sanity_check(sao);
	checkConnection();

	try {

		std::string player_name(player->getName());

		// load player
		stmtLoadPlayer->setString(1, player_name);
		std::unique_ptr<sql::ResultSet> resPlayer(stmtLoadPlayer->executeQuery());
		stmtLoadPlayer->clearParameters();

		if (!resPlayer->next()) {

			resPlayer->close();
			return false;

		}

		sao->setLookPitch(resPlayer->getFloat("pitch"));
		sao->setRotation(v3f(0, resPlayer->getFloat("yaw"), 0));
		sao->setBasePosition(v3f(resPlayer->getFloat("posX"), resPlayer->getFloat("posY"), resPlayer->getFloat("posZ")));
		sao->setHPRaw((u16) resPlayer->getInt("hp"));
		sao->setBreath((u16) resPlayer->getInt("breath"));
		resPlayer->close();

		// load player inventory
		stmtLoadPlayerInventories->setString(1, player_name);
		std::unique_ptr<sql::ResultSet> resPlayerInventories(stmtLoadPlayerInventories->executeQuery());
		stmtLoadPlayerInventories->clearParameters();

		while (resPlayerInventories->next()) {
			
			// create inventory list
			u32 inv_id = resPlayerInventories->getInt("inv_id");
			std::string inv_name(resPlayerInventories->getString("inv_name"));
			InventoryList* inventoryList = player->inventory.addList(inv_name, resPlayerInventories->getInt("inv_size"));
			inventoryList->setWidth(resPlayerInventories->getInt("inv_width"));

			// create stacks
			stmtLoadPlayerInventoryItems->setString(1, player_name);
			stmtLoadPlayerInventoryItems->setInt(2, inv_id);
			std::unique_ptr<sql::ResultSet> resPlayerInventoryItems(stmtLoadPlayerInventoryItems->executeQuery());
			stmtLoadPlayerInventoryItems->clearParameters();

			while (resPlayerInventoryItems->next()) {

				u16 slot_id = resPlayerInventoryItems->getInt("slot_id");
				const std::string item(resPlayerInventoryItems->getString("item"));

				if (item.length() > 0) {

					ItemStack stack;
					stack.deSerialize(item);
					inventoryList->changeItem(slot_id, stack);

				}

			}

			resPlayerInventoryItems->close();

		}

		resPlayerInventories->close();

		// load player metadata
		stmtLoadPlayerMetadata->setString(1, player_name);
		std::unique_ptr<sql::ResultSet> resPlayerMetadata(stmtLoadPlayerMetadata->executeQuery());
		stmtLoadPlayerMetadata->clearParameters();

		while (resPlayerMetadata->next()) {

			std::string attr(resPlayerMetadata->getString("attr"));
			std::string value(resPlayerMetadata->getString("value"));
			sao->getMeta().setString(attr, value);
			
		}

		sao->getMeta().setModified(false);

		resPlayerMetadata->close();

		return true;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void PlayerDatabaseMariaDB::listPlayers(std::vector<std::string> &list) {

	checkConnection();

	try {

		std::unique_ptr<sql::ResultSet> playerListResultSet(stmtLoadPlayerList->executeQuery());

		while (playerListResultSet->next()) {
		
			list.emplace_back(playerListResultSet->getString("name"));
		
		}
		
		playerListResultSet->close();
	
	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool PlayerDatabaseMariaDB::playerDataExists(const std::string &player_name) {

	checkConnection();

	try {

		stmtLoadPlayer->setString(1, player_name);
		std::unique_ptr<sql::ResultSet> resPlayer(stmtLoadPlayer->executeQuery());
		stmtLoadPlayer->clearParameters();

		bool exists = resPlayer->next();

		resPlayer->close();

		return exists;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}

bool PlayerDatabaseMariaDB::removePlayer(const std::string &player_name) {

	if (!playerDataExists(player_name)) {

		return false;
	
	}
	
	try {

		stmtRemovePlayer->setString(1, player_name);
		stmtRemovePlayer->executeUpdate();
		stmtRemovePlayer->clearParameters();
		
		return true;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void PlayerDatabaseMariaDB::savePlayer(RemotePlayer *player) {

	PlayerSAO* sao = player->getPlayerSAO();
	if (!sao) {
		
		return;

	}

	checkConnection();

	try {

		std::string player_name(player->getName());
		v3f pos = sao->getBasePosition();

		beginTransaction();

		// save player
		stmtSavePlayer->setString(1, player_name);
		stmtSavePlayer->setInt(2, sao->getLookPitch());
		stmtSavePlayer->setInt(3, sao->getRotation().Y);
		stmtSavePlayer->setInt(4, pos.X);
		stmtSavePlayer->setInt(5, pos.Y);
		stmtSavePlayer->setInt(6, pos.Z);
		stmtSavePlayer->setInt(7, sao->getHP());
		stmtSavePlayer->setInt(8, sao->getBreath());
		stmtSavePlayer->execute();
		stmtSavePlayer->clearParameters();

		// delete player inventories
		stmtRemovePlayerInventories->setString(1, player_name);
		stmtRemovePlayerInventories->execute();
		stmtRemovePlayerInventories->clearParameters();
		stmtRemovePlayerInventoryItems->setString(1, player_name);
		stmtRemovePlayerInventoryItems->execute();
		stmtRemovePlayerInventoryItems->clearParameters();

		// save player inventories
		const auto &inventory_lists = sao->getInventory()->getLists();
		std::ostringstream oss;
		for (u16 i = 0; i < inventory_lists.size(); i++) {

			const InventoryList* list = inventory_lists[i];

			stmtAddPlayerInventory->setString(1, player_name);
			stmtAddPlayerInventory->setInt(2, i);
			stmtAddPlayerInventory->setInt(3, list->getWidth());
			stmtAddPlayerInventory->setString(4, list->getName());
			stmtAddPlayerInventory->setInt(5, list->getSize());
			stmtAddPlayerInventory->execute();
			stmtAddPlayerInventory->clearParameters();

			for (u32 j = 0; j < list->getSize(); j++) {
				oss.str("");
				oss.clear();
				list->getItem(j).serialize(oss);
				std::string item_str = oss.str();
				stmtAddPlayerInventoryItem->setString(1, player_name);
				stmtAddPlayerInventoryItem->setInt(2, i);
				stmtAddPlayerInventoryItem->setInt(3, j);
				stmtAddPlayerInventoryItem->setString(4, item_str);
				stmtAddPlayerInventoryItem->execute();
				stmtAddPlayerInventoryItem->clearParameters();
			}
		}

		stmtRemovePlayerMetadata->setString(1, player_name);
		stmtRemovePlayerMetadata->execute();
		stmtRemovePlayerMetadata->clearParameters();

		const StringMap &attrs = sao->getMeta().getStrings();
		for (const auto &attr : attrs) {
			stmtSavePlayerMetadata->setString(1, player_name);
			stmtSavePlayerMetadata->setString(2, attr.first);
			stmtSavePlayerMetadata->setString(3, attr.second);
			stmtSavePlayerMetadata->execute();
			stmtSavePlayerMetadata->clearParameters();
		}

		commit();

		player->onSuccessfulSave();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


/*
 *
 * AuthDatabaseMariaDB -- The MariaDB Auth Database Backend
 * 
 */


AuthDatabaseMariaDB::AuthDatabaseMariaDB(const std::string &connect_string)
: Database_MariaDB(connect_string, "_auth"), AuthDatabase() {
	
	// connect to database
	connect();

	// create tables if they do not exist
	createTable("auth",
		"id INT(10) UNSIGNED NOT NULL AUTO_INCREMENT, "
		"name VARCHAR(" + std::to_string(PLAYERNAME_SIZE) + ") UNIQUE NOT NULL, "
		"password VARCHAR(512) UNIQUE NOT NULL, "
		"last_login INT NOT NULL DEFAULT 0, "
		"PRIMARY KEY (id), "
		"INDEX idx_name (name)"
	);
	
	createTable("user_privileges",
		"id INT(10) UNSIGNED NOT NULL, "
		"privilege VARCHAR(255) NOT NULL, "
		"PRIMARY KEY (id, privilege), "
		"INDEX idx_id (id), "
		"CONSTRAINT fk_id FOREIGN KEY (id) REFERENCES auth (id) ON DELETE CASCADE"
	);

	try {

		// prepare statements
		stmtCreateAuth.reset(prepareStatement(
			"INSERT INTO auth (name, password, last_login) VALUES (?, ?, ?)"
		));

		stmtDeleteAuth.reset(prepareStatement(
			"DELETE FROM auth WHERE name = ?"
		));

		stmtDeletePrivs.reset(prepareStatement(
			"DELETE FROM user_privileges WHERE id = ?"
		));

		stmtListNames.reset(prepareStatement(
			"SELECT name FROM auth ORDER BY name DESC"
		));

		stmtReadAuth.reset(prepareStatement(
			"SELECT id, name, password, last_login FROM auth WHERE name = ?"
		));

		stmtReadPrivs.reset(prepareStatement(
			"SELECT privilege FROM user_privileges WHERE id = ?"
		));

		stmtWriteAuth.reset(prepareStatement(
			"UPDATE auth SET name = ?, password = ?, last_login = ? WHERE id = ?"
		));

		stmtWritePrivs.reset(prepareStatement(
			"INSERT INTO user_privileges (id, privilege) VALUES (?, ?)"
		));

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool AuthDatabaseMariaDB::getAuth(const std::string &name, AuthEntry &res) {
	
	checkConnection();

	try {

		stmtReadAuth->setString(1, name);
		std::unique_ptr<sql::ResultSet> resAuth(stmtReadAuth->executeQuery());
		stmtReadAuth->clearParameters();

		if (!resAuth->next()) {

			resAuth->close();
			return false;

		}

		res.id = resAuth->getInt("id");
		res.name = resAuth->getString("name");
		res.password = resAuth->getString("password");
		res.last_login = resAuth->getInt("last_login");

		resAuth->close();

		stmtReadPrivs->setInt(1, res.id);
		std::unique_ptr<sql::ResultSet> resPrivs(stmtReadPrivs->executeQuery());
		stmtReadPrivs->clearParameters();

		while (resPrivs->next()) {

			res.privileges.emplace_back(resPrivs->getString("privilege"));

		}

		resPrivs->close();

		return true;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool AuthDatabaseMariaDB::saveAuth(const AuthEntry &authEntry) {

	checkConnection();

	try {
	
		beginTransaction();

		stmtWriteAuth->setString(1, authEntry.name);
		stmtWriteAuth->setString(2, authEntry.password);
		stmtWriteAuth->setInt(3, authEntry.last_login);
		stmtWriteAuth->setInt(4, authEntry.id);
		stmtWriteAuth->executeUpdate();
		stmtWriteAuth->clearParameters();

		writePrivileges(authEntry);

		commit();
		
		return true;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool AuthDatabaseMariaDB::createAuth(AuthEntry &authEntry) {
	
	checkConnection();

	try {

		beginTransaction();

		stmtCreateAuth->setString(1, authEntry.name);
		stmtCreateAuth->setString(2, authEntry.password);
		stmtCreateAuth->setInt(3, authEntry.last_login);
		stmtCreateAuth->executeUpdate();
		stmtCreateAuth->clearParameters();

		authEntry.id = lastInsertId();

		writePrivileges(authEntry);

		commit();

		return true;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}

bool AuthDatabaseMariaDB::deleteAuth(const std::string &name) {

	checkConnection();

	try {

		stmtDeleteAuth->setString(1, name);
		stmtDeleteAuth->executeUpdate();
		std::int32_t rows_affected = stmtDeleteAuth->getUpdateCount();
		stmtDeleteAuth->clearParameters();

		return (rows_affected > 0);

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}

void AuthDatabaseMariaDB::listNames(std::vector<std::string> &res) {
	
	checkConnection();

	try {

		std::unique_ptr<sql::ResultSet> resNames(stmtListNames->executeQuery());

		while (resNames->next()) {
		
			res.emplace_back(resNames->getString("name"));
		
		}

		resNames->close();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void AuthDatabaseMariaDB::reload() {}


void AuthDatabaseMariaDB::writePrivileges(const AuthEntry &authEntry) {

	checkConnection();

	try {

		stmtDeletePrivs->setInt(1, authEntry.id);
		stmtDeletePrivs->executeUpdate();
		stmtDeletePrivs->clearParameters();

		for (const std::string &privilege : authEntry.privileges) {

			stmtWritePrivs->setInt(1, authEntry.id);
			stmtWritePrivs->setString(2, privilege);
			stmtWritePrivs->execute();
			stmtWritePrivs->clearParameters();
			
		}

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


/*
 *
 * ModStorageDatabaseMariaDB -- The MariaDB Mod Storage Database Backend
 * 
 */


ModStorageDatabaseMariaDB::ModStorageDatabaseMariaDB(const std::string &connect_string)
: Database_MariaDB(connect_string, "_mod_storage"), ModStorageDatabase() {

	// connect to database
	connect();

	// create table if it does not exist
	createTable("mod_storage",
		"id INT UNSIGNED NOT NULL AUTO_INCREMENT, "
		"mod_name TEXT NOT NULL, "
		"mod_key TEXT NOT NULL, "
		"mod_value TEXT NOT NULL DEFAULT '', "
		"PRIMARY KEY (id), "
		"INDEX idx_mod_name (mod_name(192)), "
		"INDEX idx_mod_name_key (mod_name(192), mod_key(192))"
	);

	try {

		// prepare statements
		stmtGetModEntries.reset(prepareStatement(
			"SELECT mod_key, mod_value FROM mod_storage WHERE mod_name = ?"
		));

		stmtGetModKeys.reset(prepareStatement(
			"SELECT mod_key FROM mod_storage WHERE mod_name = ?"
		));

		stmtGetModEntry.reset(prepareStatement(
			"SELECT mod_value FROM mod_storage WHERE mod_name = ? AND mod_key = ?"
		));

		stmtHasModEntry.reset(prepareStatement(
			"SELECT EXISTS(SELECT mod_name FROM mod_storage WHERE mod_name = ? AND mod_key = ?)"
		));

		stmtListMods.reset(prepareStatement(
			"SELECT DISTINCT mod_name FROM mod_storage"
		));

		stmtRemoveModEntry.reset(prepareStatement(
			"DELETE FROM mod_storage WHERE mod_name = ? AND mod_key = ?"
		));

		stmtRemoveModEntries.reset(prepareStatement(
			"DELETE FROM mod_storage WHERE mod_name = ?"
		));

		stmtSetModEntry.reset(prepareStatement(
			"INSERT INTO mod_storage (mod_name, mod_key, mod_value) VALUES (?, ?, ?) "
			"ON DUPLICATE KEY UPDATE "
			"mod_value = VALUE(mod_value)"
		));

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void ModStorageDatabaseMariaDB::getModEntries(const std::string &modname, StringMap *storage) {
	
	checkConnection();

	try {

		stmtGetModEntries->setString(1, modname);
		std::unique_ptr<sql::ResultSet> resEntries(stmtGetModEntries->executeQuery());
		stmtGetModEntries->clearParameters();
		
		while (resEntries->next()) {
		
			std::string key(resEntries->getString("mod_key"));
			std::string value(resEntries->getString("mod_value"));
			(*storage)[key] = value;
		
		}

		resEntries->close();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool ModStorageDatabaseMariaDB::getModEntry(const std::string &modname, const std::string &key, std::string *value) {
	
	checkConnection();

	try {

		stmtGetModEntry->setString(1, modname);
		stmtGetModEntry->setString(2, key);
		std::unique_ptr<sql::ResultSet> resEntry(stmtGetModEntry->executeQuery());
		stmtGetModEntry->clearParameters();
		
		bool exists = false;

		if (resEntry->next()) {

			*value = resEntry->getString("mod_value");
			exists = true;

		}
		
		resEntry->close();

		return exists;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void ModStorageDatabaseMariaDB::getModKeys(const std::string &modname, std::vector<std::string> *storage) {
	
	checkConnection();

	try {

		stmtGetModKeys->setString(1, modname);
		std::unique_ptr<sql::ResultSet> resKeys(stmtGetModKeys->executeQuery());
		std::size_t num_rows = resKeys->rowsCount();
		stmtGetModKeys->clearParameters();
		storage->reserve(storage->size() + num_rows);
		
		while (resKeys->next()) {
		
			std::string key(resKeys->getString("mod_key"));
			storage->push_back(key);
		
		}
		
		resKeys->close();

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool ModStorageDatabaseMariaDB::hasModEntry(const std::string &modname, const std::string &key) {
	
	checkConnection();
	
	try {

		stmtHasModEntry->setString(1, modname);
		stmtHasModEntry->setString(2, key);
		std::unique_ptr<sql::ResultSet> resExists(stmtHasModEntry->executeQuery());
		stmtHasModEntry->clearParameters();
		
		bool exists = false;

		if (resExists->next()) {

			std::int32_t exists = resExists->getInt(1);
			exists = (exists > 0);

		}

		resExists->close();

		return exists;

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


void ModStorageDatabaseMariaDB::listMods(std::vector<std::string> *res) {
	
	checkConnection();
	
	try {

		std::unique_ptr<sql::ResultSet> resMods(stmtListMods->executeQuery());
		
		while (resMods->next()) {
		
			std::string modname(resMods->getString("mod_name"));
			res->push_back(modname);
		
		}
		
		resMods->close();
		
	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool ModStorageDatabaseMariaDB::removeModEntries(const std::string &modname) {
	
	checkConnection();
	
	try {

		stmtRemoveModEntries->setString(1, modname);
		stmtRemoveModEntries->executeUpdate();
		std::int32_t rows_affected = stmtRemoveModEntries->getUpdateCount();
		stmtRemoveModEntries->clearParameters();

		return (rows_affected > 1);
		
	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool ModStorageDatabaseMariaDB::removeModEntry(const std::string &modname, const std::string &key) {
	
	checkConnection();
	
	try {

		stmtRemoveModEntry->setString(1, modname);
		stmtRemoveModEntry->setString(2, key);
		stmtRemoveModEntry->executeUpdate();
		std::int32_t rows_affected = stmtRemoveModEntry->getUpdateCount();
		stmtRemoveModEntry->clearParameters();

		return (rows_affected > 1);

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}


bool ModStorageDatabaseMariaDB::setModEntry(const std::string &modname, const std::string &key, const std::string &value) {
	
	checkConnection();
	
	try {

		stmtSetModEntry->setString(1, modname);
		stmtSetModEntry->setString(2, key);
		stmtSetModEntry->setString(3, value);
		stmtSetModEntry->executeUpdate();
		std::int32_t rows_affected = stmtSetModEntry->getUpdateCount();
		stmtSetModEntry->clearParameters();

		return (rows_affected > 1);

	} catch (sql::SQLException &e) {
		errorstream << "[MariaDB] Error: " << + e.what() << std::endl;
		throw DatabaseException("MariaDB Error");
	}
}

#endif // USE_MARIADB