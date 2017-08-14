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

#include "controllog.h"

ControlLog::ControlLog()
{
}

ControlLogEntry::ControlLogEntry()
{
}

void ControlLogEntry::setDtime(float a_dtime)
{
	if (a_dtime <= 0.0f) {
		dtime = 0;
	} else {
		dtime = 1000 * a_dtime; // milliseconds
	}
}
float ControlLogEntry::getDtime() const
{
	return dtime / 1000.0f;
}

void ControlLogEntry::setFreeMove(bool a_free_move)
{
	free_move = a_free_move;
}
bool ControlLogEntry::getFreeMove() const
{
	return free_move;
}

void ControlLogEntry::setFastMove(bool a_fast_move)
{
	fast_move = a_fast_move;
}
bool ControlLogEntry::getFastMove() const
{
	return fast_move;
}

void ControlLogEntry::setContinuousForward(bool a_continuous_forward)
{
	continuous_forward = a_continuous_forward;
}
bool ControlLogEntry::getContinuousForward() const
{
	return continuous_forward;
}

void ControlLogEntry::setAlwaysFlyFast(bool a_always_fly_fast)
{
	always_fly_fast = a_always_fly_fast;
}
bool ControlLogEntry::getAlwaysFlyFast() const
{
	return always_fly_fast;
}

void ControlLogEntry::setAux1Descends(bool a_aux1_descends)
{
	aux1_descends = a_aux1_descends;
}
bool ControlLogEntry::getAux1Descends() const
{
	return aux1_descends;
}

void ControlLogEntry::setUp(bool a_up)
{
	up = a_up;
}
bool ControlLogEntry::getUp() const
{
	return up;
}

void ControlLogEntry::setDown(bool a_down)
{
	down = a_down;
}
bool ControlLogEntry::getDown() const
{
	return down;
}

void ControlLogEntry::setLeft(bool a_left)
{
	left = a_left;
}
bool ControlLogEntry::getLeft() const
{
	return left;
}

void ControlLogEntry::setRight(bool a_right)
{
	right = a_right;
}
bool ControlLogEntry::getRight() const
{
	return right;
}

void ControlLogEntry::setJump(bool a_jump)
{
	jump = a_jump;
}
bool ControlLogEntry::getJump() const
{
	return jump;
}

void ControlLogEntry::setSneak(bool a_sneak)
{
	sneak = a_sneak;
}
bool ControlLogEntry::getSneak() const
{
	return sneak;
}

void ControlLogEntry::setAux1(bool a_aux1)
{
	aux1 = a_aux1;
}
bool ControlLogEntry::getAux1() const
{
	return aux1;
}

void ControlLogEntry::setPitch(float a_pitch)
{
	// -63 - 63 (7 bits)
	pitch = (s8)((a_pitch / 90.0f) * 63);	
}
float ControlLogEntry::getPitch() const
{
	return (pitch / 63.0f) * 90.0f;
}

void ControlLogEntry::setYaw(float a_yaw)
{
	// 0 - 511 (9 bits)
	yaw = (u16)((a_yaw / 360.0f) * 512) & 0x1ff;
}
float ControlLogEntry::getYaw() const
{
	return (yaw / 512.f) * 360.0f;
}

void ControlLogEntry::setJoyForw(float a_joy_forw)
{
	joy_forw = (s8)(a_joy_forw * 127);
}
float ControlLogEntry::getJoyForw() const
{
	return joy_forw / 127.0f;
}
void ControlLogEntry::setJoySidew(float a_joy_sidew)
{
	joy_sidew = (s8)(a_joy_sidew * 127);
}
float ControlLogEntry::getJoySidew() const
{
	return joy_sidew / 127.0f;
}
