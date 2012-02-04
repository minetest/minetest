/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef LIGHT_HEADER
#define LIGHT_HEADER

#include "common_irrlicht.h"
#include "debug.h"

/*
	Day/night cache:
	Meshes are cached for different day-to-night transition values
*/

/*#define DAYNIGHT_CACHE_COUNT 3
// First one is day, last one is night.
extern u32 daynight_cache_ratios[DAYNIGHT_CACHE_COUNT];*/

/*
	Lower level lighting stuff
*/

// This directly sets the range of light.
// Actually this is not the real maximum, and this is not the
// brightest. The brightest is LIGHT_SUN.
#define LIGHT_MAX 14
// Light is stored as 4 bits, thus 15 is the maximum.
// This brightness is reserved for sunlight
#define LIGHT_SUN 15

inline u8 diminish_light(u8 light)
{
	if(light == 0)
		return 0;
	if(light >= LIGHT_MAX)
		return LIGHT_MAX - 1;
		
	return light - 1;
}

inline u8 diminish_light(u8 light, u8 distance)
{
	if(distance >= light)
		return 0;
	return  light - distance;
}

inline u8 undiminish_light(u8 light)
{
	// We don't know if light should undiminish from this particular 0.
	// Thus, keep it at 0.
	if(light == 0)
		return 0;
	if(light == LIGHT_MAX)
		return light;
	
	return light + 1;
}

extern u8 light_decode_table[LIGHT_MAX+1];

// 0 <= light <= LIGHT_SUN
// 0 <= return value <= 255
inline u8 decode_light(u8 light)
{
	if(light > LIGHT_MAX)
		light = LIGHT_MAX;
	
	return light_decode_table[light];
}

// 0 <= daylight_factor <= 1000
// 0 <= lightday, lightnight <= LIGHT_SUN
// 0 <= return value <= LIGHT_SUN
inline u8 blend_light(u32 daylight_factor, u8 lightday, u8 lightnight)
{
	u32 c = 1000;
	u32 l = ((daylight_factor * lightday + (c-daylight_factor) * lightnight))/c;
	if(l > LIGHT_SUN)
		l = LIGHT_SUN;
	return l;
}

#endif

