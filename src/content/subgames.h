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

#include <string>
#include <set>
#include <vector>

class Settings;

struct SubgameSpec
{
	std::string id;
	std::string name;
	std::string author;
	int release;
	std::string path;
	std::string gamemods_path;
	std::set<std::string> addon_mods_paths;
	std::string menuicon_path;

	SubgameSpec(const std::string &id = "", const std::string &path = "",
			const std::string &gamemods_path = "",
			const std::set<std::string> &addon_mods_paths =
					std::set<std::string>(),
			const std::string &name = "",
			const std::string &menuicon_path = "",
			const std::string &author = "", int release = 0) :
			id(id),
			name(name), author(author), release(release), path(path),
			gamemods_path(gamemods_path), addon_mods_paths(addon_mods_paths),
			menuicon_path(menuicon_path)
	{
	}

	bool isValid() const { return (!id.empty() && !path.empty()); }
};

// minetest.conf
bool getGameMinetestConfig(const std::string &game_path, Settings &conf);

SubgameSpec findSubgame(const std::string &id);
SubgameSpec findWorldSubgame(const std::string &world_path);

std::set<std::string> getAvailableGameIds();
std::vector<SubgameSpec> getAvailableGames();

bool getWorldExists(const std::string &world_path);
//! Try to get the displayed name of a world
std::string getWorldName(const std::string &world_path, const std::string &default_name);
std::string getWorldGameId(const std::string &world_path, bool can_be_legacy = false);

struct WorldSpec
{
	std::string path;
	std::string name;
	std::string gameid;

	WorldSpec(const std::string &path = "", const std::string &name = "",
			const std::string &gameid = "") :
			path(path),
			name(name), gameid(gameid)
	{
	}

	bool isValid() const
	{
		return (!name.empty() && !path.empty() && !gameid.empty());
	}
};

std::vector<WorldSpec> getAvailableWorlds();

// loads the subgame's config and creates world directory
// and world.mt if they don't exist
void loadGameConfAndInitWorld(const std::string &path, const std::string &name,
		const SubgameSpec &gamespec, bool create_world);
