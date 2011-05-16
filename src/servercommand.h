/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>

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

#ifndef SERVERCOMMAND_HEADER
#define SERVERCOMMAND_HEADER

#include <vector>
#include <sstream>
#include "common_irrlicht.h"
#include "player.h"
#include "server.h"

struct ServerCommandContext
{

	std::vector<std::wstring> parms;
	Server* server;
	ServerEnvironment *env;
	Player* player;
	// Effective privs for the player, which may be different to their
	// stored ones - e.g. if they are named in the config as an admin.
	u64 privs;
	u32 flags;

	ServerCommandContext(
		std::vector<std::wstring> parms,
		Server* server,
		ServerEnvironment *env,
		Player* player,
		u64 privs)
		: parms(parms), server(server), env(env), player(player), privs(privs)
	{
	}

};

// Process a command sent from a client. The environment and connection
// should be locked when this is called.
// Returns a response message, to be dealt with according to the flags set
// in the context.
std::wstring processServerCommand(ServerCommandContext *ctx);

#endif


