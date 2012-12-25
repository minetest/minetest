/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SERVERLIST_HEADER
#define SERVERLIST_HEADER

struct ServerListSpec
{
	std::string name;
	std::string address;
	std::string port;
	std::string description;
};

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
} //ServerList namespace

#endif
