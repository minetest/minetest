/*
Minetest
Copyright (C) 2017 Ben Deutsch <ben@bendeutsch.de>

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

#ifndef CONTROLLOG_HEADER
#define CONTROLLOG_HEADER

#include "irrlichttypes_bloated.h"
#include <deque>
#include <string>
#include <iostream>

/*

overhead:
- protocol version
- motion model number + version

global settings:
- free_move
- fast_move
- no_clip
- aux1_descends
- always_fly_fast
- continuous_forward ?
= easily 1 byte, but less important

line item settings:
- dtime_ms since last item (8 bits)
- pitch (7bits)
- yaw (9bits)
- control.up
- control.down
- control.left
- control.right
- control.jump
- control.aux1
- control.sneak (7 bits for control so far)
if joystick enabled / detected:
- forw_move_joystick_axis (8 bits)
- side_move_joysticx_axis (8 bits)
= 31 bits w/o joystick, 47 bits with

*/

class ControlLogEntry
{
public:
	ControlLogEntry();

	void setDtime(float dtime);
	float getDtime() const;
	void setDtimeU32(u32 dtime);
	u32 getDtimeU32() const;

	void setFreeMove(bool free_move);
	bool getFreeMove() const;
	void setFastMove(bool fast_move);
	bool getFastMove() const;
	void setContinuousForward(bool continuous_forward);
	bool getContinuousForward() const;
	void setAlwaysFlyFast(bool always_fly_fast);
	bool getAlwaysFlyFast() const;
	void setAux1Descends(bool aux_descends);
	bool getAux1Descends() const;

	void setUp(bool up);
	bool getUp() const;
	void setDown(bool down);
	bool getDown() const;
	void setLeft(bool left);
	bool getLeft() const;
	void setRight(bool right);
	bool getRight() const;

	void setJump(bool jump);
	bool getJump() const;
	void setSneak(bool sneak);
	bool getSneak() const;
	void setAux1(bool aux1);
	bool getAux1() const;

	void setPitch(float pitch);
	float getPitch() const;
	void setYaw(float yaw);
	float getYaw() const;

	void setJoyForw(float joy_forw);
	float getJoyForw() const;
	void setJoySidew(float joy_sidew);
	float getJoySidew() const;

	bool matches(const ControlLogEntry &other) const;
	void merge(const ControlLogEntry &other);

	void serialize(std::ostream &to, u8 flags, const ControlLogEntry *prev) const;
	void deserialize(std::istream &from, u8 flags, const ControlLogEntry *prev);

	u8 serDtime() const;
	u8 serSettings() const;
	u16 serYawPitch() const;
	// TODO: u16 instead of u8, add mouse buttons, future extensibility
	u8 serKeys() const;
	s8 serJoyForw() const;
	s8 serJoySidew() const;

private:
	u8 dtime;
	u16 overtime;

	u8 settings;
	// free_move;
	// fast_move;
	// continuous_forward;
	// always_fly_fast;
	// aux1_descends;
	
	u16 keys;
	// up;
	// down;
	// left;
	// right;
	// jump;
	// sneak;
	// aux1;
	// mouse and future extensions

	u16 yaw_pitch;
	// 7 bits signed:   pitch;
	// 9 bits unsigned: yaw;

	s8 joy_forw;
	s8 joy_sidew;
};

class ControlLog
{
public:
	ControlLog();
	void add(ControlLogEntry &cle); // position, too?
	void serialize(std::ostream &to, u32 bytes=800) const; // up to bytes in length
	void deserialize(std::istream &from);
	u32 getStartTime() const;
	u32 getFinishTime() const;
	u32 getSpannedTime() const;
	void acknowledge(u32 time); // removes entries
	const std::deque<ControlLogEntry> &getEntries() const
	{
		return entries;
	}
private:
	u8 version = 1; // agreed-upon version
	u32 starttime = 0;
	u8 last_acked_settings = 0;
	std::deque<ControlLogEntry> entries;
};

#endif
