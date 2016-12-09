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

#include "light.h"
#include <math.h>
#include "util/numeric.h"

#ifndef SERVER

// Length of LIGHT_MAX+1 means LIGHT_MAX is the last value.
// LIGHT_SUN is read as LIGHT_MAX from here.

u8 light_LUT[LIGHT_MAX+1];

// the const ref to light_LUT is what is actually used in the code.
const u8 *light_decode_table = light_LUT;

/** Initialize or update the light value tables using the specified \p gamma.
 *  If \p gamma == 1.0 then the light table is linear.  Typically values for
 *  gamma range between 1.8 and 2.2.
 *
 *  @note The value for gamma will be restricted to the range 1.1 <= gamma <= 3.0.
 *
 *  @note This function is not, currently, a simple linear to gamma encoding
 *        because adjustments are made so that a gamma of 1.8 gives the same
 *        results as those hardcoded for use by the server.
 */
void set_light_table(float gamma)
{
	static const float brightness_step = 255.0f / (LIGHT_MAX + 1);

	// this table is pure arbitrary values, made so that
	// at gamma 2.2 the game looks not too dark at light=1,
	// and mostly linear for the rest of the scale.
	// we could try to inverse the gamma power function, but this
	// is simpler and quicker.
	static const int adjustments[LIGHT_MAX + 1] = {
		-67,
		-91,
		-125,
		-115,
		-104,
		-85,
		-70,
		-63,
		-56,
		-49,
		-42,
		-35,
		-28,
		-22,
		0
	};

	gamma = rangelim(gamma, 1.0, 3.0);

	float brightness = brightness_step;

	for (size_t i = 0; i < LIGHT_MAX; i++) {
		light_LUT[i] = (u8)(255 * powf(brightness / 255.0f, 1.0 / gamma));
		light_LUT[i] = rangelim(light_LUT[i] + adjustments[i], 0, 255);
		if (i > 1 && light_LUT[i] < light_LUT[i - 1])
			light_LUT[i] = light_LUT[i - 1] + 1;
		brightness += brightness_step;
	}
	light_LUT[LIGHT_MAX] = 255;
}
#endif

