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
#define MGV6_JUNGLES     0x08
#define MGV6_BIOME_BLEND 0x10
#define MG_FLAT          0x20

extern FlagDesc flagdesc_mapgen[];

class BiomeDefManager;
class Biome;
class EmergeManager;
class MapBlock;
class ManualMapVoxelManipulator;
class VoxelManipulator;
class INodeDefManager;
struct BlockMakeData;
class VoxelArea;

class MapgenParams {
public:
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
	virtual ~MapgenParams() {
	};
	
	virtual bool readParams(Settings *settings) = 0;
	virtual void writeParams(Settings *settings) {};
};

class Mapgen {
public:
	virtual ~Mapgen() {};
	int seed;
	int water_level;
	bool generating;
	int id;
	ManualMapVoxelManipulator *vm;
	INodeDefManager *ndef;

	void updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax);
	void setLighting(v3s16 nmin, v3s16 nmax, u8 light);
	void lightSpread(VoxelArea &a, v3s16 p, u8 light);
	void calcLighting(v3s16 nmin, v3s16 nmax);
	void calcLightingOld(v3s16 nmin, v3s16 nmax);

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
	virtual ~MapgenFactory() {};
};

enum OreType {
	ORE_SCATTER,
	ORE_SHEET,
	ORE_CLAYLIKE
};

class Ore {
public:
	std::string ore_name;
	std::string wherein_name;

	content_t ore;
	content_t wherein;  // the node to be replaced
	s16 clust_scarcity; //
	s16 clust_num_ores; // how many ore nodes are in a chunk
	s16 clust_size;     // how large (in nodes) a chunk of ore is
	s16 height_min;
	s16 height_max;
	float nthresh;      // threshhold for noise at which an ore is placed 
	NoiseParams *np;    // noise for distribution of clusters (NULL for uniform scattering)
	Noise *noise;
	
	Ore() {
		ore     = CONTENT_IGNORE;
		wherein = CONTENT_IGNORE;
		np      = NULL;
		noise   = NULL;
	}
	
	virtual ~Ore() {};

	void resolveNodeNames(INodeDefManager *ndef);
	virtual void generate(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax) = 0;
};

class OreScatter : public Ore {
	 void generate(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
};

class OreSheet : public Ore {
	void generate(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
};

Ore *createOre(OreType type);

#endif

