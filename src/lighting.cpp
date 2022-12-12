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

#include "lighting.h"

AutoExposure::AutoExposure()
        : luminance_min(-3.f),
        luminance_max(-3.f),
        exposure_correction(0.0f),
        speed_dark_bright(1000.f),
        speed_bright_dark(1000.f),
        center_weight_power(1.f)
{}
