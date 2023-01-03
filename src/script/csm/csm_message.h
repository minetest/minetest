/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "irr_v3d.h"
#include "log.h"
#include "mapnode.h"
#include "modchannels.h"
#include "util/pointedthing.h"
#include <stddef.h>
#include <type_traits>

static_assert(std::is_trivially_copyable<PointedThing>::value,
		"PointedThing is not copyable");

// controller -> script

enum CSMC2SMsgType {
	CSM_C2S_INVALID = -1,

	CSM_C2S_RUN_SHUTDOWN,
	CSM_C2S_RUN_CLIENT_READY,
	CSM_C2S_RUN_CAMERA_READY,
	CSM_C2S_RUN_MINIMAP_READY,
	CSM_C2S_RUN_SENDING_MESSAGE,
	CSM_C2S_RUN_RECEIVING_MESSAGE,
	CSM_C2S_RUN_DAMAGE_TAKEN,
	CSM_C2S_RUN_HP_MODIFICATION,
	CSM_C2S_RUN_DEATH,
	CSM_C2S_RUN_MODCHANNEL_MESSAGE,
	CSM_C2S_RUN_MODCHANNEL_SIGNAL,
	CSM_C2S_RUN_FORMSPEC_INPUT,
	CSM_C2S_RUN_INVENTORY_OPEN,
	CSM_C2S_RUN_ITEM_USE,
	CSM_C2S_RUN_PLACENODE,
	CSM_C2S_RUN_PUNCHNODE,
	CSM_C2S_RUN_DIGNODE,
	CSM_C2S_RUN_STEP,
};

struct CSMC2SRunDamageTaken {
	CSMC2SMsgType type = CSM_C2S_RUN_DAMAGE_TAKEN;
	u16 damage;
};

struct CSMC2SRunHPModification {
	CSMC2SMsgType type = CSM_C2S_RUN_HP_MODIFICATION;
	u16 hp;
};

struct CSMC2SRunModchannelMessage {
	CSMC2SMsgType type = CSM_C2S_RUN_MODCHANNEL_MESSAGE;
	size_t channel_size, sender_size, message_size;
	// string channel, sender, and message follow
};

struct CSMC2SRunModchannelSignal {
	CSMC2SMsgType type = CSM_C2S_RUN_MODCHANNEL_SIGNAL;
	ModChannelSignal signal;
	// string channel follows
};

struct CSMC2SRunItemUse {
	CSMC2SMsgType type = CSM_C2S_RUN_ITEM_USE;
	PointedThing pointed_thing;
	// selected item string follows
};

struct CSMC2SRunPlacenode {
	CSMC2SMsgType type = CSM_C2S_RUN_PLACENODE;
	PointedThing pointed_thing;
	// item name follows
};

struct CSMC2SRunPunchnode {
	CSMC2SMsgType type = CSM_C2S_RUN_PUNCHNODE;
	v3s16 pos;
	MapNode n;
};

struct CSMC2SRunDignode {
	CSMC2SMsgType type = CSM_C2S_RUN_DIGNODE;
	v3s16 pos;
	MapNode n;
};

struct CSMC2SRunStep {
	CSMC2SMsgType type = CSM_C2S_RUN_STEP;
	float dtime;
};

struct CSMC2SGetNode {
	MapNode n;
	bool pos_ok;
};

// script -> controller

enum CSMS2CMsgType {
	CSM_S2C_INVALID = -1,

	CSM_S2C_DONE,
	CSM_S2C_LOG,
	CSM_S2C_GET_NODE,
	CSM_S2C_ADD_NODE,
};

struct CSMS2CDoneBool {
	CSMS2CMsgType type = CSM_S2C_DONE;
	char value;
};

struct CSMS2CLog {
	CSMS2CMsgType type = CSM_S2C_LOG;
	LogLevel level;
	// message string follows
};

struct CSMS2CGetNode {
	CSMS2CMsgType type = CSM_S2C_GET_NODE;
	v3s16 pos;
};

struct CSMS2CAddNode {
	CSMS2CMsgType type = CSM_S2C_ADD_NODE;
	MapNode n;
	v3s16 pos;
	char remove_metadata = true;
};
