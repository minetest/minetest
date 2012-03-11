/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SUBGAME_HEADER
#define SUBGAME_HEADER

#include <string>
#include <set>

struct SubgameSpec
{
	std::string id; // "" = game does not exist
	std::string path;
	std::set<std::string> addon_paths;

	SubgameSpec(const std::string &id_="",
			const std::string &path_="",
			const std::set<std::string> &addon_paths_=std::set<std::string>()):
		id(id_),
		path(path_),
		addon_paths(addon_paths_)
	{}

	bool isValid() const
	{
		return (id != "" && path != "");
	}
};

SubgameSpec findSubgame(const std::string &id);

std::set<std::string> getAvailableGameIds();

std::string getWorldGameId(const std::string &world_path);

#endif

