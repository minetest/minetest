/*
Minetest SplineSequence
Copyright (C) 2016-2018 Ben Deutsch <ben@bendeutsch.de>
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

#include "splinesequence.h"
#include "irrlichttypes_extrabloated.h"

SplineSequence<core::quaternion> quatSplineSequence;

static void fill_it()
{
	core::quaternion a_quat;
	quatSplineSequence.addNode(a_quat);
	quatSplineSequence.addIndex(0.0, 0, 0);
	quatSplineSequence.normalizeDurations();
}

// some specializations

template <>
core::quaternion SplineSequence<core::quaternion>::_interpolate(
		core::quaternion &bottom, core::quaternion &top, float alpha) const
{
	core::quaternion result;
	// slerp or lerp? We're dealing with splines anyway...
	result.slerp(bottom, top, alpha);
	return result;
}
