/*
Part of Minetest-c55
Copyright (C) 2010-11 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef SERVERCOMMAND_HEADER
#define SERVERCOMMAND_HEADER

#include <vector>
#include <sstream>
#include "common_irrlicht.h"
#include "player.h"
#include "server.h"

#define SEND_TO_SENDER (1<<0)
#define SEND_TO_OTHERS (1<<1)
#define SEND_NO_PREFIX (1<<2)

struct ServerCommandContext
{
	std::vector<std::wstring> parms;
	std::wstring paramstring;
	Server* server;
	ServerEnvironment *env;
	Player* player;
	// Effective privs for the player, which may be different to their
	// stored ones - e.g. if they are named in the config as an admin.
	u64 privs;
	u32 flags;

	ServerCommandContext(
		std::vector<std::wstring> parms,
		std::wstring paramstring,
		Server* server,
		ServerEnvironment *env,
		Player* player,
		u64 privs)
		: parms(parms), paramstring(paramstring),
		server(server), env(env), player(player), privs(privs)
	{
	}

};

// Process a command sent from a client. The environment and connection
// should be locked when this is called.
// Returns a response message, to be dealt with according to the flags set
// in the context.
std::wstring processServerCommand(ServerCommandContext *ctx);

#endif


