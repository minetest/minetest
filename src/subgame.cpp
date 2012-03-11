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

SubgameSpec findSubgame(const std::string &id)
{
	if(id == "")
		return SubgameSpec();
	std::string share_server = porting::path_share + DIR_DELIM + "server";
	std::string user_server = porting::path_user + DIR_DELIM + "server";
	// Find game directory
	std::string game_path =
			user_server + DIR_DELIM + "games" + DIR_DELIM + id;
	bool user_game = true; // Game is in user's directory
	if(!fs::PathExists(game_path)){
		game_path = share_server + DIR_DELIM + "games" + DIR_DELIM + id;
		user_game = false;
	}
	if(!fs::PathExists(game_path))
		return SubgameSpec();
	// Find addon directories
	std::set<std::string> addon_paths;
	if(!user_game)
		addon_paths.insert(share_server + DIR_DELIM + "addons"
				+ DIR_DELIM + id);
	addon_paths.insert(user_server + DIR_DELIM + "addons"
			+ DIR_DELIM + id);
	return SubgameSpec(id, game_path, addon_paths);
}

std::set<std::string> getAvailableGameIds()
{
	std::set<std::string> gameids;
	std::set<std::string> gamespaths;
	gamespaths.insert(porting::path_share + DIR_DELIM + "server"
			+ DIR_DELIM + "games");
	gamespaths.insert(porting::path_user + DIR_DELIM + "server"
			+ DIR_DELIM + "games");
	for(std::set<std::string>::const_iterator i = gamespaths.begin();
			i != gamespaths.end(); i++){
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(*i);
		for(u32 j=0; j<dirlist.size(); j++){
			if(!dirlist[j].dir)
				continue;
			gameids.insert(dirlist[j].name);
		}
	}
	return gameids;
}

#define LEGACY_GAMEID "mesetint"

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
	return conf.get("gameid");
}

std::vector<WorldSpec> getAvailableWorlds()
{
	std::vector<WorldSpec> worlds;
	std::set<std::string> worldspaths;
	worldspaths.insert(porting::path_user + DIR_DELIM + "server"
			+ DIR_DELIM + "worlds");
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
			std::string gameid = getWorldGameId(fullpath);
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
		std::string fullpath = porting::path_user + DIR_DELIM + ".."
				+ DIR_DELIM + "world";
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


