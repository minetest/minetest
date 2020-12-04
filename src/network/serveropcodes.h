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
	When adding new channels, ensure that CHANNEL_COUNT in connection.h 
	is big enough to accomodate all the channels in this enum.
	
	If it turns out that different message types need to wait on one another,
	assign the same number to them instead of merging them into one.
	
	Always assign numbers explicitly to allow easy rearrangement.
*/
enum ServerToClientMessageChannel {
	// Default message channel if none has been specified.
	MTSCMC_DEFAULT = 0,
	// HUD add, remove and change operations.
	MTSCMC_HUD = 1,
	// Authentication and privileges.
	MTSCMC_AUTH = 2,
	// Client state initialization, node and other init-time definitions.
	MTSCMC_INIT = 3,
	// Game asset transfer, including dynamic updates.
	MTSCMC_MEDIA = 4,
	// Map data.
	MTSCMC_MAP = 5,
	// Inventory updates.
	MTSCMC_INVENTORY = 6,
	// Entity messages.
	MTSCMC_ENTITY = 7,
	// Chat messages.
	MTSCMC_CHAT = 8,
	// Camera parameters such as FOV or local position.
	MTSCMC_CAMERA = 9,
	// Forms.
	MTSCMC_FORMSPEC = 10,
	// Particle FX.
	MTSCMC_PARTICLE = 11,
	// Everything physics and motion related.
	MTSCMC_PHYSICS = 12,
	// Visual environment settings such as skybox, time of day, sunlight ratio.
	MTSCMC_ENVIRONMENT = 16,
	// Audio play, stop and volume control.
	MTSCMC_AUDIO = 14,
	// Built in legacy player stats like health and breath.
	MTSCMC_PLAYERSTAT = 15,
	// Modchannel messages.
	MTSCMC_MODCHANNEL = 16,
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
