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
#include "strfnd.h"
#ifndef SERVER
#include "tile.h" // getImagePath
#endif
#include "util/string.h"

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

Strfnd getSubgamePathEnv() {
  std::string sp;
  char *subgame_path = getenv("MINETEST_SUBGAME_PATH");

  if(subgame_path) {
    sp = std::string(subgame_path);
  }

  return Strfnd(sp);
}

SubgameSpec findSubgame(const std::string &id)
{
	if(id == "")
		return SubgameSpec();
	std::string share = porting::path_share;
	std::string user = porting::path_user;
	std::vector<GameFindPath> find_paths;

        Strfnd search_paths = getSubgamePathEnv();

        while(!search_paths.atend()) {
                std::string path = search_paths.next(":");
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

        Strfnd search_paths = getSubgamePathEnv();

        while(!search_paths.atend()) {
                gamespaths.insert(search_paths.next(":"));
        }

	for(std::set<std::string>::const_iterator i = gamespaths.begin();
			i != gamespaths.end(); i++){
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
			i != gameids.end(); i++)
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

std::vector<WorldSpec> getAvailableWorlds()
{
	std::vector<WorldSpec> worlds;
	std::set<std::string> worldspaths;
	worldspaths.insert(porting::path_user + DIR_DELIM + "worlds");
	infostream<<"Searching worlds..."<<std::endl;
	for(std::set<std::string>::const_iterator i = worldspaths.begin();
			i != worldspaths.end(); i++){
		infostream<<"  In "<<(*i)<<": "<<std::endl;
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

bool initializeWorld(const std::string &path, const std::string &gameid)
{
	infostream<<"Initializing world at "<<path<<std::endl;
	// Create world.mt if does not already exist
	std::string worldmt_path = path + DIR_DELIM + "world.mt";
	if(!fs::PathExists(worldmt_path)){
		infostream<<"Creating world.mt ("<<worldmt_path<<")"<<std::endl;
		fs::CreateAllDirs(path);
		std::ostringstream ss(std::ios_base::binary);
		ss<<"gameid = "<<gameid<< "\nbackend = sqlite3\n";
		fs::safeWriteToFile(worldmt_path, ss.str());
	}
	return true;
}


