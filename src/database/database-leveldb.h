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

#pragma once

#include "config.h"

#if USE_LEVELDB

#include <string>
#include "database.h"
#include "leveldb/db.h"

class Database_LevelDB : public MapDatabase
{
public:
	Database_LevelDB(const std::string &savedir);
	~Database_LevelDB();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);

	void beginSave() {}
	void endSave() {}

private:
	leveldb::DB *m_database;
};

class PlayerDatabaseLevelDB : public PlayerDatabase
{
public:
	PlayerDatabaseLevelDB(const std::string &savedir);
	~PlayerDatabaseLevelDB();

	void savePlayer(RemotePlayer *player);
	bool loadPlayer(RemotePlayer *player, PlayerSAO *sao);
	bool removePlayer(const std::string &name);
	void listPlayers(std::vector<std::string> &res);

private:
	leveldb::DB *m_database;
};

class AuthDatabaseLevelDB : public AuthDatabase
{
public:
	AuthDatabaseLevelDB(const std::string &savedir);
	virtual ~AuthDatabaseLevelDB();

	virtual bool getAuth(const std::string &name, AuthEntry &res);
	virtual bool saveAuth(const AuthEntry &authEntry);
	virtual bool createAuth(AuthEntry &authEntry);
	virtual bool deleteAuth(const std::string &name);
	virtual void listNames(std::vector<std::string> &res);
	virtual void reload();

private:
	leveldb::DB *m_database;
};

#endif // USE_LEVELDB
