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
		overtime = 0;
	} else {
		u16 new_dtime = 1000 * a_dtime; // milliseconds
		if (new_dtime < 200) {
			dtime = new_dtime;
			overtime = 0;
		} else {
			dtime = 200;
			overtime = new_dtime - 200;
		}
	}
}
float ControlLogEntry::getDtime() const
{
	return ((int)dtime + (int)overtime) / 1000.0f;
}

#define CLE_ACCESSOR(attribute, name, flag) \
void ControlLogEntry::set ## name (bool on)\
{\
	attribute = on ? attribute | flag : attribute & ~flag;\
}\
bool ControlLogEntry::get ## name () const\
{\
	return (attribute & flag) ? true : false;\
}

CLE_ACCESSOR(settings, FreeMove,          0x01)
CLE_ACCESSOR(settings, FastMove,          0x02)
CLE_ACCESSOR(settings, ContinuousForward, 0x04)
CLE_ACCESSOR(settings, AlwaysFlyFast,     0x08)
CLE_ACCESSOR(settings, Aux1Descends,      0x10)

CLE_ACCESSOR(keys, Up,    0x01)
CLE_ACCESSOR(keys, Down,  0x02)
CLE_ACCESSOR(keys, Left,  0x04)
CLE_ACCESSOR(keys, Right, 0x08)
CLE_ACCESSOR(keys, Jump,  0x10)
CLE_ACCESSOR(keys, Sneak, 0x20)
CLE_ACCESSOR(keys, Aux1,  0x40)

#undef CLE_ACCESSOR

void ControlLogEntry::setPitch(float a_pitch)
{
	// -63 - 63 -> 0-126 (7 bits)
	u8 pitch = (u8)((a_pitch / 90.0f) * 63 + 63);
	yaw_pitch = (yaw_pitch & 0xff80) | pitch;
}
float ControlLogEntry::getPitch() const
{
	u8 pitch = (u8)(yaw_pitch & 0x7f);
	return (((int)pitch - 63) / 63.0f) * 90.0f;
}

void ControlLogEntry::setYaw(float a_yaw)
{
	// 0 - 511 (9 bits)
	u16 yaw = (u16)((a_yaw / 360.0f) * 512) & 0x1ff;
	yaw_pitch = (yaw_pitch & 0x7f) | ( yaw << 7);
}
float ControlLogEntry::getYaw() const
{
	u16 yaw = (u16)((yaw_pitch >> 7) & 0x1ff);
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

bool ControlLogEntry::matches(const ControlLogEntry &other) const
{
	return (
		settings == other.settings &&
		keys == other.keys &&
		yaw_pitch == other.yaw_pitch &&
		joy_forw == other.joy_forw &&
		joy_sidew == other.joy_sidew
	);
}
void ControlLogEntry::merge(const ControlLogEntry &other)
{
	u16 new_dtime = dtime + overtime + other.dtime + other.overtime;
	if (new_dtime > 200) {
		dtime = 200;
		overtime = new_dtime - 200;
	} else {
		dtime = new_dtime;
		overtime = 0;
	}
	// the rest is supposedly identical
}

std::string ControlLogEntry::serialize(u8 flags, const ControlLogEntry *prev) const
{
	std::stringstream output;
	if (!prev || (settings != prev->settings)) {
		output << (u8)255 << settings;
	}
	output << dtime;
	if (dtime >= 200) {
		output << overtime;
	}
	output << keys << yaw_pitch;
	if (flags & 0x01) { // joystick
		output << joy_forw << joy_sidew;
	}
	return output.str();
}

void ControlLogEntry::deserialize(const std::string &from, u8 flags, const ControlLogEntry *prev)
{
	std::stringstream input(from);
	u8 i_dtime;
	input >> i_dtime;
	if (i_dtime == 255) {
		input >> settings;
		input >> i_dtime; // start of next entry
	}
	if (i_dtime >= 200) {
		dtime = i_dtime;
		input >> overtime;
	} else {
		dtime = i_dtime;
		overtime = 0;
	}
	input >> keys >> yaw_pitch;
	if (flags & 0x01) { // joystick
		input >> joy_forw >> joy_sidew;
	}
	return;
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

	std::stringstream output;

	output << version;
	output << flags;

	output << starttime;

	u8 motion_model = 1;
	u8 motion_model_version = 1;

	bool leftovers = false;
	output << motion_model << motion_model_version;
	ControlLogEntry *prev_cle = NULL;
	for( ControlLogEntry cle : log ) {
		if (output.tellp() >= bytes_max) {
			leftovers = true;
			break;
		}
		output << cle.serialize(flags, prev_cle);
		prev_cle = &cle;
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
		u8 dtime;
		input >> dtime;
		if (dtime <= 200) {
			ControlLogEntry cle;
		}
	}
	dstream << "deserialized " << count << " bytes" << std::endl;
}

