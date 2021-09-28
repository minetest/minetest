/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes.h"

class MtEvent
{
public:
	enum Type : u8
	{
		VIEW_BOBBING_STEP = 0,
		CAMERA_PUNCH_LEFT,
		CAMERA_PUNCH_RIGHT,
		PLAYER_FALLING_DAMAGE,
		PLAYER_DAMAGE,
		NODE_DUG,
		PLAYER_JUMP,
		PLAYER_REGAIN_GROUND,
		TYPE_MAX,
	};

	virtual ~MtEvent() = default;
	virtual Type getType() const = 0;
};

// An event with no parameters and customizable name
class SimpleTriggerEvent : public MtEvent
{
	Type type;

public:
	SimpleTriggerEvent(Type type) : type(type) {}
	Type getType() const override { return type; }
};

class MtEventReceiver
{
public:
	virtual ~MtEventReceiver() = default;
	virtual void onEvent(MtEvent *e) = 0;
};

typedef void (*event_receive_func)(MtEvent *e, void *data);

class MtEventManager
{
public:
	virtual ~MtEventManager() = default;
	virtual void put(MtEvent *e) = 0;
	virtual void reg(MtEvent::Type type, event_receive_func f, void *data) = 0;
	// If data==NULL, every occurence of f is deregistered.
	virtual void dereg(MtEvent::Type type, event_receive_func f, void *data) = 0;
};
