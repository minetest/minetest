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

#include <iostream>
#include "config.h"
#include "mods.h"
#include "json/json.h"

#ifndef SERVERLIST_HEADER
#define SERVERLIST_HEADER

typedef Json::Value ServerListSpec;

namespace ServerList
{
	std::vector<ServerListSpec> getLocal();
	#if USE_CURL
	std::vector<ServerListSpec> getOnline();
	#endif
	bool deleteEntry(ServerListSpec server);
	bool insert(ServerListSpec server);
	std::vector<ServerListSpec> deSerialize(std::string liststring);
	std::string serialize(std::vector<ServerListSpec>);
	std::vector<ServerListSpec> deSerializeJson(std::string liststring);
	std::string serializeJson(std::vector<ServerListSpec>);
	#if USE_CURL
	void sendAnnounce(std::string action = "", const std::vector<std::string> & clients_names = std::vector<std::string>(), 
				double uptime = 0, u32 game_time = 0, float lag = 0, std::string gameid = "", 
				std::vector<ModSpec> mods = std::vector<ModSpec>());
	#endif
} //ServerList namespace

#endif
