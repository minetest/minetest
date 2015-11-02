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
#ifndef __FAR_MAP_COMMON_H__
#define __FAR_MAP_COMMON_H__

#include "irrlichttypes.h" // v3s16
#include "mapnode.h" // CONTENT_IGNORE
#include "voxel.h" // VoxelArea
#include <string>

struct FarNode
{
	u16 id;
	u8 light; // (day | (night << 4))

	FarNode(u16 id=CONTENT_IGNORE, u8 light=0): id(id), light(light) {}
};

std::string analyze_far_block(v3s16 p, const std::vector<FarNode> &content,
		const VoxelArea &content_area, const VoxelArea &effective_area);

#endif
