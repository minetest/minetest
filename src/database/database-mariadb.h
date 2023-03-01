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

#pragma once

#include "config.h"

#if USE_MARIADB

#include <mariadb/conncpp.hpp>
#include <string>
#include "database.h"
#include "util/basic_macros.h"

class Settings;

class Database_MariaDB: public Database {
	public:
		Database_MariaDB(const std::string &connect_string, const char *type);
		~Database_MariaDB();
		void beginSave() { beginTransaction(); };
		void endSave() { commit(); };
		void rollback();

	protected:
		void beginTransaction();
		void checkConnection();
		void commit();
		void connect();
		void createDatabase(std::string db_name);
		void createTable(std::string table_name, std::string table_spec);
		bool databaseExists(std::string db_name);
		std::int32_t lastInsertId();
		sql::PreparedStatement* prepareStatement(const std::string query);
		bool tableExists(std::string table_name);
		std::unique_ptr<sql::Connection> conn;
		
	private:
		std::map<std::string, std::string> parseConnectionString(std::string input);
        std::map<std::string, std::string> params;
		std::unique_ptr<sql::PreparedStatement> stmtDatabaseExists;
		std::unique_ptr<sql::PreparedStatement> stmtLastInsertId;
		std::unique_ptr<sql::PreparedStatement> stmtRollback;
		std::unique_ptr<sql::PreparedStatement> stmtTableExists;

};

class MapDatabaseMariaDB : private Database_MariaDB, public MapDatabase {
	public:
		MapDatabaseMariaDB(const std::string &connect_string);
		virtual ~MapDatabaseMariaDB() = default;
		virtual void beginSave() { Database_MariaDB::beginSave(); };
		virtual void endSave() { Database_MariaDB::endSave(); };
		virtual void rollback() { Database_MariaDB::rollback(); };
		bool deleteBlock(const v3s16 &pos);
		void listAllLoadableBlocks(std::vector<v3s16> &dst);
		void loadBlock(const v3s16 &pos, std::string *block);
		bool saveBlock( const v3s16 &pos, const std::string &data);
	
	private:
		std::unique_ptr<sql::PreparedStatement> stmtDeleteBlock;
		std::unique_ptr<sql::PreparedStatement> stmtListAllLoadableBlocks;
		std::unique_ptr<sql::PreparedStatement> stmtLoadBlock;
		std::unique_ptr<sql::PreparedStatement> stmtSaveBlock;
		
};

class PlayerDatabaseMariaDB : private Database_MariaDB, public PlayerDatabase {

	public:
		PlayerDatabaseMariaDB(const std::string &connect_string);
		virtual ~PlayerDatabaseMariaDB() = default;
		virtual void beginSave() { Database_MariaDB::beginSave(); };
		virtual void endSave() { Database_MariaDB::endSave(); };
		virtual void rollback() { Database_MariaDB::rollback(); };
		void savePlayer(RemotePlayer *player);
		bool loadPlayer(RemotePlayer *player, PlayerSAO *sao);
		bool removePlayer(const std::string &name);
		void listPlayers(std::vector<std::string> &res);

	private:
		bool playerDataExists(const std::string &playername);
		std::unique_ptr<sql::PreparedStatement> stmtAddPlayerInventory;
		std::unique_ptr<sql::PreparedStatement> stmtAddPlayerInventoryItem;
		std::unique_ptr<sql::PreparedStatement> stmtCreatePlayer;
		std::unique_ptr<sql::PreparedStatement> stmtLoadPlayer;
		std::unique_ptr<sql::PreparedStatement> stmtLoadPlayerInventories;
		std::unique_ptr<sql::PreparedStatement> stmtLoadPlayerInventoryItems;
		std::unique_ptr<sql::PreparedStatement> stmtLoadPlayerList;
		std::unique_ptr<sql::PreparedStatement> stmtLoadPlayerMetadata;
		std::unique_ptr<sql::PreparedStatement> stmtPlayerExists;
		std::unique_ptr<sql::PreparedStatement> stmtRemovePlayer;
		std::unique_ptr<sql::PreparedStatement> stmtRemovePlayerInventories;
		std::unique_ptr<sql::PreparedStatement> stmtRemovePlayerInventoryItems;
		std::unique_ptr<sql::PreparedStatement> stmtRemovePlayerMetadata;
		std::unique_ptr<sql::PreparedStatement> stmtSavePlayer;
		std::unique_ptr<sql::PreparedStatement> stmtSavePlayerMetadata;
		std::unique_ptr<sql::PreparedStatement> stmtUpdatePlayer;

};

class AuthDatabaseMariaDB : private Database_MariaDB, public AuthDatabase {

	public:
		AuthDatabaseMariaDB(const std::string &connect_string);
		virtual ~AuthDatabaseMariaDB() = default;
		virtual void beginSave() { Database_MariaDB::beginSave(); };
		virtual void endSave() { Database_MariaDB::endSave(); };
		virtual void rollback() { Database_MariaDB::rollback(); };
		bool getAuth(const std::string &name, AuthEntry &res);
		bool saveAuth(const AuthEntry &authEntry);
		bool createAuth(AuthEntry &authEntry);
		bool deleteAuth(const std::string &name);
		void listNames(std::vector<std::string> &res);
		void reload();
		void writePrivileges(const AuthEntry &authEntry);
	
	private:
		std::unique_ptr<sql::PreparedStatement> stmtCreateAuth;
		std::unique_ptr<sql::PreparedStatement> stmtDeleteAuth;
		std::unique_ptr<sql::PreparedStatement> stmtDeletePrivs;
		std::unique_ptr<sql::PreparedStatement> stmtListNames;
		std::unique_ptr<sql::PreparedStatement> stmtReadAuth;
		std::unique_ptr<sql::PreparedStatement> stmtReadPrivs;
		std::unique_ptr<sql::PreparedStatement> stmtWriteAuth;
		std::unique_ptr<sql::PreparedStatement> stmtWritePrivs;
};

class ModStorageDatabaseMariaDB : private Database_MariaDB, public ModStorageDatabase {

	public:
		ModStorageDatabaseMariaDB(const std::string &connect_string);
		virtual ~ModStorageDatabaseMariaDB() = default;
		virtual void beginSave() { Database_MariaDB::beginSave(); };
		virtual void endSave() { Database_MariaDB::endSave(); };
		virtual void rollback() { Database_MariaDB::rollback(); };
		void getModEntries(const std::string &modname, StringMap *storage);
		void getModKeys(const std::string &modname, std::vector<std::string> *storage);
		bool getModEntry(const std::string &modname, const std::string &key, std::string *value);
		bool hasModEntry(const std::string &modname, const std::string &key);
		bool setModEntry(const std::string &modname, const std::string &key, const std::string &value);
		bool removeModEntry(const std::string &modname, const std::string &key);
		bool removeModEntries(const std::string &modname);
		void listMods(std::vector<std::string> *res);
	
	private:
		std::unique_ptr<sql::PreparedStatement> stmtGetModEntries;
		std::unique_ptr<sql::PreparedStatement> stmtGetModKeys;
		std::unique_ptr<sql::PreparedStatement> stmtGetModEntry;
		std::unique_ptr<sql::PreparedStatement> stmtHasModEntry;
		std::unique_ptr<sql::PreparedStatement> stmtListMods;
		std::unique_ptr<sql::PreparedStatement> stmtRemoveModEntry;
		std::unique_ptr<sql::PreparedStatement> stmtRemoveModEntries;
		std::unique_ptr<sql::PreparedStatement> stmtSetModEntry;
};

#endif // USE_MARIADB