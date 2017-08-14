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

private:
	u16 dtime;

	bool free_move;
	bool fast_move;
	bool continuous_forward;
	bool always_fly_fast;
	bool aux1_descends;
	
	bool up;
	bool down;
	bool left;
	bool right;

	bool jump;
	bool sneak;
	bool aux1;

	s8 pitch;
	u16 yaw; // 9 bits

	s8 joy_forw;
	s8 joy_sidew;
};

class ControlLog
{
public:
	ControlLog();
private:
	u8 version;
};

#endif
