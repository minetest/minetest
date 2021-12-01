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

#include <set>
#include <string>
#include <vector>
#include "irr_v3d.h"
#include "irrlichttypes.h"
#include "util/basic_macros.h"
#include "util/string.h"

class Database
{
public:
	virtual void beginSave() = 0;
	virtual void endSave() = 0;
	virtual bool initialized() const { return true; }
};

class MapDatabase : public Database
{
public:
	virtual ~MapDatabase() = default;

	virtual bool saveBlock(const v3s16 &pos, const std::string &data) = 0;
	virtual void loadBlock(const v3s16 &pos, std::string *block) = 0;
	virtual bool deleteBlock(const v3s16 &pos) = 0;

	static s64 getBlockAsInteger(const v3s16 &pos);
	static v3s16 getIntegerAsBlock(s64 i);

	virtual void listAllLoadableBlocks(std::vector<v3s16> &dst) = 0;
};

class PlayerSAO;
class RemotePlayer;

class PlayerDatabase
{
public:
	virtual ~PlayerDatabase() = default;

	virtual void savePlayer(RemotePlayer *player) = 0;
	virtual bool loadPlayer(RemotePlayer *player, PlayerSAO *sao) = 0;
	virtual bool removePlayer(const std::string &name) = 0;
	virtual void listPlayers(std::vector<std::string> &res) = 0;
};

struct AuthEntry
{
	u64 id;
	std::string name;
	std::string password;
	std::vector<std::string> privileges;
	s64 last_login;
};

class AuthDatabase
{
public:
	virtual ~AuthDatabase() = default;

	virtual bool getAuth(const std::string &name, AuthEntry &res) = 0;
	virtual bool saveAuth(const AuthEntry &authEntry) = 0;
	virtual bool createAuth(AuthEntry &authEntry) = 0;
	virtual bool deleteAuth(const std::string &name) = 0;
	virtual void listNames(std::vector<std::string> &res) = 0;
	virtual void reload() = 0;
};

class ModMetadataDatabase : public Database
{
public:
	virtual ~ModMetadataDatabase() = default;

	virtual bool getModEntries(const std::string &modname, StringMap *storage) = 0;
	virtual bool setModEntry(const std::string &modname,
		const std::string &key, const std::string &value) = 0;
	virtual bool removeModEntry(const std::string &modname, const std::string &key) = 0;
	virtual void listMods(std::vector<std::string> *res) = 0;
};
