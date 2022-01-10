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

#include "content/subgames.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "log.h"
#include "util/strfnd.h"
#include "defaultsettings.h" // for set_default_settings
#include "map_settings_manager.h"
#include "util/string.h"

#ifndef SERVER
#include "client/tile.h" // getImagePath
#endif

// The maximum number of identical world names allowed
#define MAX_WORLD_NAMES 100

namespace
{

bool getGameMinetestConfig(const std::string &game_path, Settings &conf)
{
	std::string conf_path = game_path + DIR_DELIM + "minetest.conf";
	return conf.readConfigFile(conf_path.c_str());
}

}

struct GameFindPath
{
	std::string path;
	bool user_specific;
	GameFindPath(const std::string &path, bool user_specific) :
			path(path), user_specific(user_specific)
	{
	}
};

std::string getSubgamePathEnv()
{
	char *subgame_path = getenv("MINETEST_SUBGAME_PATH");
	return subgame_path ? std::string(subgame_path) : "";
}

SubgameSpec findSubgame(const std::string &id)
{
	if (id.empty())
		return SubgameSpec();
	std::string share = porting::path_share;
	std::string user = porting::path_user;

	// Get games install locations
	Strfnd search_paths(getSubgamePathEnv());

	// Get all possible paths fo game
	std::vector<GameFindPath> find_paths;
	while (!search_paths.at_end()) {
		std::string path = search_paths.next(PATH_DELIM);
		path.append(DIR_DELIM).append(id);
		find_paths.emplace_back(path, false);
		path.append("_game");
		find_paths.emplace_back(path, false);
	}

	std::string game_base = DIR_DELIM;
	game_base = game_base.append("games").append(DIR_DELIM).append(id);
	std::string game_suffixed = game_base + "_game";
	find_paths.emplace_back(user + game_suffixed, true);
	find_paths.emplace_back(user + game_base, true);
	find_paths.emplace_back(share + game_suffixed, false);
	find_paths.emplace_back(share + game_base, false);

	// Find game directory
	std::string game_path;
	bool user_game = true; // Game is in user's directory
	for (const GameFindPath &find_path : find_paths) {
		const std::string &try_path = find_path.path;
		if (fs::PathExists(try_path)) {
			game_path = try_path;
			user_game = find_path.user_specific;
			break;
		}
	}

	if (game_path.empty())
		return SubgameSpec();

	std::string gamemod_path = game_path + DIR_DELIM + "mods";

	// Find mod directories
	std::set<std::string> mods_paths;
	if (!user_game)
		mods_paths.insert(share + DIR_DELIM + "mods");
	if (user != share || user_game)
		mods_paths.insert(user + DIR_DELIM + "mods");

	for (const std::string &mod_path : getEnvModPaths()) {
		mods_paths.insert(mod_path);
	}

	// Get meta
	std::string conf_path = game_path + DIR_DELIM + "game.conf";
	Settings conf;
	conf.readConfigFile(conf_path.c_str());

	std::string game_name;
	if (conf.exists("name"))
		game_name = conf.get("name");
	else
		game_name = id;

	std::string game_author;
	if (conf.exists("author"))
		game_author = conf.get("author");

	int game_release = 0;
	if (conf.exists("release"))
		game_release = conf.getS32("release");

	std::string menuicon_path;
#ifndef SERVER
	menuicon_path = getImagePath(
			game_path + DIR_DELIM + "menu" + DIR_DELIM + "icon.png");
#endif
	return SubgameSpec(id, game_path, gamemod_path, mods_paths, game_name,
			menuicon_path, game_author, game_release);
}

SubgameSpec findWorldSubgame(const std::string &world_path)
{
	std::string world_gameid = getWorldGameId(world_path, true);
	// See if world contains an embedded game; if so, use it.
	std::string world_gamepath = world_path + DIR_DELIM + "game";
	if (fs::PathExists(world_gamepath)) {
		SubgameSpec gamespec;
		gamespec.id = world_gameid;
		gamespec.path = world_gamepath;
		gamespec.gamemods_path = world_gamepath + DIR_DELIM + "mods";

		Settings conf;
		std::string conf_path = world_gamepath + DIR_DELIM + "game.conf";
		conf.readConfigFile(conf_path.c_str());

		if (conf.exists("name"))
			gamespec.name = conf.get("name");
		else
			gamespec.name = world_gameid;

		return gamespec;
	}
	return findSubgame(world_gameid);
}

std::set<std::string> getAvailableGameIds()
{
	std::set<std::string> gameids;
	std::set<std::string> gamespaths;
	gamespaths.insert(porting::path_share + DIR_DELIM + "games");
	gamespaths.insert(porting::path_user + DIR_DELIM + "games");

	Strfnd search_paths(getSubgamePathEnv());

	while (!search_paths.at_end())
		gamespaths.insert(search_paths.next(PATH_DELIM));

	for (const std::string &gamespath : gamespaths) {
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(gamespath);
		for (const fs::DirListNode &dln : dirlist) {
			if (!dln.dir)
				continue;

			// If configuration file is not found or broken, ignore game
			Settings conf;
			std::string conf_path = gamespath + DIR_DELIM + dln.name +
						DIR_DELIM + "game.conf";
			if (!conf.readConfigFile(conf_path.c_str()))
				continue;

			// Add it to result
			const char *ends[] = {"_game", NULL};
			std::string shorter = removeStringEnd(dln.name, ends);
			if (!shorter.empty())
				gameids.insert(shorter);
			else
				gameids.insert(dln.name);
		}
	}
	return gameids;
}

std::vector<SubgameSpec> getAvailableGames()
{
	std::vector<SubgameSpec> specs;
	std::set<std::string> gameids = getAvailableGameIds();
	specs.reserve(gameids.size());
	for (const auto &gameid : gameids)
		specs.push_back(findSubgame(gameid));
	return specs;
}

#define LEGACY_GAMEID "minetest"

bool getWorldExists(const std::string &world_path)
{
	return (fs::PathExists(world_path + DIR_DELIM + "map_meta.txt") ||
			fs::PathExists(world_path + DIR_DELIM + "world.mt"));
}

//! Try to get the displayed name of a world
std::string getWorldName(const std::string &world_path, const std::string &default_name)
{
	std::string conf_path = world_path + DIR_DELIM + "world.mt";
	Settings conf;
	bool succeeded = conf.readConfigFile(conf_path.c_str());
	if (!succeeded) {
		return default_name;
	}

	if (!conf.exists("world_name"))
		return default_name;
	return conf.get("world_name");
}

std::string getWorldGameId(const std::string &world_path, bool can_be_legacy)
{
	std::string conf_path = world_path + DIR_DELIM + "world.mt";
	Settings conf;
	bool succeeded = conf.readConfigFile(conf_path.c_str());
	if (!succeeded) {
		if (can_be_legacy) {
			// If map_meta.txt exists, it is probably an old minetest world
			if (fs::PathExists(world_path + DIR_DELIM + "map_meta.txt"))
				return LEGACY_GAMEID;
		}
		return "";
	}
	if (!conf.exists("gameid"))
		return "";
	// The "mesetint" gameid has been discarded
	if (conf.get("gameid") == "mesetint")
		return "minetest";
	return conf.get("gameid");
}

std::string getWorldPathEnv()
{
	char *world_path = getenv("MINETEST_WORLD_PATH");
	return world_path ? std::string(world_path) : "";
}

std::vector<WorldSpec> getAvailableWorlds()
{
	std::vector<WorldSpec> worlds;
	std::set<std::string> worldspaths;

	Strfnd search_paths(getWorldPathEnv());

	while (!search_paths.at_end())
		worldspaths.insert(search_paths.next(PATH_DELIM));

	worldspaths.insert(porting::path_user + DIR_DELIM + "worlds");
	infostream << "Searching worlds..." << std::endl;
	for (const std::string &worldspath : worldspaths) {
		infostream << "  In " << worldspath << ": ";
		std::vector<fs::DirListNode> dirvector = fs::GetDirListing(worldspath);
		for (const fs::DirListNode &dln : dirvector) {
			if (!dln.dir)
				continue;
			std::string fullpath = worldspath + DIR_DELIM + dln.name;
			std::string name = getWorldName(fullpath, dln.name);
			// Just allow filling in the gameid always for now
			bool can_be_legacy = true;
			std::string gameid = getWorldGameId(fullpath, can_be_legacy);
			WorldSpec spec(fullpath, name, gameid);
			if (!spec.isValid()) {
				infostream << "(invalid: " << name << ") ";
			} else {
				infostream << name << " ";
				worlds.push_back(spec);
			}
		}
		infostream << std::endl;
	}
	// Check old world location
	do {
		std::string fullpath = porting::path_user + DIR_DELIM + "world";
		if (!fs::PathExists(fullpath))
			break;
		std::string name = "Old World";
		std::string gameid = getWorldGameId(fullpath, true);
		WorldSpec spec(fullpath, name, gameid);
		infostream << "Old world found." << std::endl;
		worlds.push_back(spec);
	} while (false);
	infostream << worlds.size() << " found." << std::endl;
	return worlds;
}

void loadGameConfAndInitWorld(const std::string &path, const std::string &name,
		const SubgameSpec &gamespec, bool create_world)
{
	std::string final_path = path;

	// If we're creating a new world, ensure that the path isn't already taken
	if (create_world) {
		int counter = 1;
		while (fs::PathExists(final_path) && counter < MAX_WORLD_NAMES) {
			final_path = path + "_" + std::to_string(counter);
			counter++;
		}

		if (fs::PathExists(final_path)) {
			throw BaseException("Too many similar filenames");
		}
	}

	Settings *game_settings = Settings::getLayer(SL_GAME);
	const bool new_game_settings = (game_settings == nullptr);
	if (new_game_settings) {
		// Called by main-menu without a Server instance running
		// -> create and free manually
		game_settings = Settings::createLayer(SL_GAME);
	}

	getGameMinetestConfig(gamespec.path, *game_settings);
	game_settings->removeSecureSettings();

	infostream << "Initializing world at " << final_path << std::endl;

	fs::CreateAllDirs(final_path);

	// Create world.mt if does not already exist
	std::string worldmt_path = final_path + DIR_DELIM "world.mt";
	if (!fs::PathExists(worldmt_path)) {
		Settings conf;

		conf.set("world_name", name);
		conf.set("gameid", gamespec.id);
		conf.set("backend", "sqlite3");
		conf.set("player_backend", "sqlite3");
		conf.set("auth_backend", "sqlite3");
		conf.set("mod_storage_backend", "sqlite3");
		conf.setBool("creative_mode", g_settings->getBool("creative_mode"));
		conf.setBool("enable_damage", g_settings->getBool("enable_damage"));

		if (!conf.updateConfigFile(worldmt_path.c_str())) {
			throw BaseException("Failed to update the config file");
		}
	}

	// Create map_meta.txt if does not already exist
	std::string map_meta_path = final_path + DIR_DELIM + "map_meta.txt";
	if (!fs::PathExists(map_meta_path)) {
		MapSettingsManager mgr(map_meta_path);

		mgr.setMapSetting("seed", g_settings->get("fixed_map_seed"));

		mgr.makeMapgenParams();
		mgr.saveMapMeta();
	}

	// The Settings object is no longer needed for created worlds
	if (new_game_settings)
		delete game_settings;
}

std::vector<std::string> getEnvModPaths()
{
	const char *c_mod_path = getenv("MINETEST_MOD_PATH");
	std::vector<std::string> paths;
	Strfnd search_paths(c_mod_path ? c_mod_path : "");
	while (!search_paths.at_end())
		paths.push_back(search_paths.next(PATH_DELIM));
	return paths;
}
