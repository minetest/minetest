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

#ifndef MAPGEN_HEADER
#define MAPGEN_HEADER

#include "irrlichttypes_extrabloated.h"
#include "util/container.h" // UniqueQueue
#include "gamedef.h"
#include "mapnode.h"
#include "noise.h"
#include "settings.h"
#include <map>

/////////////////// Mapgen flags
#define MG_TREES         0x01
#define MG_CAVES         0x02
#define MG_DUNGEONS      0x04
#define MGV6_FORESTS     0x08
#define MGV6_BIOME_BLEND 0x10
#define MG_FLAT          0x20

class BiomeDefManager;
class Biome;
class EmergeManager;
class MapBlock;
class ManualMapVoxelManipulator;
class VoxelManipulator;
class INodeDefManager;

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
	
	virtual bool readParams(Settings *settings) = 0;
	virtual void writeParams(Settings *settings) {};
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

struct MapgenFactory {
	virtual Mapgen *createMapgen(int mgid, MapgenParams *params,
								 EmergeManager *emerge) = 0;
	virtual MapgenParams *createMapgenParams() = 0;
};

class EmergeManager {
public:
	std::map<std::string, MapgenFactory *> mglist;

	//settings
	MapgenParams *params;

	//mapgen objects here
	Mapgen *mapgen;

	//biome manager
	BiomeDefManager *biomedef;

	EmergeManager(IGameDef *gamedef, BiomeDefManager *bdef);
	~EmergeManager();

	void initMapgens(MapgenParams *mgparams);
	Mapgen *createMapgen(std::string mgname, int mgid,
						MapgenParams *mgparams, EmergeManager *emerge);
	MapgenParams *createMapgenParams(std::string mgname);
	Mapgen *getMapgen();
	void addBlockToQueue();
	
	void registerMapgen(std::string name, MapgenFactory *mgfactory);
	MapgenParams *getParamsFromSettings(Settings *settings);
	void setParamsToSettings(Settings *settings);
	
	//mapgen helper methods
	Biome *getBiomeAtPoint(v3s16 p);
	int getGroundLevelAtPoint(v2s16 p);
	bool isBlockUnderground(v3s16 blockpos);
	u32 getBlockSeed(v3s16 p);
};

#endif

