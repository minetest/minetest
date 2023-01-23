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
#include "util/string.h"
#include <stddef.h>
#include <type_traits>
#include <tuple>
#include <utility>

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

using CSMC2SRunSendingMessage = std::pair<CSMC2SMsgType, std::string>;

using CSMC2SRunReceivingMessage = std::pair<CSMC2SMsgType, std::string>;

using CSMC2SRunDamageTaken = std::pair<CSMC2SMsgType, u16>;

using CSMC2SRunHPModification = std::pair<CSMC2SMsgType, u16>;

using CSMC2SRunModchannelMessage = std::tuple<CSMC2SMsgType,
		std::string, std::string, std::string>;

using CSMC2SRunModchannelSignal = std::tuple<CSMC2SMsgType, std::string, ModChannelSignal>;

using CSMC2SRunFormspecInput = std::tuple<CSMC2SMsgType, std::string, StringMap>;

using CSMC2SRunInventoryOpen = std::pair<CSMC2SMsgType, std::string>;

using CSMC2SRunItemUse = std::tuple<CSMC2SMsgType, std::string, PointedThing>;

using CSMC2SRunPlacenode = std::tuple<CSMC2SMsgType, PointedThing, std::string>;

using CSMC2SRunPunchnode = std::tuple<CSMC2SMsgType, v3s16, MapNode>;

using CSMC2SRunDignode = std::tuple<CSMC2SMsgType, v3s16, MapNode>;

using CSMC2SRunStep = std::pair<CSMC2SMsgType, float>;

using CSMC2SGetNode = std::pair<MapNode, bool>;

// script -> controller

enum CSMS2CMsgType {
	CSM_S2C_INVALID = -1,

	CSM_S2C_DONE,
	CSM_S2C_TERMINATED,
	CSM_S2C_LOG,
	CSM_S2C_GET_NODE,
	CSM_S2C_ADD_NODE,
	CSM_S2C_NODE_META_CLEAR,
	CSM_S2C_NODE_META_CONTAINS,
	CSM_S2C_NODE_META_SET_STRING,
	CSM_S2C_NODE_META_GET_STRINGS,
	CSM_S2C_NODE_META_GET_KEYS,
	CSM_S2C_NODE_META_GET_STRING,
};

using CSMS2CDoneBool = std::pair<CSMS2CMsgType, char>;

using CSMS2CLog = std::tuple<CSMS2CMsgType, LogLevel, std::string>;

using CSMS2CGetNode = std::pair<CSMS2CMsgType, v3s16>;

using CSMS2CAddNode = std::tuple<CSMS2CMsgType, v3s16, MapNode, char>;

using CSMS2CNodeMetaClear = std::pair<CSMS2CMsgType, v3s16>;

using CSMS2CNodeMetaContains = std::tuple<CSMS2CMsgType, v3s16, std::string>;

using CSMS2CNodeMetaSetString = std::tuple<CSMS2CMsgType, v3s16, std::string, std::string>;

using CSMS2CNodeMetaGetStrings = std::pair<CSMS2CMsgType, v3s16>;

using CSMS2CNodeMetaGetKeys = std::pair<CSMS2CMsgType, v3s16>;

using CSMS2CNodeMetaGetString = std::tuple<CSMS2CMsgType, v3s16, std::string>;
