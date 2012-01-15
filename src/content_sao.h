/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CONTENT_SAO_HEADER
#define CONTENT_SAO_HEADER

#include "serverobject.h"
#include "content_object.h"
#include "collision.h"
#include "environment.h"
#include "settings.h"
#include "main.h" // For g_profiler
#include "profiler.h"
#include "serialization.h" // For compressZlib
#include "materials.h" // For MaterialProperties
#include "tooldef.h" // ToolDiggingProperties

class Settings;

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

void accelerate_xz(v3f &speed, v3f target_speed, f32 max_increase);
bool checkFreePosition(Map *map, v3s16 p0, v3s16 size);
bool checkWalkablePosition(Map *map, v3s16 p0);
bool checkFreeAndWalkablePosition(Map *map, v3s16 p0, v3s16 size);
void get_random_u32_array(u32 a[], u32 len);
void explodeSquare(Map *map, v3s16 p0, v3s16 size);

#endif

