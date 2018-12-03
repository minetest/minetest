/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "serverobject.h"

struct PlayerHPChangeReason {
	enum Type : u8 {
		SET_HP,
		PLAYER_PUNCH,
		FALL,
		NODE_DAMAGE,
		DROWNING,
		RESPAWN
	};

	Type type = SET_HP;
	ServerActiveObject *object;
	bool from_mod = false;
	int lua_reference = -1;

	bool setTypeFromString(const std::string &typestr)
	{
		if (typestr == "set_hp")
			type = SET_HP;
		else if (typestr == "punch")
			type = PLAYER_PUNCH;
		else if (typestr == "fall")
			type = FALL;
		else if (typestr == "node_damage")
			type = NODE_DAMAGE;
		else if (typestr == "drown")
			type = DROWNING;
		else if (typestr == "respawn")
			type = RESPAWN;
		else
			return false;

		return true;
	}

	std::string getTypeAsString() const
	{
		switch (type) {
		case PlayerHPChangeReason::SET_HP:
			return "set_hp";
		case PlayerHPChangeReason::PLAYER_PUNCH:
			return "punch";
		case PlayerHPChangeReason::FALL:
			return "fall";
		case PlayerHPChangeReason::NODE_DAMAGE:
			return "node_damage";
		case PlayerHPChangeReason::DROWNING:
			return "drown";
		case PlayerHPChangeReason::RESPAWN:
			return "respawn";
		default:
			return "?";
		}
	}

	PlayerHPChangeReason(Type type, ServerActiveObject *object=NULL):
			type(type), object(object)
	{}
};

