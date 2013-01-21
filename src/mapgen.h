/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef MAPGEN_HEADER
#define MAPGEN_HEADER

#include "irrlichttypes_extrabloated.h"
#include "util/container.h" // UniqueQueue
#include "gamedef.h"
#include "mapnode.h"
#include "noise.h"
#include "settings.h"

/////////////////// Mapgen flags
#define MG_TREES         0x01
#define MG_CAVES         0x02
#define MG_DUNGEONS      0x04
#define MGV6_FORESTS     0x08
#define MGV6_BIOME_BLEND 0x10

#define AVERAGE_MUD_AMOUNT 4

class BiomeDefManager;
class Biome;

//struct BlockMakeData;
class MapBlock;
class ManualMapVoxelManipulator;
class VoxelManipulator;
class INodeDefManager;

extern NoiseParams nparams_v6_def_terrain_base;
extern NoiseParams nparams_v6_def_terrain_higher;
extern NoiseParams nparams_v6_def_steepness;
extern NoiseParams nparams_v6_def_height_select;
extern NoiseParams nparams_v6_def_trees;
extern NoiseParams nparams_v6_def_mud;
extern NoiseParams nparams_v6_def_beach;
extern NoiseParams nparams_v6_def_biome;
extern NoiseParams nparams_v6_def_cave;

extern NoiseParams nparams_v7_def_terrain;
extern NoiseParams nparams_v7_def_bgroup;
extern NoiseParams nparams_v7_def_heat;
extern NoiseParams nparams_v7_def_humidity;

enum BiomeType
{
	BT_NORMAL,
	BT_DESERT
};


struct BlockMakeData {
	bool no_op;
	ManualMapVoxelManipulator *vmanip;
	u64 seed;
	v3s16 blockpos_min;
	v3s16 blockpos_max;
	v3s16 blockpos_requested;
	UniqueQueue<v3s16> transforming_liquid;
	INodeDefManager *nodedef;

	BlockMakeData();
	~BlockMakeData();
};


struct MapgenParams {
	std::string mg_name;
	int chunksize;
	u64 seed;
	int water_level;
	u32 flags;

	MapgenParams() {
		mg_name     = "v6";
		seed        = 0;
		water_level = 1;
		chunksize   = 5;
		flags       = MG_TREES | MG_CAVES | MGV6_BIOME_BLEND;
	}

	static MapgenParams *createMapgenParams(std::string &mgname);
	static MapgenParams *getParamsFromSettings(Settings *settings);

};

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
};


class Mapgen {
public:
	int seed;
	int water_level;
	bool generating;
	int id;

	virtual void makeChunk(BlockMakeData *data) {};
	virtual int getGroundLevelAtPoint(v2s16 p) = 0;

	//Legacy functions for Farmesh (pending removal)
	static bool get_have_beach(u64 seed, v2s16 p2d);
	static double tree_amount_2d(u64 seed, v2s16 p);
	static s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision);
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
	static s16 find_ground_level(VoxelManipulator &vmanip, v2s16 p2d, INodeDefManager *ndef);
	static s16 find_stone_level(VoxelManipulator &vmanip, v2s16 p2d, INodeDefManager *ndef);
	void make_tree(ManualMapVoxelManipulator &vmanip, v3s16 p0, bool is_apple_tree, INodeDefManager *ndef);
	double tree_amount_2d(u64 seed, v2s16 p);
	bool block_is_underground(u64 seed, v3s16 blockpos);
	double base_rock_level_2d(u64 seed, v2s16 p);
	s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision);
	double get_mud_add_amount(u64 seed, v2s16 p);
	bool get_have_beach(u64 seed, v2s16 p2d);
	BiomeType get_biome(u64 seed, v2s16 p2d);
	u32 get_blockseed(u64 seed, v3s16 p);
};


class EmergeManager {
public:
	//settings
	MapgenParams *params;

	//mapgen objects here
	Mapgen *mapgen;

	//biome manager
	BiomeDefManager *biomedef;

	EmergeManager(IGameDef *gamedef, BiomeDefManager *bdef, MapgenParams *mgparams);
	~EmergeManager();

	Mapgen *getMapgen();
	void addBlockToQueue();

	//mapgen helper methods
	Biome *getBiomeAtPoint(v3s16 p);
	int getGroundLevelAtPoint(v2s16 p);
	bool isBlockUnderground(v3s16 blockpos);
	u32 getBlockSeed(v3s16 p);
};

#endif

