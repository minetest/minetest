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

struct SkyboxParams
{
	video::SColor bgcolor;
	std::string type;
	std::vector<std::string> textures;
	bool clouds;
	video::SColor day_sky;
	video::SColor day_horizon;
	video::SColor dawn_sky;
	video::SColor dawn_horizon;
	video::SColor night_sky;
	video::SColor night_horizon;
	video::SColor indoors;
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
