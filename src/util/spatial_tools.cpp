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

#include "irr_aabb3d.h"
#include "util/spatial_tools.h"

namespace sp_convert {

template<>
SpatialIndex::Region get_spatial_region(const aabb3f &space)
{
	double coordsMin[3];
	double coordsMax[3];
	get_doubles_from_point(space.MinEdge, coordsMin);
	get_doubles_from_point(space.MaxEdge, coordsMax);
	return SpatialIndex::Region(coordsMin, coordsMax, 3);
}

}