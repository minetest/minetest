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

#ifndef MAPGENV6_HEADER
#define MAPGENV6_HEADER

#include "dungeongen.h"
#include "mapgen.h"

#define AVERAGE_MUD_AMOUNT 4

enum BiomeType
{
	BT_NORMAL,
	BT_DESERT
};

extern NoiseParams nparams_v6_def_terrain_base;
extern NoiseParams nparams_v6_def_terrain_higher;
extern NoiseParams nparams_v6_def_steepness;
extern NoiseParams nparams_v6_def_height_select;
extern NoiseParams nparams_v6_def_trees;
extern NoiseParams nparams_v6_def_mud;
extern NoiseParams nparams_v6_def_beach;
extern NoiseParams nparams_v6_def_biome;
extern NoiseParams nparams_v6_def_cave;

struct MapgenV6Params : public MapgenParams {
	float freq_desert;
	float freq_beach;
	NoiseParams *np_terrain_base;
	NoiseParams *np_terrain_higher;
	NoiseParams *np_steepness;
	NoiseParams *np_height_select;
	NoiseParams *np_trees;
	NoiseParams *np_mud;
	NoiseParams *np_beach;
	NoiseParams *np_biome;
	NoiseParams *np_cave;

	MapgenV6Params() {
		freq_desert       = 0.45;
		freq_beach        = 0.15;
		np_terrain_base   = &nparams_v6_def_terrain_base;
		np_terrain_higher = &nparams_v6_def_terrain_higher;
		np_steepness      = &nparams_v6_def_steepness;
		np_height_select  = &nparams_v6_def_height_select;
		np_trees          = &nparams_v6_def_trees;
		np_mud            = &nparams_v6_def_mud;
		np_beach          = &nparams_v6_def_beach;
		np_biome          = &nparams_v6_def_biome;
		np_cave           = &nparams_v6_def_cave;
	}
	
	bool readParams(Settings *settings);
	void writeParams(Settings *settings);
};

class MapgenV6 : public Mapgen {
public:
	//ManualMapVoxelManipulator &vmanip;

	int ystride;
	v3s16 csize;

	v3s16 node_min;
	v3s16 node_max;

	Noise *noise_terrain_base;
	Noise *noise_terrain_higher;
	Noise *noise_steepness;
	Noise *noise_height_select;
	Noise *noise_trees;
	Noise *noise_mud;
	Noise *noise_beach;
	Noise *noise_biome;

	float *map_terrain_base;
	float *map_terrain_higher;
	float *map_steepness;
	float *map_height_select;
	float *map_trees;
	float *map_mud;
	float *map_beach;
	float *map_biome;

	NoiseParams *np_cave;

	u32 flags;
	float freq_desert;
	float freq_beach;

	MapgenV6(int mapgenid, MapgenV6Params *params);
	~MapgenV6();
	
	void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p);

	double baseRockLevelFromNoise(v2s16 p);
	static s16 find_ground_level(VoxelManipulator &vmanip,
								 v2s16 p2d, INodeDefManager *ndef);
	static s16 find_stone_level(VoxelManipulator &vmanip,
								 v2s16 p2d, INodeDefManager *ndef);
	void make_tree(ManualMapVoxelManipulator &vmanip, v3s16 p0,
					 bool is_apple_tree, INodeDefManager *ndef);
	double tree_amount_2d(u64 seed, v2s16 p);
	bool block_is_underground(u64 seed, v3s16 blockpos);
	double base_rock_level_2d(u64 seed, v2s16 p);
	s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision);
	double get_mud_add_amount(u64 seed, v2s16 p);
	bool get_have_beach(u64 seed, v2s16 p2d);
	BiomeType get_biome(u64 seed, v2s16 p2d);
	u32 get_blockseed(u64 seed, v3s16 p);
};

struct MapgenFactoryV6 : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge) {
		return new MapgenV6(mgid, (MapgenV6Params *)params);
	};
	
	MapgenParams *createMapgenParams() {
		return new MapgenV6Params();
	};
};

#endif
