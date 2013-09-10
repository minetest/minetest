/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef LIGHT_HEADER
#define LIGHT_HEADER

#include "irrlichttypes.h"

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

// 0.0 <= light <= 1.0
// 0.0 <= return value <= 1.0
inline float decode_light_f(float light_f)
{
	s32 i = (u32)(light_f * LIGHT_MAX + 0.5);

	if(i <= 0)
		return (float)light_decode_table[0] / 255.0;
	if(i >= LIGHT_MAX)
		return (float)light_decode_table[LIGHT_MAX] / 255.0;

	float v1 = (float)light_decode_table[i-1] / 255.0;
	float v2 = (float)light_decode_table[i] / 255.0;
	float f0 = (float)i - 0.5;
	float f = light_f * LIGHT_MAX - f0;
	return f * v2 + (1.0 - f) * v1;
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

// 0.0 <= daylight_factor <= 1.0
// 0 <= lightday, lightnight <= LIGHT_SUN
// 0 <= return value <= 255
inline u8 blend_light_f1(float daylight_factor, u8 lightday, u8 lightnight)
{
	u8 l = ((daylight_factor * decode_light(lightday) +
			(1.0-daylight_factor) * decode_light(lightnight)));
	return l;
}

#endif

