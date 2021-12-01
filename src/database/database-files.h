/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

// !!! WARNING !!!
// This backend is intended to be used on Minetest 0.4.16 only for the transition backend
// for player files

#include "database.h"
#include <unordered_map>
#include <unordered_set>
#include <json/json.h>

class PlayerDatabaseFiles : public PlayerDatabase
{
public:
	PlayerDatabaseFiles(const std::string &savedir);
	virtual ~PlayerDatabaseFiles() = default;

	void savePlayer(RemotePlayer *player);
	bool loadPlayer(RemotePlayer *player, PlayerSAO *sao);
	bool removePlayer(const std::string &name);
	void listPlayers(std::vector<std::string> &res);

private:
	void deSerialize(RemotePlayer *p, std::istream &is, const std::string &playername,
			PlayerSAO *sao);
	/*
		serialize() writes a bunch of text that can contain
		any characters except a '\0', and such an ending that
		deSerialize stops reading exactly at the right point.
	*/
	void serialize(RemotePlayer *p, std::ostream &os);

	std::string m_savedir;
};

class AuthDatabaseFiles : public AuthDatabase
{
public:
	AuthDatabaseFiles(const std::string &savedir);
	virtual ~AuthDatabaseFiles() = default;

	virtual bool getAuth(const std::string &name, AuthEntry &res);
	virtual bool saveAuth(const AuthEntry &authEntry);
	virtual bool createAuth(AuthEntry &authEntry);
	virtual bool deleteAuth(const std::string &name);
	virtual void listNames(std::vector<std::string> &res);
	virtual void reload();

private:
	std::unordered_map<std::string, AuthEntry> m_auth_list;
	std::string m_savedir;
	bool readAuthFile();
	bool writeAuthFile();
};

class ModMetadataDatabaseFiles : public ModMetadataDatabase
{
public:
	ModMetadataDatabaseFiles(const std::string &savedir);
	virtual ~ModMetadataDatabaseFiles() = default;

	virtual bool getModEntries(const std::string &modname, StringMap *storage);
	virtual bool setModEntry(const std::string &modname,
		const std::string &key, const std::string &value);
	virtual bool removeModEntry(const std::string &modname, const std::string &key);
	virtual void listMods(std::vector<std::string> *res);

	virtual void beginSave();
	virtual void endSave();

private:
	Json::Value *getOrCreateJson(const std::string &modname);
	bool writeJson(const std::string &modname, const Json::Value &json);

	std::string m_storage_dir;
	std::unordered_map<std::string, Json::Value> m_mod_meta;
	std::unordered_set<std::string> m_modified;
};
