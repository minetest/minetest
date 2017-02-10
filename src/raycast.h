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

#ifndef SRC_RAYCAST_H_
#define SRC_RAYCAST_H_


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
	v3f *collision_point, v3s16 *collision_normal);


#endif /* SRC_RAYCAST_H_ */
