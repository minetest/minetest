/*
Minetest
Copyright (C) 2021 x2048, Dmitry Kostenko <codeforsmile@gmail.com>

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

#pragma once
#include "irrlichttypes_bloated.h"


/**
 * Parameters for automatic exposure compensation
 *
 * Automatic exposure compensation uses the following equation:
 *
 * wanted_exposure = 2^exposure_correction / clamp(observed_luminance, 2^luminance_min, 2^luminance_max)
 *
 */
struct AutoExposure
{
	/// @brief Minimum boundary for computed luminance
	float luminance_min;
	/// @brief Maximum boundary for computed luminance
	float luminance_max;
	/// @brief Luminance bias. Higher values make the scene darker, can be negative.
	float exposure_correction;
	/// @brief Speed of transition from dark to bright scenes
	float speed_dark_bright;
	/// @brief Speed of transition from bright to dark scenes
	float speed_bright_dark;
	/// @brief Power value for center-weighted metering. Value of 1.0 measures entire screen uniformly
	float center_weight_power;

	AutoExposure();
};

/*
 * Parameters for adjusting ambient light affecting on colors of map nodes and entities
 */
struct AmbientLight
{
	AmbientLight() : luminance(0), color(255, 255, 255, 255) {}
	/// @brief Minimal threshold of luminance of ambience. Can be from 0 - 14.
	u8 luminance;

	/// @brief Color of ambient light. Default is white color.
	video::SColor color;
};

/** Describes ambient light settings for a player
 */
struct Lighting
{
	AutoExposure exposure;
	AmbientLight ambient_light;
	float shadow_intensity {0.0f};
	float saturation {1.0f};
	float volumetric_light_strength {0.0f};
};
