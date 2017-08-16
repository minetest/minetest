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
#include "settings.h"

#ifndef SERVER

// Length of LIGHT_MAX+1 means LIGHT_MAX is the last value.
// LIGHT_SUN is read as LIGHT_MAX from here.

u8 light_LUT[LIGHT_MAX+1];

// the const ref to light_LUT is what is actually used in the code.
const u8 *light_decode_table = light_LUT;

/** Initialize or update the light value tables using the specified \p gamma.
 */
void set_light_table(float gamma)
{
// lighting curve derivatives
	const float alpha = g_settings->getFloat("lighting_alpha");
	const float beta  = g_settings->getFloat("lighting_beta");
// lighting curve coefficients
	const float a = alpha + beta - 2;
	const float b = 3 - 2 * alpha - beta;
	const float c = alpha;
// gamma correction
	gamma = rangelim(gamma, 0.5, 3.0);

	for (size_t i = 0; i < LIGHT_MAX; i++) {
		float x = i;
		x /= LIGHT_MAX;
		float brightness = a * x * x * x + b * x * x + c * x;
		brightness = powf(brightness, 1.0 / gamma);
		light_LUT[i] = rangelim((u32)(255 * brightness), 0, 255);
		if (i > 1 && light_LUT[i] <= light_LUT[i - 1])
			light_LUT[i] = light_LUT[i - 1] + 1;
	}
	light_LUT[LIGHT_MAX] = 255;
}
#endif

