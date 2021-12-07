/*
Minetest
Copyright (C) 2013-2018 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2018 paramat

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

#include "mapgen_singlenode.h"
#include "voxel.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "emerge.h"


MapgenSinglenode::MapgenSinglenode(MapgenParams *params, EmergeParams *emerge)
	: Mapgen(MAPGEN_SINGLENODE, params, emerge)
{
	const NodeDefManager *ndef = emerge->ndef;

	c_node = ndef->getId("mapgen_singlenode");
	if (c_node == CONTENT_IGNORE)
		c_node = CONTENT_AIR;

	MapNode n_node(c_node);
	set_light = (ndef->get(n_node).sunlight_propagates) ? LIGHT_SUN : 0x00;
}


//////////////////////// Map generator

void MapgenSinglenode::makeChunk(BlockMakeData *data)
{
	// Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	v3bpos_t blockpos_min = data->blockpos_min;
	v3bpos_t blockpos_max = data->blockpos_max;

	// Area of central chunk
	v3pos_t node_min = blockpos_min * MAP_BLOCKSIZE;
	v3pos_t node_max = (blockpos_max + v3pos_t(1, 1, 1)) * MAP_BLOCKSIZE - v3pos_t(1, 1, 1);

	blockseed = getBlockSeed2(node_min, data->seed);

	MapNode n_node(c_node);

	for (pos_t z = node_min.Z; z <= node_max.Z; z++)
	for (pos_t y = node_min.Y; y <= node_max.Y; y++) {
		u32 i = vm->m_area.index(node_min.X, y, z);
		for (pos_t x = node_min.X; x <= node_max.X; x++) {
			if (vm->m_data[i].getContent() == CONTENT_IGNORE)
				vm->m_data[i] = n_node;
			i++;
		}
	}

	// Add top and bottom side of water to transforming_liquid queue
	updateLiquid(&data->transforming_liquid, node_min, node_max);

	// Set lighting
	if ((flags & MG_LIGHT) && set_light == LIGHT_SUN)
		setLighting(LIGHT_SUN, node_min, node_max);

	this->generating = false;
}


int MapgenSinglenode::getSpawnLevelAtPoint(v2pos_t p)
{
	return 0;
}
