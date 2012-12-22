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

class BiomeDefManager;
class Biome;

//struct BlockMakeData;
class MapBlock;
class ManualMapVoxelManipulator;
class VoxelManipulator;
class INodeDefManager;


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


class Mapgen {
public:

	int seed;
	int water_level;

	bool generating;
	int id;


	//virtual Mapgen(BiomeDefManager *biomedef, int mapgenid=0, u64 seed=0);
	//virtual ~Mapgen();
	virtual void makeChunk(BlockMakeData *data) {};

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
	Noise *noise_cave;

	float *map_terrain_base;
	float *map_terrain_higher;
	float *map_steepness;
	float *map_height_select;
	float *map_trees;
	float *map_mud;
	float *map_beach;
	float *map_biome;
	float *map_cave;

	bool use_smooth_biome_trans;

	MapgenV6(int mapgenid=0, u64 seed=0);
	~MapgenV6();

	void makeChunk(BlockMakeData *data);


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


class MapgenV7 : public Mapgen {
public:
	BlockMakeData *data;
	ManualMapVoxelManipulator *vmanip;
	INodeDefManager *ndef;
	BiomeDefManager *biomedef;

	int ystride;
	int zstride;

	v3s16 csize;
	//int seed;
	//int water_level;

	Noise *noise_terrain;
	Noise *noise_bgroup;
	Noise *noise_heat;
	Noise *noise_humidity;

	v3s16 node_min;
	v3s16 node_max;

	float *map_terrain;
	float *map_bgroup;
	float *map_heat;
	float *map_humidity;

	bool generating;
	int id;

	NoiseParams *np_terrain;
	NoiseParams *np_bgroup;
	NoiseParams *np_heat;
	NoiseParams *np_humidity;

	//should these be broken off into a "commonly used nodes" class?
	MapNode n_air;
	MapNode n_water;
	MapNode n_lava;

	MapgenV7(BiomeDefManager *biomedef, int mapgenid=0, u64 seed=0);
	MapgenV7(BiomeDefManager *biomedef, int mapgenid, u64 seed,
		   NoiseParams *np_terrain, NoiseParams *np_bgroup,
		   NoiseParams *np_heat,    NoiseParams *np_humidity);
	void init(BiomeDefManager *biomedef, int mapgenid, u64 seed,
			   NoiseParams *np_terrain, NoiseParams *np_bgroup,
			   NoiseParams *np_heat,    NoiseParams *np_humidity);
	~MapgenV7();

	void makeChunk(BlockMakeData *data);
	void updateLiquid(v3s16 node_min, v3s16 node_max);
	void updateLighting(v3s16 node_min, v3s16 node_max);

	//Legacy functions for Farmesh (pending removal)
//	static bool get_have_beach(u64 seed, v2s16 p2d);
//	static double tree_amount_2d(u64 seed, v2s16 p);
//	static s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision);
};

class EmergeManager {
public:
	//settings
	u64 seed;
	int water_level;
	NoiseParams *np_terrain;
	NoiseParams *np_bgroup;
	NoiseParams *np_heat;
	NoiseParams *np_humidity;

	//biome manager
	BiomeDefManager *biomedef;

	//mapgen objects here

	EmergeManager(IGameDef *gamedef);
	~EmergeManager();
	void addBlockToQueue();



	//mapgen helper methods
	Biome *getBiomeAtPoint(v3s16 p);
	int getGroundLevelAtPoint(v2s16 p);
	bool isBlockUnderground(v3s16 blockpos);
	u32 getBlockSeed(v3s16 p);
};


/*
namespace mapgen
{
	// Finds precise ground level at any position
	s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision);

	// Find out if block is completely underground
	bool block_is_underground(u64 seed, v3s16 blockpos);

	// Get a pseudorandom seed for a position on the map
	u32 get_blockseed(u64 seed, v3s16 p);

	// Main map generation routine
	void make_block(BlockMakeData *data);


	//These are used by FarMesh
	bool get_have_beach(u64 seed, v2s16 p2d);
	double tree_amount_2d(u64 seed, v2s16 p);

	struct BlockMakeData
	{
		bool no_op;
		ManualMapVoxelManipulator *vmanip; // Destructor deletes
		u64 seed;
		v3s16 blockpos_min;
		v3s16 blockpos_max;
		v3s16 blockpos_requested;
		UniqueQueue<v3s16> transforming_liquid;
		INodeDefManager *nodedef;

		BlockMakeData();
		~BlockMakeData();
	};

}; // namespace mapgen
*/
#endif

