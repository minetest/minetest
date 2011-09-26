/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAPGEN_HEADER
#define MAPGEN_HEADER

#include "common_irrlicht.h"
#include "utility.h" // UniqueQueue

struct BlockMakeData;
class MapBlock;
class ManualMapVoxelManipulator;

namespace mapgen
{
	// Finds precise ground level at any position
	s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision);

	// Find out if block is completely underground
	bool block_is_underground(u64 seed, v3s16 blockpos);

	// Main map generation routine
	void make_block(BlockMakeData *data);
	
	// Add objects according to block content
	void add_random_objects(MapBlock *block);

	// Add a tree
	void make_tree(ManualMapVoxelManipulator &vmanip, v3s16 p0, bool is_apple_tree);
	
	/*
		These are used by FarMesh
	*/
	bool get_have_sand(u64 seed, v2s16 p2d);
	double tree_amount_2d(u64 seed, v2s16 p);
	

	struct BlockMakeData
	{
		bool no_op;
		ManualMapVoxelManipulator *vmanip;
		u64 seed;
		v3s16 blockpos;
		UniqueQueue<v3s16> transforming_liquid;

		BlockMakeData();
		~BlockMakeData();
	};

}; // namespace mapgen

#endif

