/*
Minetest
Copyright (C) 2022 JosiahWI <josiah_vanderzee@mediacombb.net>

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

#include "irr_aabb3d.h"

#include <RTree.h>


namespace sp_util {

template <typename T>
struct TaggedBBox {
	spatial::BoundingBox<double, 3> bbox;
	T idTag;

	TaggedBBox() : bbox {}, idTag {} {}
	TaggedBBox(spatial::BoundingBox<double, 3> bbox, T idTag)
			: bbox { bbox }
			, idTag { idTag }
	{}

	friend bool operator==(const TaggedBBox<T> &a, const TaggedBBox &b)
	{
		return a.idTag == b.idTag;
	}
};

template <typename T>
struct TaggedBBoxIndexable
{
	static const double *min(const TaggedBBox<T> &value) { return value.bbox.min; }
	static const double *max(const TaggedBBox<T> &value) { return value.bbox.max; }
};

template <typename T>
void get_doubles_from_point(const T &from, double (& to)[3]) {
	to[0] = from.X;
	to[1] = from.Y;
	to[2] = from.Z;
}

template <typename T>
spatial::BoundingBox<double, 3> get_spatial_region(const T &space)
{
	double coordsMin[3];
	double coordsMax[3];
	get_doubles_from_point(space.minedge, coordsMin);
	get_doubles_from_point(space.maxedge, coordsMax);
	return spatial::BoundingBox<double, 3> {coordsMin, coordsMax };
}

template<>
spatial::BoundingBox<double, 3> get_spatial_region(const aabb3f &space);

} // namespace sp_util
