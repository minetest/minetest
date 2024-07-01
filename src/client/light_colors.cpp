/*
Minetest
Copyright (C) 2024 Andrey2470T, AndreyT <andreyt2203@gmail.com>

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

#include "light_colors.h"

video::SColor encode_light(u16 light, u8 emissive_light)
{
	// Get components
	u32 day = (light & 0xff);
	u32 night = (light >> 8);
	// Add emissive light
	night += emissive_light * 2.5f;
	if (night > 255)
		night = 255;
	// Since we don't know if the day light is sunlight or
	// artificial light, assume it is artificial when the night
	// light bank is also lit.
	if (day < night)
		day = 0;
	else
		day = day - night;
	u32 sum = day + night;
	// Ratio of sunlight:
	u32 r;
	if (sum > 0)
		r = day * 255 / sum;
	else
		r = 0;
	// Average light:
	float b = (day + night) / 2;
	return video::SColor(r, b, b, b);
}

void get_sunlight_color(video::SColorf *sunlight, u32 daynight_ratio)
{
	f32 rg = daynight_ratio / 1000.0f - 0.04f;
	f32 b = (0.98f * daynight_ratio) / 1000.0f + 0.078f;
	sunlight->r = rg;
	sunlight->g = rg;
	sunlight->b = b;
}

void final_color_blend(video::SColor *result,
		u16 light, u32 daynight_ratio, const video::SColorf &ambientLight)
{
	video::SColorf dayLight;
	get_sunlight_color(&dayLight, daynight_ratio);
	final_color_blend(result,
		encode_light(light, 0), dayLight, ambientLight);
}

void final_color_blend(video::SColor *result,
		const video::SColor &data, const video::SColorf &dayLight,
		const video::SColorf &ambientLight)
{
	static const video::SColorf artificialColor(1.04f, 1.04f, 1.04f);

	video::SColorf c(data);
	f32 n = 1 - c.a;

	f32 r = c.r * (c.a * dayLight.r + n * artificialColor.r) * 2.0f;
	f32 g = c.g * (c.a * dayLight.g + n * artificialColor.g) * 2.0f;
	f32 b = c.b * (c.a * dayLight.b + n * artificialColor.b) * 2.0f;

	// Emphase blue a bit in darker places
	// Each entry of this array represents a range of 8 blue levels
	static const u8 emphase_blue_when_dark[32] = {
		1, 4, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	b += emphase_blue_when_dark[irr::core::clamp((s32) ((r + g + b) / 3 * 255),
		0, 255) / 8] / 255.0f;

	// Add ambient light
	r += ambientLight.r;
	g += ambientLight.g;
	b += ambientLight.b;

	result->setRed(core::clamp((s32)(r * 255.f), 0, 255));
	result->setGreen(core::clamp((s32)(g * 255.f), 0, 255));
	result->setBlue(core::clamp((s32)(b * 255.f), 0, 255));
}
