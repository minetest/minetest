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
#include "content/mods.h"
#include <json/json.h>

#pragma once

namespace ServerList
{
#if USE_CURL
enum AnnounceAction {AA_START, AA_UPDATE, AA_DELETE};
void sendAnnounce(AnnounceAction, u16 port,
		const std::vector<std::string> &clients_names = std::vector<std::string>(),
		double uptime = 0, u32 game_time = 0, float lag = 0,
		const std::string &gameid = "", const std::string &mg_name = "",
		const std::vector<ModSpec> &mods = std::vector<ModSpec>(),
		bool dedicated = false);
#endif

} // namespace ServerList
