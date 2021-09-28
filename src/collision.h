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

#pragma once

#include "irrlichttypes_bloated.h"
#include <vector>

class Map;
class IGameDef;
class Environment;
class ActiveObject;

enum CollisionType
{
	COLLISION_NODE,
	COLLISION_OBJECT,
};

enum CollisionAxis
{
	COLLISION_AXIS_NONE = -1,
	COLLISION_AXIS_X,
	COLLISION_AXIS_Y,
	COLLISION_AXIS_Z,
};

struct CollisionInfo
{
	CollisionInfo() = default;

	CollisionType type = COLLISION_NODE;
	CollisionAxis axis = COLLISION_AXIS_NONE;
	v3s16 node_p = v3s16(-32768,-32768,-32768); // COLLISION_NODE
	ActiveObject *object = nullptr; // COLLISION_OBJECT
	v3f old_speed;
	v3f new_speed;
	int plane = -1;
};

struct collisionMoveResult
{
	collisionMoveResult() = default;

	bool touching_ground = false;
	bool collides = false;
	bool standing_on_object = false;
	std::vector<CollisionInfo> collisions;
};

// Moves using a single iteration; speed should not exceed pos_max_d/dtime
collisionMoveResult collisionMoveSimple(Environment *env,IGameDef *gamedef,
		f32 pos_max_d, const aabb3f &box_0,
		f32 stepheight, f32 dtime,
		v3f *pos_f, v3f *speed_f,
		v3f accel_f, ActiveObject *self=NULL,
		bool collideWithObjects=true);

// Helper function:
// Checks for collision of a moving aabbox with a static aabbox
// Returns -1 if no collision, 0 if X collision, 1 if Y collision, 2 if Z collision
// dtime receives time until first collision, invalid if -1 is returned
CollisionAxis axisAlignedCollision(
		const aabb3f &staticbox, const aabb3f &movingbox,
		const v3f &speed, f32 *dtime);

// Helper function:
// Checks if moving the movingbox up by the given distance would hit a ceiling.
bool wouldCollideWithCeiling(
		const std::vector<aabb3f> &staticboxes,
		const aabb3f &movingbox,
		f32 y_increase, f32 d);
