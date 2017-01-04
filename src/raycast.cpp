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

#include "irr_v3d.h"
#include "irr_aabb3d.h"

bool boxLineCollision(const aabb3f &box, const v3f &start,
		const v3f &dir, v3f *collision_point, v3s16 *collision_normal) {
	if (box.isPointInside(start)) {
		*collision_point = start;
		collision_normal->set(0, 0, 0);
		return true;
	}
	float m = 0;

	// Test X collision
	if (dir.X != 0) {
		if (dir.X > 0)
			m = (box.MinEdge.X - start.X) / dir.X;
		else
			m = (box.MaxEdge.X - start.X) / dir.X;

		if (m >= 0 && m <= 1) {
			*collision_point = start + dir * m;
			if ((collision_point->Y >= box.MinEdge.Y)
					&& (collision_point->Y <= box.MaxEdge.Y)
					&& (collision_point->Z >= box.MinEdge.Z)
					&& (collision_point->Z <= box.MaxEdge.Z)) {
				collision_normal->set((dir.X > 0) ? -1 : 1, 0, 0);
				return true;
			}
		}
	}

	// Test Y collision
	if (dir.Y != 0) {
		if (dir.Y > 0)
			m = (box.MinEdge.Y - start.Y) / dir.Y;
		else
			m = (box.MaxEdge.Y - start.Y) / dir.Y;

		if (m >= 0 && m <= 1) {
			*collision_point = start + dir * m;
			if ((collision_point->X >= box.MinEdge.X)
					&& (collision_point->X <= box.MaxEdge.X)
					&& (collision_point->Z >= box.MinEdge.Z)
					&& (collision_point->Z <= box.MaxEdge.Z)) {
				collision_normal->set(0, (dir.Y > 0) ? -1 : 1, 0);
				return true;
			}
		}
	}

	// Test Z collision
	if (dir.Z != 0) {
		if (dir.Z > 0)
			m = (box.MinEdge.Z - start.Z) / dir.Z;
		else
			m = (box.MaxEdge.Z - start.Z) / dir.Z;

		if (m >= 0 && m <= 1) {
			*collision_point = start + dir * m;
			if ((collision_point->X >= box.MinEdge.X)
					&& (collision_point->X <= box.MaxEdge.X)
					&& (collision_point->Y >= box.MinEdge.Y)
					&& (collision_point->Y <= box.MaxEdge.Y)) {
				collision_normal->set(0, 0, (dir.Z > 0) ? -1 : 1);
				return true;
			}
		}
	}
	return false;
}
