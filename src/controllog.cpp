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

#include "util/serialize.h"
#include "controllog.h"
#include "log.h"
#include <sstream>
#include <cerrno>
#include <cstring>

ControlLogEntry::ControlLogEntry()
{
}

void ControlLogEntry::setDtime(float a_dtime)
{
	if (a_dtime <= 0.0f) {
		dtime = 0;
		overtime = 0;
	} else {
		u32 new_dtime = 1000 * a_dtime; // milliseconds
		setDtimeU32(new_dtime);
	}
}
float ControlLogEntry::getDtime() const
{
	return getDtimeU32() / 1000.0f;
}

void ControlLogEntry::setDtimeU32(u32 a_dtime)
{
	if (a_dtime < 200) {
		dtime = a_dtime;
		overtime = 0;
	} else {
		dtime = 200;
		overtime = a_dtime - 200;
	}
}
u32 ControlLogEntry::getDtimeU32() const
{
	return dtime + overtime;
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

void ControlLogEntry::serialize(std::ostream &output, u8 flags, const ControlLogEntry *prev) const
{
	if (!prev || (settings != prev->settings)) {
		writeU8(output, 255);
		writeU8(output, settings);
	}
	writeU8(output, dtime);
	if (dtime >= 200) {
		writeU16(output, overtime);
	}
	writeU16(output, keys);
	writeU16(output, yaw_pitch);
	if (flags & 0x01) { // joystick
		writeS8(output, joy_forw);
		writeS8(output, joy_sidew);
	}
	return;
}

void ControlLogEntry::deserialize(std::istream &input, u8 flags, const ControlLogEntry *prev)
{
	u8 i_dtime;
	i_dtime = readU8(input);
	if (i_dtime == 255) {
		settings = readU8(input);
		i_dtime = readU8(input); // start of next entry
	}
	if (i_dtime >= 200) {
		dtime = i_dtime;
		overtime = readU16(input);
	} else {
		dtime = i_dtime;
		overtime = 0;
	}
	keys = readU16(input);
	yaw_pitch = readU16(input);
	if (flags & 0x01) { // joystick
		joy_forw = readS8(input);
		joy_sidew = readS8(input);
	} else {
		joy_forw = joy_sidew = 0;
	}
	return;
}



ControlLog::ControlLog()
{
}

void ControlLog::add(ControlLogEntry &cle)
{
	if (entries.size()) {
		ControlLogEntry &cur = entries.back();
		if (cur.matches(cle)) {
			cur.merge(cle); // in-place
		} else {
			entries.push_back(cle);
		}
	} else {
		entries.push_back(cle);
	}
}

void ControlLog::serialize(std::ostream &output, u32 bytes_max) const
{
	// loop to find flags (with/without joystick)
	u8 flags = 0;

	writeU8(output, version);
	writeU8(output, flags);

	writeU32(output, starttime);

	u8 motion_model = 1;
	u8 motion_model_version = 1;

	bool leftovers = false;
	writeU8(output, motion_model);
	writeU8(output, motion_model_version);
	ControlLogEntry *prev_cle = NULL;
	int count = 0;
	for( ControlLogEntry cle : entries ) {
		if (output.tellp() >= bytes_max) {
			leftovers = true;
			break;
		}
		cle.serialize(output, flags, prev_cle);
		prev_cle = &cle;
		count++;
	}
	if (leftovers) {
		writeU8(output, 253);
	}

	return;
}

void ControlLog::deserialize(std::istream &input)
{
	u8 flags;

	version = readU8(input);
	flags   = readU8(input);

	starttime = readU32(input);
	u8 motion_model, motion_model_version;
	motion_model = readU8(input);
	motion_model_version = readU8(input);
	motion_model = motion_model - motion_model;
	motion_model_version = motion_model_version - motion_model_version;

	//bool leftovers = false;
	//long int prev_tellg = input.tellg();
	ControlLogEntry *prev_cle = NULL;
	int count = 0;
	while (!input.eof()) {
		int code = input.peek();
		if (code < 0) {
			break; // eof
		} else if (code == 253) {
			// leftovers
			u8 marker = readU8(input);
			marker = marker - marker;
		} else {
			ControlLogEntry cle;
			cle.deserialize(input, flags, prev_cle);
			add(cle);
			prev_cle = &cle;
			count++;
		}
	}
	return;
}

u32 ControlLog::getStartTime() const
{
	return starttime;
}

u32 ControlLog::getFinishTime() const
{
	return starttime + getSpannedTime();
}
u32 ControlLog::getSpannedTime() const
{
	u32 spannedtime = 0;
	for( ControlLogEntry cle : entries ) {
		spannedtime += cle.getDtimeU32();
	}
	return spannedtime;
}

void ControlLog::acknowledge(u32 finishtime) {
	dstream << "acknowledge " << finishtime << std::endl;
	while (starttime < finishtime && !entries.empty()) {
		ControlLogEntry cle = entries.front(); entries.pop_front();
		u32 dtime = cle.getDtimeU32();
		if (finishtime - starttime < dtime) {
			// only consume part of this, put the rest back
			cle.setDtimeU32(finishtime - starttime);
			starttime = finishtime;
			// don't acknowledge any more entries
			break;
		} else {
			// eat the entire entry, try the next one.
			starttime += dtime;
		}
	}
	dstream << "new start time " << starttime << std::endl;
}
