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

#include "serveropcodes.h"

const static ToServerCommandHandler null_command_handler = { "TOSERVER_NULL", TOSERVER_STATE_ALL, &Server::handleCommand_Null };

const ToServerCommandHandler toServerCommandTable[TOSERVER_NUM_MSG_TYPES] =
{
	null_command_handler, // 0x00 (never use this)
	null_command_handler, // 0x01
	{ "TOSERVER_INIT",                     TOSERVER_STATE_NOT_CONNECTED, &Server::handleCommand_Init }, // 0x02
	null_command_handler, // 0x03
	null_command_handler, // 0x04
	null_command_handler, // 0x05
	null_command_handler, // 0x06
	null_command_handler, // 0x07
	null_command_handler, // 0x08
	null_command_handler, // 0x09
	null_command_handler, // 0x0a
	null_command_handler, // 0x0b
	null_command_handler, // 0x0c
	null_command_handler, // 0x0d
	null_command_handler, // 0x0e
	null_command_handler, // 0x0f
	null_command_handler, // 0x10
	{ "TOSERVER_INIT2",                    TOSERVER_STATE_NOT_CONNECTED, &Server::handleCommand_Init2 }, // 0x11
	null_command_handler, // 0x12
	null_command_handler, // 0x13
	null_command_handler, // 0x14
	null_command_handler, // 0x15
	null_command_handler, // 0x16
	{ "TOSERVER_MODCHANNEL_JOIN",          TOSERVER_STATE_INGAME, &Server::handleCommand_ModChannelJoin }, // 0x17
	{ "TOSERVER_MODCHANNEL_LEAVE",         TOSERVER_STATE_INGAME, &Server::handleCommand_ModChannelLeave }, // 0x18
	{ "TOSERVER_MODCHANNEL_MSG",           TOSERVER_STATE_INGAME, &Server::handleCommand_ModChannelMsg }, // 0x19
	null_command_handler, // 0x1a
	null_command_handler, // 0x1b
	null_command_handler, // 0x1c
	null_command_handler, // 0x1d
	null_command_handler, // 0x1e
	null_command_handler, // 0x1f
	null_command_handler, // 0x20
	null_command_handler, // 0x21
	null_command_handler, // 0x22
	{ "TOSERVER_PLAYERPOS",                TOSERVER_STATE_INGAME, &Server::handleCommand_PlayerPos }, // 0x23
	{ "TOSERVER_GOTBLOCKS",                TOSERVER_STATE_STARTUP, &Server::handleCommand_GotBlocks }, // 0x24
	{ "TOSERVER_DELETEDBLOCKS",            TOSERVER_STATE_INGAME, &Server::handleCommand_DeletedBlocks }, // 0x25
	null_command_handler, // 0x26
	null_command_handler, // 0x27
	null_command_handler, // 0x28
	null_command_handler, // 0x29
	null_command_handler, // 0x2a
	null_command_handler, // 0x2b
	null_command_handler, // 0x2c
	null_command_handler, // 0x2d
	null_command_handler, // 0x2e
	null_command_handler, // 0x2f
	null_command_handler, // 0x30
	{ "TOSERVER_INVENTORY_ACTION",         TOSERVER_STATE_INGAME, &Server::handleCommand_InventoryAction }, // 0x31
	{ "TOSERVER_CHAT_MESSAGE",             TOSERVER_STATE_INGAME, &Server::handleCommand_ChatMessage }, // 0x32
	null_command_handler, // 0x33
	null_command_handler, // 0x34
	{ "TOSERVER_DAMAGE",                   TOSERVER_STATE_INGAME, &Server::handleCommand_Damage }, // 0x35
	null_command_handler, // 0x36
	{ "TOSERVER_PLAYERITEM",               TOSERVER_STATE_INGAME, &Server::handleCommand_PlayerItem }, // 0x37
	{ "TOSERVER_RESPAWN",                  TOSERVER_STATE_INGAME, &Server::handleCommand_Respawn }, // 0x38
	{ "TOSERVER_INTERACT",                 TOSERVER_STATE_INGAME, &Server::handleCommand_Interact }, // 0x39
	{ "TOSERVER_REMOVED_SOUNDS",           TOSERVER_STATE_INGAME, &Server::handleCommand_RemovedSounds }, // 0x3a
	{ "TOSERVER_NODEMETA_FIELDS",          TOSERVER_STATE_INGAME, &Server::handleCommand_NodeMetaFields }, // 0x3b
	{ "TOSERVER_INVENTORY_FIELDS",         TOSERVER_STATE_INGAME, &Server::handleCommand_InventoryFields }, // 0x3c
	null_command_handler, // 0x3d
	null_command_handler, // 0x3e
	null_command_handler, // 0x3f
	{ "TOSERVER_REQUEST_MEDIA",            TOSERVER_STATE_STARTUP, &Server::handleCommand_RequestMedia }, // 0x40
	null_command_handler, // 0x41
	null_command_handler, // 0x42
	{ "TOSERVER_CLIENT_READY",             TOSERVER_STATE_STARTUP, &Server::handleCommand_ClientReady }, // 0x43
	null_command_handler, // 0x44
	null_command_handler, // 0x45
	null_command_handler, // 0x46
	null_command_handler, // 0x47
	null_command_handler, // 0x48
	null_command_handler, // 0x49
	null_command_handler, // 0x4a
	null_command_handler, // 0x4b
	null_command_handler, // 0x4c
	null_command_handler, // 0x4d
	null_command_handler, // 0x4e
	null_command_handler, // 0x4f
	{ "TOSERVER_FIRST_SRP",                TOSERVER_STATE_NOT_CONNECTED, &Server::handleCommand_FirstSrp }, // 0x50
	{ "TOSERVER_SRP_BYTES_A",              TOSERVER_STATE_NOT_CONNECTED, &Server::handleCommand_SrpBytesA }, // 0x51
	{ "TOSERVER_SRP_BYTES_M",              TOSERVER_STATE_NOT_CONNECTED, &Server::handleCommand_SrpBytesM }, // 0x52
};

const static ClientCommandFactory null_command_factory = { "TOCLIENT_NULL", 0, false };

/*
	Channels used for Server -> Client communication

	Packet order is only guaranteed inside a channel, so packets that operate on
	the same objects are *required* to be in the same channel.
*/

const ClientCommandFactory clientCommandFactoryTable[TOCLIENT_NUM_MSG_TYPES] =
{
	null_command_factory, // 0x00
	null_command_factory, // 0x01
	{ "TOCLIENT_HELLO",                    MTSCMC_AUTH, true }, // 0x02
	{ "TOCLIENT_AUTH_ACCEPT",              MTSCMC_AUTH, true }, // 0x03
	{ "TOCLIENT_ACCEPT_SUDO_MODE",         MTSCMC_AUTH, true }, // 0x04
	{ "TOCLIENT_DENY_SUDO_MODE",           MTSCMC_AUTH, true }, // 0x05
	null_command_factory, // 0x06
	null_command_factory, // 0x07
	null_command_factory, // 0x08
	null_command_factory, // 0x09
	{ "TOCLIENT_ACCESS_DENIED",            MTSCMC_AUTH, true }, // 0x0A
	null_command_factory, // 0x0B
	null_command_factory, // 0x0C
	null_command_factory, // 0x0D
	null_command_factory, // 0x0E
	null_command_factory, // 0x0F
	{ "TOCLIENT_INIT",                     MTSCMC_INIT, true }, // 0x10
	null_command_factory, // 0x11
	null_command_factory, // 0x12
	null_command_factory, // 0x13
	null_command_factory, // 0x14
	null_command_factory, // 0x15
	null_command_factory, // 0x16
	null_command_factory, // 0x17
	null_command_factory, // 0x18
	null_command_factory, // 0x19
	null_command_factory, // 0x1A
	null_command_factory, // 0x1B
	null_command_factory, // 0x1C
	null_command_factory, // 0x1D
	null_command_factory, // 0x1E
	null_command_factory, // 0x1F
	{ "TOCLIENT_BLOCKDATA",                MTSCMC_MAP, true }, // 0x20
	{ "TOCLIENT_ADDNODE",                  MTSCMC_MAP, true }, // 0x21
	{ "TOCLIENT_REMOVENODE",               MTSCMC_MAP, true }, // 0x22
	null_command_factory, // 0x23
	null_command_factory, // 0x24
	null_command_factory, // 0x25
	null_command_factory, // 0x26
	{ "TOCLIENT_INVENTORY",                MTSCMC_INVENTORY, true }, // 0x27
	null_command_factory, // 0x28
	{ "TOCLIENT_TIME_OF_DAY",              MTSCMC_ENVIRONMENT, true }, // 0x29
	{ "TOCLIENT_CSM_RESTRICTION_FLAGS",    MTSCMC_AUTH, true }, // 0x2A
	{ "TOCLIENT_PLAYER_SPEED",             MTSCMC_PHYSICS, true }, // 0x2B
	{ "TOCLIENT_MEDIA_PUSH",               MTSCMC_MEDIA, true }, // 0x2C (sent over channel 1 too)
	null_command_factory, // 0x2D
	null_command_factory, // 0x2E
	{ "TOCLIENT_CHAT_MESSAGE",             MTSCMC_CHAT, true }, // 0x2F
	null_command_factory, // 0x30
	{ "TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD", MTSCMC_ENTITY, true }, // 0x31
	{ "TOCLIENT_ACTIVE_OBJECT_MESSAGES",   MTSCMC_ENTITY, true }, // 0x32 (may be sent as unrel over channel 1 too)
	{ "TOCLIENT_HP",                       MTSCMC_PLAYERSTAT, true }, // 0x33
	{ "TOCLIENT_MOVE_PLAYER",              MTSCMC_PHYSICS, true }, // 0x34
	{ "TOCLIENT_ACCESS_DENIED_LEGACY",     MTSCMC_AUTH, true }, // 0x35
	{ "TOCLIENT_FOV",                      MTSCMC_CAMERA, true }, // 0x36
	{ "TOCLIENT_DEATHSCREEN",              MTSCMC_FORMSPEC, true }, // 0x37
	{ "TOCLIENT_MEDIA",                    MTSCMC_MEDIA, true }, // 0x38
	null_command_factory, // 0x39
	{ "TOCLIENT_NODEDEF",                  MTSCMC_INIT, true }, // 0x3A
	null_command_factory, // 0x3B
	{ "TOCLIENT_ANNOUNCE_MEDIA",           MTSCMC_MEDIA, true }, // 0x3C
	{ "TOCLIENT_ITEMDEF",                  MTSCMC_INIT, true }, // 0x3D
	null_command_factory, // 0x3E
	{ "TOCLIENT_PLAY_SOUND",               MTSCMC_AUDIO, true }, // 0x3f (may be sent as unrel too)
	{ "TOCLIENT_STOP_SOUND",               MTSCMC_AUDIO, true }, // 0x40
	{ "TOCLIENT_PRIVILEGES",               MTSCMC_AUTH, true }, // 0x41
	{ "TOCLIENT_INVENTORY_FORMSPEC",       MTSCMC_FORMSPEC, true }, // 0x42
	{ "TOCLIENT_DETACHED_INVENTORY",       MTSCMC_INVENTORY, true }, // 0x43
	{ "TOCLIENT_SHOW_FORMSPEC",            MTSCMC_FORMSPEC, true }, // 0x44
	{ "TOCLIENT_MOVEMENT",                 MTSCMC_PHYSICS, true }, // 0x45
	{ "TOCLIENT_SPAWN_PARTICLE",           MTSCMC_PARTICLE, true }, // 0x46
	{ "TOCLIENT_ADD_PARTICLESPAWNER",      MTSCMC_PARTICLE, true }, // 0x47
	null_command_factory, // 0x48
	{ "TOCLIENT_HUDADD",                   MTSCMC_HUD, true }, // 0x49
	{ "TOCLIENT_HUDRM",                    MTSCMC_HUD, true }, // 0x4a
	{ "TOCLIENT_HUDCHANGE",                MTSCMC_HUD, true }, // 0x4b
	{ "TOCLIENT_HUD_SET_FLAGS",            MTSCMC_HUD, true }, // 0x4c
	{ "TOCLIENT_HUD_SET_PARAM",            MTSCMC_HUD, true }, // 0x4d
	{ "TOCLIENT_BREATH",                   MTSCMC_PLAYERSTAT, true }, // 0x4e
	{ "TOCLIENT_SET_SKY",                  MTSCMC_ENVIRONMENT, true }, // 0x4f
	{ "TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO", MTSCMC_ENVIRONMENT, true }, // 0x50
	{ "TOCLIENT_LOCAL_PLAYER_ANIMATIONS",  MTSCMC_ENTITY, true }, // 0x51
	{ "TOCLIENT_EYE_OFFSET",               MTSCMC_CAMERA, true }, // 0x52
	{ "TOCLIENT_DELETE_PARTICLESPAWNER",   MTSCMC_PARTICLE, true }, // 0x53
	{ "TOCLIENT_CLOUD_PARAMS",             MTSCMC_ENVIRONMENT, true }, // 0x54
	{ "TOCLIENT_FADE_SOUND",               MTSCMC_AUDIO, true }, // 0x55
	{ "TOCLIENT_UPDATE_PLAYER_LIST",       MTSCMC_AUTH, true }, // 0x56
	{ "TOCLIENT_MODCHANNEL_MSG",           MTSCMC_MODCHANNEL, true }, // 0x57
	{ "TOCLIENT_MODCHANNEL_SIGNAL",        MTSCMC_MODCHANNEL, true }, // 0x58
	{ "TOCLIENT_NODEMETA_CHANGED",         MTSCMC_MAP, true }, // 0x59
	{ "TOCLIENT_SET_SUN",                  MTSCMC_ENVIRONMENT, true }, // 0x5a
	{ "TOCLIENT_SET_MOON",                 MTSCMC_ENVIRONMENT, true }, // 0x5b
	{ "TOCLIENT_SET_STARS",                MTSCMC_ENVIRONMENT, true }, // 0x5c
	null_command_factory, // 0x5d
	null_command_factory, // 0x5e
	null_command_factory, // 0x5f
	{ "TOSERVER_SRP_BYTES_S_B",            MTSCMC_AUTH, true }, // 0x60
	{ "TOCLIENT_FORMSPEC_PREPEND",         MTSCMC_FORMSPEC, true }, // 0x61
	{ "TOCLIENT_MINIMAP_MODES",            MTSCMC_HUD, true }, // 0x62
};

//	Legacy channels for 5.3 and earlier.
//	The mapping is different (and more balanced) than the original,
//	but clients shouldn't care.
//	Guideline: 2 for high bandwidth, 0 for high frequency, 1 for the rest
const u8 legacyChannelMap[256] = {
	0, // MTSCMC_DEFAULT
	1, // MTSCMC_HUD
	0, // MTSCMC_AUTH
	0, // MTSCMC_INIT
	2, // MTSCMC_MEDIA
	2, // MTSCMC_MAP
	1, // MTSCMC_INVENTORY
	0, // MTSCMC_ENTITY
	1, // MTSCMC_CHAT
	1, // MTSCMC_CAMERA
	1, // MTSCMC_FORMSPEC
	1, // MTSCMC_PARTICLE
	0, // MTSCMC_PHYSICS
	1, // MTSCMC_ENVIRONMENT
	1, // MTSCMC_AUDIO
	0, // MTSCMC_PLAYERSTAT
	0, // MTSCMC_MODCHANNEL
};
