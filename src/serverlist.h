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
	std::vector<ServerListSpec> getOnline();
	bool deleteEntry(const ServerListSpec &server);
	bool insert(const ServerListSpec &server);
	std::vector<ServerListSpec> deSerialize(const std::string &liststring);
	const std::string serialize(const std::vector<ServerListSpec> &serverlist);
	std::vector<ServerListSpec> deSerializeJson(const std::string &liststring);
	const std::string serializeJson(const std::vector<ServerListSpec> &serverlist);
	#if USE_CURL
	void sendAnnounce(const std::string &action,
			const std::vector<std::string> &clients_names = std::vector<std::string>(),
			const double uptime = 0, const u32 game_time = 0,
			const float lag = 0, const std::string &gameid = "",
			const std::vector<ModSpec> &mods = std::vector<ModSpec>());
	#endif
} // ServerList namespace

#endif
