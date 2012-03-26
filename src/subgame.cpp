/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "subgame.h"
#include "porting.h"
#include "filesys.h"
#include "settings.h"
#include "log.h"
#include "utility_string.h"

std::string getGameName(const std::string &game_path)
{
	std::string conf_path = game_path + DIR_DELIM + "game.conf";
	Settings conf;
	bool succeeded = conf.readConfigFile(conf_path.c_str());
	if(!succeeded)
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

SubgameSpec findSubgame(const std::string &id)
{
	if(id == "")
		return SubgameSpec();
	std::string share = porting::path_share;
	std::string user = porting::path_user;
	std::vector<GameFindPath> find_paths;
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
	// Find mod directories
	std::set<std::string> mods_paths;
	mods_paths.insert(game_path + DIR_DELIM + "mods");
	if(!user_game)
		mods_paths.insert(share + DIR_DELIM + "mods" + DIR_DELIM + id);
	if(user != share || user_game)
		mods_paths.insert(user + DIR_DELIM + "mods" + DIR_DELIM + id);
	std::string game_name = getGameName(game_path);
	if(game_name == "")
		game_name = id;
	return SubgameSpec(id, game_path, mods_paths, game_name);
}

std::set<std::string> getAvailableGameIds()
{
	std::set<std::string> gameids;
	std::set<std::string> gamespaths;
	gamespaths.insert(porting::path_share + DIR_DELIM + "games");
	gamespaths.insert(porting::path_user + DIR_DELIM + "games");
	for(std::set<std::string>::const_iterator i = gamespaths.begin();
			i != gamespaths.end(); i++){
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(*i);
		for(u32 j=0; j<dirlist.size(); j++){
			if(!dirlist[j].dir)
				continue;
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
		std::ofstream of(worldmt_path.c_str(), std::ios::binary);
		of<<"gameid = "<<gameid<<"\n";
	}
	return true;
}


