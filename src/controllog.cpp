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
#include "log.h"
#include <sstream>

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

u8 ControlLogEntry::serDtime() const
{
	return (u8)dtime;
}
u8 ControlLogEntry::serSettings() const
{
	u8 result = 0;
	if (free_move)
		result = result | 0x01;
	if (fast_move)
		result = result | 0x02;
	if (continuous_forward)
		result = result | 0x04;
	if (always_fly_fast)
		result = result | 0x08;
	if (aux1_descends)
		result = result | 0x10;
	return result;
}
u8 ControlLogEntry::serKeys() const
{
	u8 result = 0;
	if (up)
		result = result | 0x01;
	if (down)
		result = result | 0x02;
	if (left)
		result = result | 0x04;
	if (right)
		result = result | 0x08;
	if (jump)
		result = result | 0x10;
	if (sneak)
		result = result | 0x20;
	if (aux1)
		result = result | 0x40;
	return result;
}
u16 ControlLogEntry::serYawPitch() const
{
	u16 result = 0;
	result = result | (yaw & 0x1ff << 7);
	result = result | (pitch & 0x7f);
	return result;
}
s8 ControlLogEntry::serJoyForw() const
{
	return joy_forw;
}
s8 ControlLogEntry::serJoySidew() const
{
	return joy_sidew;
}
bool ControlLogEntry::matches(const ControlLogEntry &other) const
{
	return (
		free_move == other.free_move &&
		fast_move == other.fast_move &&
		continuous_forward == other.continuous_forward &&
		always_fly_fast == other.always_fly_fast &&
		aux1_descends == other.aux1_descends &&
		up == other.up &&
		down == other.down &&
		left == other.left &&
		right == other.right &&
		jump == other.jump &&
		sneak == other.sneak &&
		pitch == other.pitch &&
		yaw == other.yaw &&
		joy_forw == other.joy_forw &&
		joy_sidew == other.joy_sidew
	);
}
void ControlLogEntry::merge(const ControlLogEntry &other)
{
	dtime = dtime + other.dtime;
	// the rest is supposedly identical
}



ControlLog::ControlLog()
{
}

void ControlLog::add(ControlLogEntry &cle)
{
	if (log.size()) {
		ControlLogEntry &cur = log.back();
		if (cur.matches(cle)) {
			cur.merge(cle); // in-place
		} else {
			log.push_back(cle);
		}
	} else {
		log.push_back(cle);
	}
}

// TODO: operator<<
std::string ControlLog::serialize(u32 bytes_max) const
{
	// loop to find flags (with/without joystick)
	u8 flags = 0;
	// TODO: last sent settings instead? They may change
	//       during the log itself.
	bool settings_included = false;

	std::stringstream output;

	output << version;
	output << flags;

	output << starttime;

	u8 motion_model = 1;
	u8 motion_model_version = 1;
	int count = 0;

	bool leftovers = false;
	output << motion_model << motion_model_version;
	for( ControlLogEntry cle : log ) {
		if (output.tellp() > bytes_max) {
			leftovers = true;
			break;
		}
		if (cle.serSettings() != last_acked_settings && !settings_included) {
			output << (u8)255 << cle.serSettings();
			settings_included = true;
		}
		u16 overtime = 0;
		if (cle.getDtime() > 200) {
			overtime = cle.getDtime() - 200;
			cle.setDtime(200);
		}
		output << cle.serDtime() << cle.serKeys() << cle.serYawPitch();
		count++;
		if (flags & 0x01) { // joystick
			output << cle.serJoyForw() << cle.serJoySidew();
		}
		if (overtime) {
			output << (u8)254 << (u16)overtime;
		}
	}
	if (leftovers) {
		output << (u8)253;
	}

	return output.str();
}

void ControlLog::deserialize(const std::string logbytes)
{
	u8 flags;

	std::stringstream input(logbytes);
	input >> version;
	input >> flags;

	input >> starttime;
	u8 motion_model, motion_model_version;
	input >> motion_model >> motion_model_version;

	//bool leftovers = false;
	int count = 0;
	while ((unsigned)input.tellp() < logbytes.length()) {
		u8 byte;
		input >> byte;
		count++;
	}
	dstream << "deserialized " << count << " bytes" << std::endl;
}

