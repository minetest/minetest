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

#include <map>
#include <string>
#include "database.h"
#include "irrlichttypes.h"

class Database_Dummy : public MapDatabase, public PlayerDatabase, public ModMetadataDatabase
{
public:
	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);

	void savePlayer(RemotePlayer *player);
	bool loadPlayer(RemotePlayer *player, PlayerSAO *sao);
	bool removePlayer(const std::string &name);
	void listPlayers(std::vector<std::string> &res);

	bool getModEntries(const std::string &modname, StringMap *storage);
	bool setModEntry(const std::string &modname,
			const std::string &key, const std::string &value);
	bool removeModEntry(const std::string &modname, const std::string &key);
	void listMods(std::vector<std::string> *res);

	void beginSave() {}
	void endSave() {}

private:
	std::map<s64, std::string> m_database;
	std::set<std::string> m_player_database;
	std::unordered_map<std::string, StringMap> m_mod_meta_database;
};
