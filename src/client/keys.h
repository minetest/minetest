/*
Minetest
Copyright (C) 2016 est31, <MTest31@outlook.com>

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

#ifndef KEYS_HEADER
#define KEYS_HEADER

#include <list>

class KeyType
{
public:
	enum T
	{
		// Player movement
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		JUMP,
		SPECIAL1,
		SNEAK,
		AUTORUN,

		ESC,

		// Other
		DROP,
		INVENTORY,
		CHAT,
		CMD,
		CMD_LOCAL,
		CONSOLE,
		MINIMAP,
		FREEMOVE,
		FASTMOVE,
		NOCLIP,
		CINEMATIC,
		SCREENSHOT,
		TOGGLE_HUD,
		TOGGLE_CHAT,
		TOGGLE_FORCE_FOG_OFF,
		TOGGLE_UPDATE_CAMERA,
		TOGGLE_DEBUG,
		TOGGLE_PROFILER,
		CAMERA_MODE,
		INCREASE_VIEWING_RANGE,
		DECREASE_VIEWING_RANGE,
		RANGESELECT,
		ZOOM,

		QUICKTUNE_NEXT,
		QUICKTUNE_PREV,
		QUICKTUNE_INC,
		QUICKTUNE_DEC,

		DEBUG_STACKS,

		// joystick specific keys
		MOUSE_L,
		MOUSE_R,
		SCROLL_UP,
		SCROLL_DOWN,

		// Fake keycode for array size and internal checks
		INTERNAL_ENUM_COUNT

	};
};

typedef KeyType::T GameKeyType;

#endif
