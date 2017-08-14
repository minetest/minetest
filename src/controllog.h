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

private:
};

class ControlLog
{
public:
	ControlLog();
private:
	unsigned char version;
};

#endif
