/*
Minetest
Copyright (C) 2018 Jordach, Jordan Snelling <jordach.snelling@gmail.com>

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

struct sol
{
    bool visible;
    bool sunrise_glow;
    f32 yaw;
    f32 tilt;
    std::string texture;
};

struct luna
{
    bool visible;
    f32 yaw;
    f32 tilt;
    std::string texture;
};

struct star
{
    u16 number;
    bool visible;
    f32 yaw;
    f32 tilt;
};

struct SkyParams
{
	video::SColor bgcolor;
    std::string type;
    std::vector<std::string> params;
    bool clouds;
    bool default_fog;
    bool overlay_visible;
    struct sol sun;
    struct luna moon;
    struct star stars;
    std::vector<std::string> overlay_textures;
};