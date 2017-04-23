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

#include <cassert>
#include <json/json.h>
#include "database-files.h"
#include "content_sao.h"
#include "remoteplayer.h"
#include "settings.h"
#include "porting.h"
#include "filesys.h"

// !!! WARNING !!!
// This backend is intended to be used on Minetest 0.4.16 only for the transition backend
// for player files

void PlayerDatabaseFiles::serialize(std::ostringstream &os, RemotePlayer *player)
{
	// Utilize a Settings object for storing values
	Settings args;
	args.setS32("version", 1);
	args.set("name", player->getName());

	sanity_check(player->getPlayerSAO());
	args.setS32("hp", player->getPlayerSAO()->getHP());
	args.setV3F("position", player->getPlayerSAO()->getBasePosition());
	args.setFloat("pitch", player->getPlayerSAO()->getPitch());
	args.setFloat("yaw", player->getPlayerSAO()->getYaw());
	args.setS32("breath", player->getPlayerSAO()->getBreath());

	std::string extended_attrs = "";
	player->serializeExtraAttributes(extended_attrs);
	args.set("extended_attributes", extended_attrs);

	args.writeLines(os);

	os << "PlayerArgsEnd\n";

	player->inventory.serialize(os);
}

void PlayerDatabaseFiles::savePlayer(RemotePlayer *player)
{
	std::string savedir = m_savedir + DIR_DELIM;
	std::string path = savedir + player->getName();
	bool path_found = false;
	RemotePlayer testplayer("", NULL);

	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES && !path_found; i++) {
		if (!fs::PathExists(path)) {
			path_found = true;
			continue;
		}

		// Open and deserialize file to check player name
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good()) {
			errorstream << "Failed to open " << path << std::endl;
			return;
		}

		testplayer.deSerialize(is, path, NULL);
		is.close();
		if (strcmp(testplayer.getName(), player->getName()) == 0) {
			path_found = true;
			continue;
		}

		path = savedir + player->getName() + itos(i);
	}

	if (!path_found) {
		errorstream << "Didn't find free file for player " << player->getName()
				<< std::endl;
		return;
	}

	// Open and serialize file
	std::ostringstream ss(std::ios_base::binary);
	serialize(ss, player);
	if (!fs::safeWriteToFile(path, ss.str())) {
		infostream << "Failed to write " << path << std::endl;
	}
	player->setModified(false);
}

bool PlayerDatabaseFiles::removePlayer(const std::string &name)
{
	std::string players_path = m_savedir + DIR_DELIM;
	std::string path = players_path + name;

	RemotePlayer temp_player("", NULL);
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good())
			continue;

		temp_player.deSerialize(is, path, NULL);
		is.close();

		if (temp_player.getName() == name) {
			fs::DeleteSingleFileOrEmptyDirectory(path);
			return true;
		}

		path = players_path + name + itos(i);
	}

	return false;
}

bool PlayerDatabaseFiles::loadPlayer(RemotePlayer *player, PlayerSAO *sao)
{
	std::string players_path = m_savedir + DIR_DELIM;
	std::string path = players_path + player->getName();

	const std::string player_to_load = player->getName();
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good())
			continue;

		player->deSerialize(is, path, sao);
		is.close();

		if (player->getName() == player_to_load)
			return true;

		path = players_path + player_to_load + itos(i);
	}

	infostream << "Player file for player " << player_to_load << " not found" << std::endl;
	return false;
}

void PlayerDatabaseFiles::listPlayers(std::vector<std::string> &res)
{
	std::vector<fs::DirListNode> files = fs::GetDirListing(m_savedir);
	// list files into players directory
	for (std::vector<fs::DirListNode>::const_iterator it = files.begin(); it !=
		files.end(); ++it) {
		// Ignore directories
		if (it->dir)
			continue;

		const std::string &filename = it->name;
		std::string full_path = m_savedir + DIR_DELIM + filename;
		std::ifstream is(full_path.c_str(), std::ios_base::binary);
		if (!is.good())
			continue;

		RemotePlayer player(filename.c_str(), NULL);
		// Null env & dummy peer_id
		PlayerSAO playerSAO(NULL, &player, 15789, false);

		player.deSerialize(is, "", &playerSAO);
		is.close();

		res.push_back(player.getName());
	}
}
