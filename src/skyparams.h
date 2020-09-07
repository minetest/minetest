/*
Minetest
Copyright (C) 2019 Jordach, Jordan Snelling <jordach.snelling@gmail.com>

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

struct SkyColor
{
	video::SColor day_sky;
	video::SColor day_horizon;
	video::SColor dawn_sky;
	video::SColor dawn_horizon;
	video::SColor night_sky;
	video::SColor night_horizon;
	video::SColor indoors;
};

struct SkyboxParams
{
	video::SColor bgcolor;
	std::string type;
	std::vector<std::string> textures;
	bool clouds;
	SkyColor sky_color;
	video::SColor sun_tint;
	video::SColor moon_tint;
	std::string tint_type;
};

struct SunParams
{
	bool visible;
	std::string texture;
	std::string tonemap;
	std::string sunrise;
	bool sunrise_visible;
	f32 scale;
};

struct MoonParams
{
	bool visible;
	std::string texture;
	std::string tonemap;
	f32 scale;
};

struct StarParams
{
	bool visible;
	u32 count;
	video::SColor starcolor;
	f32 scale;
};

// Utility class for setting default sky, sun, moon, stars values:
class SkyboxDefaults
{
public:
	const SkyColor getSkyColorDefaults()
	{
		SkyColor sky;
		// Horizon colors
		sky.day_horizon = video::SColor(255, 155, 193, 240);
		sky.indoors = video::SColor(255, 100, 100, 100);
		sky.dawn_horizon = video::SColor(255, 186, 193, 240);
		sky.night_horizon = video::SColor(255, 64, 144, 255);
		// Sky colors
		sky.day_sky = video::SColor(255, 140, 186, 250);
		sky.dawn_sky = video::SColor(255, 180, 186, 250);
		sky.night_sky = video::SColor(255, 0, 107, 255);
		return sky;
	}

	const SunParams getSunDefaults()
	{
		SunParams sun;
		sun.visible = true;
		sun.sunrise_visible = true;
		sun.texture = "sun.png";
		sun.tonemap = "sun_tonemap.png";
		sun.sunrise = "sunrisebg.png";
		sun.scale = 1;
		return sun;
	}

	const MoonParams getMoonDefaults()
	{
		MoonParams moon;
		moon.visible = true;
		moon.texture = "moon.png";
		moon.tonemap = "moon_tonemap.png";
		moon.scale = 1;
		return moon;
	}

	const StarParams getStarDefaults()
	{
		StarParams stars;
		stars.visible = true;
		stars.count = 1000;
		stars.starcolor = video::SColor(105, 235, 235, 255);
		stars.scale = 1;
		return stars;
	}
};
