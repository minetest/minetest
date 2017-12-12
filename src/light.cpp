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
#include <cmath>
#include "util/numeric.h"
#include "settings.h"

#ifndef SERVER

static u8 light_LUT[LIGHT_SUN + 1];

// The const ref to light_LUT is what is actually used in the code
const u8 *light_decode_table = light_LUT;

// Initialize or update the light value tables using the specified gamma
void set_light_table(float gamma)
{
// Lighting curve derivatives
	const float alpha = g_settings->getFloat("lighting_alpha");
	const float beta  = g_settings->getFloat("lighting_beta");
// Lighting curve coefficients
	const float a = alpha + beta - 2.0f;
	const float b = 3.0f - 2.0f * alpha - beta;
	const float c = alpha;
// Mid boost
	const float d = g_settings->getFloat("lighting_boost");
	const float e = g_settings->getFloat("lighting_boost_center");
	const float f = g_settings->getFloat("lighting_boost_spread");
// Gamma correction
	gamma = rangelim(gamma, 0.5f, 3.0f);

	for (size_t i = 0; i < LIGHT_SUN; i++) {
		float x = i;
		x /= LIGHT_SUN;
		float brightness = a * x * x * x + b * x * x + c * x;
		float boost = d * std::exp(-((x - e) * (x - e)) / (2.0f * f * f));
		brightness = powf(brightness + boost, 1.0f / gamma);
		light_LUT[i] = rangelim((u32)(255.0f * brightness), 0, 255);
		if (i > 1 && light_LUT[i] <= light_LUT[i - 1])
			light_LUT[i] = light_LUT[i - 1] + 1;
	}
	light_LUT[LIGHT_SUN] = 255;
}
#endif
