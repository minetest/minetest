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
#include "util/string.h"

// !!! WARNING !!!
// This backend is intended to be used on Minetest 0.4.16 only for the transition backend
// for player files

PlayerDatabaseFiles::PlayerDatabaseFiles(const std::string &savedir) : m_savedir(savedir)
{
	fs::CreateDir(m_savedir);
}

void PlayerDatabaseFiles::serialize(std::ostringstream &os, RemotePlayer *player)
{
	// Utilize a Settings object for storing values
	Settings args;
	args.setS32("version", 1);
	args.set("name", player->getName());

	sanity_check(player->getPlayerSAO());
	args.setU16("hp", player->getPlayerSAO()->getHP());
	args.setV3F("position", player->getPlayerSAO()->getBasePosition());
	args.setFloat("pitch", player->getPlayerSAO()->getLookPitch());
	args.setFloat("yaw", player->getPlayerSAO()->getRotation().Y);
	args.setU16("breath", player->getPlayerSAO()->getBreath());

	std::string extended_attrs;
	player->serializeExtraAttributes(extended_attrs);
	args.set("extended_attributes", extended_attrs);

	args.writeLines(os);

	os << "PlayerArgsEnd\n";

	player->inventory.serialize(os);
}

void PlayerDatabaseFiles::savePlayer(RemotePlayer *player)
{
	fs::CreateDir(m_savedir);

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

	player->onSuccessfulSave();
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

		res.emplace_back(player.getName());
	}
}

AuthDatabaseFiles::AuthDatabaseFiles(const std::string &savedir) : m_savedir(savedir)
{
	readAuthFile();
}

bool AuthDatabaseFiles::getAuth(const std::string &name, AuthEntry &res)
{
	const auto res_i = m_auth_list.find(name);
	if (res_i == m_auth_list.end()) {
		return false;
	}
	res = res_i->second;
	return true;
}

bool AuthDatabaseFiles::saveAuth(const AuthEntry &authEntry)
{
	m_auth_list[authEntry.name] = authEntry;

	// save entire file
	return writeAuthFile();
}

bool AuthDatabaseFiles::createAuth(AuthEntry &authEntry)
{
	m_auth_list[authEntry.name] = authEntry;

	// save entire file
	return writeAuthFile();
}

bool AuthDatabaseFiles::deleteAuth(const std::string &name)
{
	if (!m_auth_list.erase(name)) {
		// did not delete anything -> hadn't existed
		return false;
	}
	return writeAuthFile();
}

void AuthDatabaseFiles::listNames(std::vector<std::string> &res)
{
	res.clear();
	res.reserve(m_auth_list.size());
	for (const auto &res_pair : m_auth_list) {
		res.push_back(res_pair.first);
	}
}

void AuthDatabaseFiles::reload()
{
	readAuthFile();
}

bool AuthDatabaseFiles::readAuthFile()
{
	std::string path = m_savedir + DIR_DELIM + "auth.txt";
	std::ifstream file(path, std::ios::binary);
	if (!file.good()) {
		return false;
	}
	m_auth_list.clear();
	while (file.good()) {
		std::string line;
		std::getline(file, line);
		std::vector<std::string> parts = str_split(line, ':');
		if (parts.size() < 3) // also: empty line at end
			continue;
		const std::string &name = parts[0];
		const std::string &password = parts[1];
		std::vector<std::string> privileges = str_split(parts[2], ',');
		s64 last_login = parts.size() > 3 ? atol(parts[3].c_str()) : 0;

		m_auth_list[name] = {
				1,
				name,
				password,
				privileges,
				last_login,
		};
	}
	return true;
}

bool AuthDatabaseFiles::writeAuthFile()
{
	std::string path = m_savedir + DIR_DELIM + "auth.txt";
	std::ostringstream output(std::ios_base::binary);
	for (const auto &auth_i : m_auth_list) {
		const AuthEntry &authEntry = auth_i.second;
		output << authEntry.name << ":" << authEntry.password << ":";
		output << str_join(authEntry.privileges, ",");
		output << ":" << authEntry.last_login;
		output << std::endl;
	}
	if (!fs::safeWriteToFile(path, output.str())) {
		infostream << "Failed to write " << path << std::endl;
		return false;
	}
	return true;
}
