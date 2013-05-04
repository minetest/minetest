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

#ifndef SUBGAME_HEADER
#define SUBGAME_HEADER

#include <string>
#include <set>
#include <vector>

class Settings;

#define WORLDNAME_BLACKLISTED_CHARS "/\\"

struct SubgameSpec
{
	std::string id; // "" = game does not exist
	std::string path; // path to game
	std::string gamemods_path; //path to mods of the game
	std::set<std::string> addon_mods_paths; //paths to addon mods for this game
	std::string name;
	std::string menuicon_path;

	SubgameSpec(const std::string &id_="",
			const std::string &path_="",	
			const std::string &gamemods_path_="",
			const std::set<std::string> &addon_mods_paths_=std::set<std::string>(),
			const std::string &name_="",
			const std::string &menuicon_path_=""):
		id(id_),
		path(path_),
		gamemods_path(gamemods_path_),		
		addon_mods_paths(addon_mods_paths_),
		name(name_),
		menuicon_path(menuicon_path_)
	{}

	bool isValid() const
	{
		return (id != "" && path != "");
	}
};

// minetest.conf
bool getGameMinetestConfig(const std::string &game_path, Settings &conf);
// game.conf
bool getGameConfig(const std::string &game_path, Settings &conf);

std::string getGameName(const std::string &game_path);

SubgameSpec findSubgame(const std::string &id);
SubgameSpec findWorldSubgame(const std::string &world_path);

std::set<std::string> getAvailableGameIds();
std::vector<SubgameSpec> getAvailableGames();

bool getWorldExists(const std::string &world_path);
std::string getWorldGameId(const std::string &world_path,
		bool can_be_legacy=false);

struct WorldSpec
{
	std::string path;
	std::string name;
	std::string gameid;

	WorldSpec(
		const std::string &path_="",
		const std::string &name_="",
		const std::string &gameid_=""
	):
		path(path_),
		name(name_),
		gameid(gameid_)
	{}

	bool isValid() const
	{
		return (name != "" && path != "" && gameid != "");
	}
};

std::vector<WorldSpec> getAvailableWorlds();

// Create world directory and world.mt if they don't exist
bool initializeWorld(const std::string &path, const std::string &gameid);

#endif

