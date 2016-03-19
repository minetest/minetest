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

#include "subgame.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "log.h"
#include "util/strfnd.h"
#include "defaultsettings.h"  // for override_default_settings
#include "mapgen.h"  // for MapgenParams
#include "util/string.h"

#ifndef SERVER
	#include "client/tile.h" // getImagePath
#endif

bool getGameMinetestConfig(const std::string &game_path, Settings &conf)
{
	std::string conf_path = game_path + DIR_DELIM + "minetest.conf";
	return conf.readConfigFile(conf_path.c_str());
}

bool getGameConfig(const std::string &game_path, Settings &conf)
{
	std::string conf_path = game_path + DIR_DELIM + "game.conf";
	return conf.readConfigFile(conf_path.c_str());
}

std::string getGameName(const std::string &game_path)
{
	Settings conf;
	if(!getGameConfig(game_path, conf))
		return "";
	if(!conf.exists("name"))
		return "";
	return conf.get("name");
}

struct GameFindPath
{
	std::string path;
	bool user_specific;
	GameFindPath(const std::string &path, bool user_specific):
		path(path),
		user_specific(user_specific)
	{}
};

std::string getSubgamePathEnv()
{
	char *subgame_path = getenv("MINETEST_SUBGAME_PATH");
	return subgame_path ? std::string(subgame_path) : "";
}

SubgameSpec findSubgame(const std::string &id)
{
	if(id == "")
		return SubgameSpec();
	std::string share = porting::path_share;
	std::string user = porting::path_user;
	std::vector<GameFindPath> find_paths;

	Strfnd search_paths(getSubgamePathEnv());

	while (!search_paths.at_end()) {
		std::string path = search_paths.next(PATH_DELIM);
		find_paths.push_back(GameFindPath(
				path + DIR_DELIM + id, false));
		find_paths.push_back(GameFindPath(
				path + DIR_DELIM + id + "_game", false));
	}

	find_paths.push_back(GameFindPath(
			user + DIR_DELIM + "games" + DIR_DELIM + id + "_game", true));
	find_paths.push_back(GameFindPath(
			user + DIR_DELIM + "games" + DIR_DELIM + id, true));
	find_paths.push_back(GameFindPath(
			share + DIR_DELIM + "games" + DIR_DELIM + id + "_game", false));
	find_paths.push_back(GameFindPath(
			share + DIR_DELIM + "games" + DIR_DELIM + id, false));
	// Find game directory
	std::string game_path;
	bool user_game = true; // Game is in user's directory
	for(u32 i=0; i<find_paths.size(); i++){
		const std::string &try_path = find_paths[i].path;
		if(fs::PathExists(try_path)){
			game_path = try_path;
			user_game = find_paths[i].user_specific;
			break;
		}
	}
	if(game_path == "")
		return SubgameSpec();
	std::string gamemod_path = game_path + DIR_DELIM + "mods";
	// Find mod directories
	std::set<std::string> mods_paths;
	if(!user_game)
		mods_paths.insert(share + DIR_DELIM + "mods");
	if(user != share || user_game)
		mods_paths.insert(user + DIR_DELIM + "mods");
	std::string game_name = getGameName(game_path);
	if(game_name == "")
		game_name = id;
	std::string menuicon_path;
#ifndef SERVER
	menuicon_path = getImagePath(game_path + DIR_DELIM + "menu" + DIR_DELIM + "icon.png");
#endif
	return SubgameSpec(id, game_path, gamemod_path, mods_paths, game_name,
			menuicon_path);
}

SubgameSpec findWorldSubgame(const std::string &world_path)
{
	std::string world_gameid = getWorldGameId(world_path, true);
	// See if world contains an embedded game; if so, use it.
	std::string world_gamepath = world_path + DIR_DELIM + "game";
	if(fs::PathExists(world_gamepath)){
		SubgameSpec gamespec;
		gamespec.id = world_gameid;
		gamespec.path = world_gamepath;
		gamespec.gamemods_path= world_gamepath + DIR_DELIM + "mods";
		gamespec.name = getGameName(world_gamepath);
		if(gamespec.name == "")
			gamespec.name = "unknown";
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

	for (std::set<std::string>::const_iterator i = gamespaths.begin();
			i != gamespaths.end(); ++i){
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(*i);
		for(u32 j=0; j<dirlist.size(); j++){
			if(!dirlist[j].dir)
				continue;
			// If configuration file is not found or broken, ignore game
			Settings conf;
			if(!getGameConfig(*i + DIR_DELIM + dirlist[j].name, conf))
				continue;
			// Add it to result
			const char *ends[] = {"_game", NULL};
			std::string shorter = removeStringEnd(dirlist[j].name, ends);
			if(shorter != "")
				gameids.insert(shorter);
			else
				gameids.insert(dirlist[j].name);
		}
	}
	return gameids;
}

std::vector<SubgameSpec> getAvailableGames()
{
	std::vector<SubgameSpec> specs;
	std::set<std::string> gameids = getAvailableGameIds();
	for(std::set<std::string>::const_iterator i = gameids.begin();
			i != gameids.end(); ++i)
		specs.push_back(findSubgame(*i));
	return specs;
}

#define LEGACY_GAMEID "minetest"

bool getWorldExists(const std::string &world_path)
{
	return (fs::PathExists(world_path + DIR_DELIM + "map_meta.txt") ||
			fs::PathExists(world_path + DIR_DELIM + "world.mt"));
}

std::string getWorldGameId(const std::string &world_path, bool can_be_legacy)
{
	std::string conf_path = world_path + DIR_DELIM + "world.mt";
	Settings conf;
	bool succeeded = conf.readConfigFile(conf_path.c_str());
	if(!succeeded){
		if(can_be_legacy){
			// If map_meta.txt exists, it is probably an old minetest world
			if(fs::PathExists(world_path + DIR_DELIM + "map_meta.txt"))
				return LEGACY_GAMEID;
		}
		return "";
	}
	if(!conf.exists("gameid"))
		return "";
	// The "mesetint" gameid has been discarded
	if(conf.get("gameid") == "mesetint")
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
	for (std::set<std::string>::const_iterator i = worldspaths.begin();
			i != worldspaths.end(); ++i) {
		infostream << "  In " << (*i) << ": " <<std::endl;
		std::vector<fs::DirListNode> dirvector = fs::GetDirListing(*i);
		for(u32 j=0; j<dirvector.size(); j++){
			if(!dirvector[j].dir)
				continue;
			std::string fullpath = *i + DIR_DELIM + dirvector[j].name;
			std::string name = dirvector[j].name;
			// Just allow filling in the gameid always for now
			bool can_be_legacy = true;
			std::string gameid = getWorldGameId(fullpath, can_be_legacy);
			WorldSpec spec(fullpath, name, gameid);
			if(!spec.isValid()){
				infostream<<"(invalid: "<<name<<") ";
			} else {
				infostream<<name<<" ";
				worlds.push_back(spec);
			}
		}
		infostream<<std::endl;
	}
	// Check old world location
	do{
		std::string fullpath = porting::path_user + DIR_DELIM + "world";
		if(!fs::PathExists(fullpath))
			break;
		std::string name = "Old World";
		std::string gameid = getWorldGameId(fullpath, true);
		WorldSpec spec(fullpath, name, gameid);
		infostream<<"Old world found."<<std::endl;
		worlds.push_back(spec);
	}while(0);
	infostream<<worlds.size()<<" found."<<std::endl;
	return worlds;
}

bool loadGameConfAndInitWorld(const std::string &path, const SubgameSpec &gamespec)
{
	// Override defaults with those provided by the game.
	// We clear and reload the defaults because the defaults
	// might have been overridden by other subgame config
	// files that were loaded before.
	g_settings->clearDefaults();
	set_default_settings(g_settings);
	Settings game_defaults;
	getGameMinetestConfig(gamespec.path, game_defaults);
	override_default_settings(g_settings, &game_defaults);

	infostream << "Initializing world at " << path << std::endl;

	fs::CreateAllDirs(path);

	// Create world.mt if does not already exist
	std::string worldmt_path = path + DIR_DELIM "world.mt";
	if (!fs::PathExists(worldmt_path)) {
		std::ostringstream ss(std::ios_base::binary);
		ss << "gameid = " << gamespec.id
			<< "\nbackend = sqlite3"
			<< "\ncreative_mode = " << g_settings->get("creative_mode")
			<< "\nenable_damage = " << g_settings->get("enable_damage")
			<< "\n";
		if (!fs::safeWriteToFile(worldmt_path, ss.str()))
			return false;

		infostream << "Wrote world.mt (" << worldmt_path << ")" << std::endl;
	}

	// Create map_meta.txt if does not already exist
	std::string map_meta_path = path + DIR_DELIM + "map_meta.txt";
	if (!fs::PathExists(map_meta_path)){
		verbosestream << "Creating map_meta.txt (" << map_meta_path << ")" << std::endl;
		fs::CreateAllDirs(path);
		std::ostringstream oss(std::ios_base::binary);

		Settings conf;
		MapgenParams params;

		params.load(*g_settings);
		params.save(conf);
		conf.writeLines(oss);
		oss << "[end_of_params]\n";

		fs::safeWriteToFile(map_meta_path, oss.str());
	}
	return true;
}

