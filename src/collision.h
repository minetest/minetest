/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef COLLISION_HEADER
#define COLLISION_HEADER

#include "common_irrlicht.h"

class Map;

struct collisionMoveResult
{
	bool touching_ground;

	collisionMoveResult():
		touching_ground(false)
	{}
};

collisionMoveResult collisionMoveSimple(Map *map, f32 pos_max_d,
		const core::aabbox3d<f32> &box_0,
		f32 dtime, v3f &pos_f, v3f &speed_f);
//{return collisionMoveResult();}

enum CollisionType
{
	COLLISION_FALL
};

struct CollisionInfo
{
	CollisionType t;
	f32 speed;
};

#endif

