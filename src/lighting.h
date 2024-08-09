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

#include "irr_v3d.h"

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

/**
 * Parameters for set color and intensity of night and day light.
 *
 * Light color is calculated in function get_sunlight_color.
 * Variable daynight_ration can be from 0 to 1000.
 *
 * sunlight->r = colorOffset_rgb.X+colorRatioCoef_rgb.X*daynight_ratio;
 * sunlight->g = colorOffset_rgb.Y+colorRatioCoef_rgb.Y*daynight_ratio;
 * sunlight->b = colorOffset_rgb.Z+colorRatioCoef_rgb.Z*daynight_ratio;
 *
 */
struct LightIntensity
{
	/// @brief Sunlight color offset
	v3f colorOffset_rgb;
	/// @brief Sunlight color dayratio effect
	v3f colorRatioCoef_rgb;
	
	LightIntensity();
};

/** Describes ambient light settings for a player
 */
struct Lighting
{
	AutoExposure exposure;
	LightIntensity lightIntensity;
	float shadow_intensity {0.0f};
	float saturation {1.0f};
	float volumetric_light_strength {0.0f};
};
