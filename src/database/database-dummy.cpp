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
Dummy database class
*/

#include "database-dummy.h"
#include "remoteplayer.h"


bool Database_Dummy::saveBlock(const v3BPOS &pos, const std::string &data)
{
	m_database[getBlockAsString(pos)] = data;
	return true;
}

void Database_Dummy::loadBlock(const v3BPOS &pos, std::string *block)
{
	std::string i = getBlockAsString(pos);
	auto it = m_database.find(i);
	if (it == m_database.end()) {
		*block = "";
		return;
	}

	*block = it->second;
}

bool Database_Dummy::deleteBlock(const v3BPOS &pos)
{
	m_database.erase(getBlockAsString(pos));
	return true;
}

void Database_Dummy::listAllLoadableBlocks(std::vector<v3BPOS> &dst)
{
	dst.reserve(m_database.size());
	for (std::map<std::string, std::string>::const_iterator x = m_database.begin();
			x != m_database.end(); ++x) {
		dst.push_back(getStringAsBlock(x->first));
	}
}

void Database_Dummy::savePlayer(RemotePlayer *player)
{
	m_player_database.insert(player->getName());
}

bool Database_Dummy::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	return m_player_database.find(player->getName()) != m_player_database.end();
}

bool Database_Dummy::removePlayer(const std::string &name)
{
	m_player_database.erase(name);
	return true;
}

void Database_Dummy::listPlayers(std::vector<std::string> &res)
{
	for (const auto &player : m_player_database) {
		res.emplace_back(player);
	}
}
