/*
Minetest
Copyright (C) 2022 Minetest core developers & community

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

#pragma once

#include "server.h"
#include "server/mods.h"
#include "scripting_server.h"

class MockServer : public Server
{
public:
	/* Set `path_world` to a real existing folder if you plan to initialize scripting! */
	MockServer(const std::string &path_world = "fakepath") :
		Server(path_world, SubgameSpec("fakespec", "fakespec"), true,
			Address(), true, nullptr
		)
	{}

	/*
	 * Use this in unit tests to create scripting.
	 * Note that you still need to call script->loadBuiltin() and don't forget
	 * a try-catch for `ModError`.
	 */
	void createScripting() {
		m_script = std::make_unique<ServerScripting>(this);
		m_modmgr = std::make_unique<ServerModManager>(nullptr);
	}

	void start() = delete;
	void stop() = delete;

private:
	void SendChatMessage(session_t peer_id, const ChatMessage &message) {}
};
