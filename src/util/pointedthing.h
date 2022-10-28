/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <iostream>
#include <string>

enum PointedThingType
{
	POINTEDTHING_NOTHING,
	POINTEDTHING_NODE,
	POINTEDTHING_OBJECT
};

//! An active object or node which is selected by a ray on the map.
struct PointedThing
{
	//! The type of the pointed object.
	PointedThingType type = POINTEDTHING_NOTHING;
	/*!
	 * Only valid if type is POINTEDTHING_NODE.
	 * The coordinates of the node which owns the
	 * nodebox that the ray hits first.
	 * This may differ from node_real_undersurface if
	 * a nodebox exceeds the limits of its node.
	 */
	v3s16 node_undersurface;
	/*!
	 * Only valid if type is POINTEDTHING_NODE.
	 * The coordinates of the last node the ray intersects
	 * before node_undersurface. Same as node_undersurface
	 * if the ray starts in a nodebox.
	 */
	v3s16 node_abovesurface;
	/*!
	 * Only valid if type is POINTEDTHING_NODE.
	 * The coordinates of the node which contains the
	 * point of the collision and the nodebox of the node.
	 */
	v3s16 node_real_undersurface;
	/*!
	 * Only valid if type is POINTEDTHING_OBJECT.
	 * The ID of the object the ray hit.
	 */
	u16 object_id = 0;
	/*!
	 * Only valid if type isn't POINTEDTHING_NONE.
	 * First intersection point of the ray and the nodebox in irrlicht
	 * coordinates.
	 */
	v3f intersection_point;
	/*!
	 * Only valid if type isn't POINTEDTHING_NONE.
	 * Normal vector of the intersection.
	 * This is perpendicular to the face the ray hits,
	 * points outside of the box and it's length is 1.
	 */
	v3f intersection_normal;
	/*!
	 * Only valid if type is POINTEDTHING_OBJECT.
	 * Raw normal vector of the intersection before applying rotation.
	 */
	v3f raw_intersection_normal;
	/*!
	 * Only valid if type isn't POINTEDTHING_NONE.
	 * Indicates which selection box is selected, if there are more of them.
	 */
	u16 box_id = 0;
	/*!
	 * Square of the distance between the pointing
	 * ray's start point and the intersection point in irrlicht coordinates.
	 */
	f32 distanceSq = 0;

	//! Constructor for POINTEDTHING_NOTHING
	PointedThing() = default;
	//! Constructor for POINTEDTHING_NODE
	PointedThing(const v3s16 &under, const v3s16 &above,
		const v3s16 &real_under, const v3f &point, const v3f &normal,
		u16 box_id, f32 distSq);
	//! Constructor for POINTEDTHING_OBJECT
	PointedThing(u16 id, const v3f &point, const v3f &normal, const v3f &raw_normal, f32 distSq);
	std::string dump() const;
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
	/*!
	 * This function ignores the intersection point and normal.
	 */
	bool operator==(const PointedThing &pt2) const;
	bool operator!=(const PointedThing &pt2) const;
};
