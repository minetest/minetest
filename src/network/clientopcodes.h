/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2015 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "client/client.h"
#include "networkprotocol.h"

class NetworkPacket;

enum ToClientConnectionState {
	TOCLIENT_STATE_NOT_CONNECTED,
	TOCLIENT_STATE_CONNECTED,
	TOCLIENT_STATE_ALL,
};

struct ToClientCommandHandler
{
    const char* name;
    ToClientConnectionState state;
    void (Client::*handler)(NetworkPacket* pkt);
};

struct ServerCommandFactory
{
	const char* name;
	u8 channel;
	bool reliable;
};

extern const ToClientCommandHandler toClientCommandTable[TOCLIENT_NUM_MSG_TYPES];

extern const ServerCommandFactory serverCommandFactoryTable[TOSERVER_NUM_MSG_TYPES];
