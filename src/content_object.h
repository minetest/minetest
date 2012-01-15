/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CONTENT_OBJECT_HEADER
#define CONTENT_OBJECT_HEADER

#define ACTIVEOBJECT_TYPE_TEST		0x01
#define ACTIVEOBJECT_TYPE_ITEM		0x02
#define ACTIVEOBJECT_TYPE_RAT		0x03
#define ACTIVEOBJECT_TYPE_OERKKI1	0x04
#define ACTIVEOBJECT_TYPE_FIREFLY	0x05
#define ACTIVEOBJECT_TYPE_MOBV2		0x06
#define ACTIVEOBJECT_TYPE_LUAENTITY 0x07

// Special type, not stored as a static object
#define ACTIVEOBJECT_TYPE_PLAYER    0x64

struct AO_Message_type {
	static const u8 SetPosition      = 0x00;
	static const u8 SetTextureMod    = 0x01;
	static const u8 SetSprite        = 0x02;
	static const u8 Punched          = 0x03;
	static const u8 TakeDamage       = 0x04;
	static const u8 Shoot            = 0x05;
	static const u8 Link             = 0x06;
	static const u8 UnLink           = 0x07;
};

#endif

