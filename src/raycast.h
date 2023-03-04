/*
Minetest
Copyright (C) 2016 juhdanad, Daniel Juhasz <juhdanad@gmail.com>

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

#include "voxelalgorithms.h"
#include "util/pointedthing.h"

//! Sorts PointedThings based on their distance.
struct RaycastSort
{
	bool operator() (const PointedThing &pt1, const PointedThing &pt2) const;
};

//! Describes the state of a raycast.
class RaycastState
{
public:
	/*!
	 * Creates a raycast.
	 * @param objects_pointable if false, only nodes will be found
	 * @param liquids pointable if false, liquid nodes won't be found
	 */
	RaycastState(const core::line3d<f32> &shootline, bool objects_pointable,
		bool liquids_pointable);

	//! Shootline of the raycast.
	core::line3d<f32> m_shootline;
	//! Iterator to store the progress of the raycast.
	voxalgo::VoxelLineIterator m_iterator;
	//! Previous tested node during the raycast.
	v3s16 m_previous_node;

	/*!
	 * This priority queue stores the found pointed things
	 * waiting to be returned.
	 */
	std::priority_queue<PointedThing, std::vector<PointedThing>, RaycastSort> m_found;

	bool m_objects_pointable;
	bool m_liquids_pointable;

	//! The code needs to search these nodes around the center node.
	core::aabbox3d<s16> m_search_range { 0, 0, 0, 0, 0, 0 };

	//! If true, the Environment will initialize this state.
	bool m_initialization_needed = true;
};

/*!
 * Checks if a line and a box intersects.
 * @param[in]  box              box to test collision
 * @param[in]  start            starting point of the line
 * @param[in]  dir              direction and length of the line
 * @param[out] collision_point  first point of the collision
 * @param[out] collision_normal normal vector at the collision, points
 * outwards of the surface. If start is in the box, zero vector.
 * @returns true if a collision point was found
 */
bool boxLineCollision(const aabb3f &box, const v3f &start, const v3f &dir,
	v3f *collision_point, v3f *collision_normal);

bool boxLineCollision(const aabb3f &box, const v3f &box_rotation,
	const v3f &start, const v3f &dir,
	v3f *collision_point, v3f *collision_normal, v3f *raw_collision_normal);
