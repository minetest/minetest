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

#include "server.h"
#include "networkprotocol.h"

class NetworkPacket;

enum ToServerConnectionState {
	TOSERVER_STATE_NOT_CONNECTED,
	TOSERVER_STATE_STARTUP,
	TOSERVER_STATE_INGAME,
	TOSERVER_STATE_ALL,
};

/*
	If it turns out that different message types need to wait on one another,
	assign the same number to them instead of merging them into one.
	
	If you add a new message type to this enum, please also add it to legacyChannelMap.
*/
enum ServerToClientMessageChannel : u8 {
	// Default message channel if none has been specified.
	MTSCMC_DEFAULT,
	// HUD add, remove and change operations.
	MTSCMC_HUD,
	// Authentication and privileges.
	MTSCMC_AUTH,
	// Client state initialization, node and other init-time definitions.
	MTSCMC_INIT,
	// Game asset transfer, including dynamic updates.
	MTSCMC_MEDIA,
	// Map data.
	MTSCMC_MAP,
	// Inventory updates.
	MTSCMC_INVENTORY,
	// Entity messages.
	MTSCMC_ENTITY,
	// Chat messages.
	MTSCMC_CHAT,
	// Camera parameters such as FOV or local position.
	MTSCMC_CAMERA,
	// Forms.
	MTSCMC_FORMSPEC,
	// Particle FX.
	MTSCMC_PARTICLE,
	// Everything physics and motion related.
	MTSCMC_PHYSICS,
	// Visual environment settings such as skybox, time of day, sunlight ratio.
	MTSCMC_ENVIRONMENT,
	// Audio play, stop and volume control.
	MTSCMC_AUDIO,
	// Built in legacy player stats like health and breath.
	MTSCMC_PLAYERSTAT,
	// Modchannel messages.
	MTSCMC_MODCHANNEL,
};

struct ToServerCommandHandler
{
    const std::string name;
    ToServerConnectionState state;
    void (Server::*handler)(NetworkPacket* pkt);
};

struct ClientCommandFactory
{
	const char* name;
	u8 channel;
	bool reliable;
};

extern const ToServerCommandHandler toServerCommandTable[TOSERVER_NUM_MSG_TYPES];

extern const ClientCommandFactory clientCommandFactoryTable[TOCLIENT_NUM_MSG_TYPES];
extern const u8 legacyChannelMap[256];
