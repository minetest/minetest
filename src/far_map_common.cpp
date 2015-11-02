/*
Minetest
Copyright (C) 2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "far_map_common.h"
#include <sstream>

std::string analyze_far_block(v3s16 p, const std::vector<FarNode> &content,
		const VoxelArea &content_area, const VoxelArea &effective_area)
{
	std::ostringstream os(std::ios::binary);
	os<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")";

	if (content_area == effective_area)
		os<<", no extra content area";
	else if (content_area.contains(effective_area))
		os<<", content area contains effective area";
	else if (effective_area.contains(content_area))
		os<<", effective area contains content area (WARNING)";
	else
		os<<", neither area contain each other (WARNING)";

	s32 num_zero = 0;
	s32 num_ignore = 0;
	s32 num_air = 0;
	s32 num_other = 0;
	s32 num_zero_daylight = 0;
	s32 num_max_daylight = 0;

	v3s16 fnp;
	for (fnp.Y=effective_area.MinEdge.Y; fnp.Y<=effective_area.MaxEdge.Y; fnp.Y++)
	for (fnp.X=effective_area.MinEdge.X; fnp.X<=effective_area.MaxEdge.X; fnp.X++)
	for (fnp.Z=effective_area.MinEdge.Z; fnp.Z<=effective_area.MaxEdge.Z; fnp.Z++) {
		size_t ci = content_area.index(fnp);
		const FarNode &n = content[ci];
		if (n.id == 0)
			num_zero++;
		else if (n.id == CONTENT_IGNORE)
			num_ignore++;
		else if (n.id == CONTENT_AIR)
			num_air++;
		else
			num_other++;

		if ((n.light & 0x0f) == 0)
			num_zero_daylight++;
		else if ((n.light & 0x0f) == 15)
			num_max_daylight++;
	}

	os<<", "<<num_zero<<" zero";
	os<<", "<<num_ignore<<" ignore";
	os<<", "<<num_air<<" air";
	os<<", "<<num_other<<" other";
	os<<", "<<num_zero_daylight<<" zero_daylight";
	os<<", "<<num_max_daylight<<" max_daylight";

	return os.str();
}

