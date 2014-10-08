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

#include "irrlichttypes_bloated.h"
#include "util/container.h" // UniqueQueue
#include "gamedef.h"
#include "nodedef.h"
#include "mapnode.h"
#include "noise.h"
#include "settings.h"

#define DEFAULT_MAPGEN "v6"

/////////////////// Mapgen flags
#define MG_TREES         0x01
#define MG_CAVES         0x02
#define MG_DUNGEONS      0x04
#define MG_FLAT          0x08
#define MG_LIGHT         0x10

/////////////////// Ore generation flags
// Use absolute value of height to determine ore placement
#define OREFLAG_ABSHEIGHT 0x01
// Use 3d noise to get density of ore placement, instead of just the position
#define OREFLAG_DENSITY   0x02 // not yet implemented
// For claylike ore types, place ore if the number of surrounding
// nodes isn't the specified node
#define OREFLAG_NODEISNT  0x04 // not yet implemented

/////////////////// Decoration flags
#define DECO_PLACE_CENTER_X     1
#define DECO_PLACE_CENTER_Y     2
#define DECO_PLACE_CENTER_Z     4
#define DECO_SCHEM_CIDS_UPDATED 8

#define ORE_RANGE_ACTUAL 1
#define ORE_RANGE_MIRROR 2

#define NUM_GEN_NOTIFY 6


extern FlagDesc flagdesc_mapgen[];
extern FlagDesc flagdesc_ore[];
extern FlagDesc flagdesc_deco_schematic[];
extern FlagDesc flagdesc_gennotify[];

class BiomeDefManager;
class Biome;
class EmergeManager;
class MapBlock;
class ManualMapVoxelManipulator;
class VoxelManipulator;
struct BlockMakeData;
class VoxelArea;
class Map;


enum MapgenObject {
	MGOBJ_VMANIP,
	MGOBJ_HEIGHTMAP,
	MGOBJ_BIOMEMAP,
	MGOBJ_HEATMAP,
	MGOBJ_HUMIDMAP,
	MGOBJ_GENNOTIFY
};

enum GenNotify {
	GENNOTIFY_DUNGEON,
	GENNOTIFY_TEMPLE,
	GENNOTIFY_CAVE_BEGIN,
	GENNOTIFY_CAVE_END,
	GENNOTIFY_LARGECAVE_BEGIN,
	GENNOTIFY_LARGECAVE_END
};

enum OreType {
	ORE_SCATTER,
	ORE_SHEET,
	ORE_CLAYLIKE
};


struct MapgenSpecificParams {
	virtual void readParams(Settings *settings) = 0;
	virtual void writeParams(Settings *settings) = 0;
	virtual ~MapgenSpecificParams() {}
};

struct MapgenParams {
	std::string mg_name;
	s16 chunksize;
	u64 seed;
	s16 water_level;
	u32 flags;

	MapgenSpecificParams *sparams;

	MapgenParams() {
		mg_name     = DEFAULT_MAPGEN;
		seed        = 0;
		water_level = 1;
		chunksize   = 5;
		flags       = MG_TREES | MG_CAVES | MG_LIGHT;
		sparams     = NULL;
	}
};

class Mapgen {
public:
	int seed;
	int water_level;
	bool generating;
	int id;
	ManualMapVoxelManipulator *vm;
	INodeDefManager *ndef;

	s16 *heightmap;
	u8 *biomemap;
	v3s16 csize;

	u32 gennotify;
	std::vector<v3s16> *gen_notifications[NUM_GEN_NOTIFY];

	Mapgen();
	virtual ~Mapgen();

	s16 findGroundLevelFull(v2s16 p2d);
	s16 findGroundLevel(v2s16 p2d, s16 ymin, s16 ymax);
	void updateHeightmap(v3s16 nmin, v3s16 nmax);
	void updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax);
	void setLighting(v3s16 nmin, v3s16 nmax, u8 light);
	void lightSpread(VoxelArea &a, v3s16 p, u8 light);
	void calcLighting(v3s16 nmin, v3s16 nmax);
	void calcLightingOld(v3s16 nmin, v3s16 nmax);

	virtual void makeChunk(BlockMakeData *data) {}
	virtual int getGroundLevelAtPoint(v2s16 p) { return 0; }
};

struct MapgenFactory {
	virtual Mapgen *createMapgen(int mgid, MapgenParams *params,
								 EmergeManager *emerge) = 0;
	virtual MapgenSpecificParams *createMapgenParams() = 0;
	virtual ~MapgenFactory() {}
};

class Ore {
public:
	content_t c_ore;                  // the node to place
	std::vector<content_t> c_wherein; // the nodes to be placed in
	u32 clust_scarcity; // ore cluster has a 1-in-clust_scarcity chance of appearing at a node
	s16 clust_num_ores; // how many ore nodes are in a chunk
	s16 clust_size;     // how large (in nodes) a chunk of ore is
	s16 height_min;
	s16 height_max;
	u8 ore_param2;		// to set node-specific attributes
	u32 flags;          // attributes for this ore
	float nthresh;      // threshhold for noise at which an ore is placed
	NoiseParams *np;    // noise for distribution of clusters (NULL for uniform scattering)
	Noise *noise;

	Ore() {
		c_ore   = CONTENT_IGNORE;
		np      = NULL;
		noise   = NULL;
	}

	virtual ~Ore();

	void placeOre(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
	virtual void generate(ManualMapVoxelManipulator *vm, int seed,
						u32 blockseed, v3s16 nmin, v3s16 nmax) = 0;
};

class OreScatter : public Ore {
	~OreScatter() {}
	virtual void generate(ManualMapVoxelManipulator *vm, int seed,
						u32 blockseed, v3s16 nmin, v3s16 nmax);
};

class OreSheet : public Ore {
	~OreSheet() {}
	virtual void generate(ManualMapVoxelManipulator *vm, int seed,
						u32 blockseed, v3s16 nmin, v3s16 nmax);
};

Ore *createOre(OreType type);


enum DecorationType {
	DECO_SIMPLE = 1,
	DECO_SCHEMATIC,
	DECO_LSYSTEM
};

#if 0
struct CutoffData {
	VoxelArea a;
	Decoration *deco;
	//v3s16 p;
	//v3s16 size;
	//s16 height;

	CutoffData(s16 x, s16 y, s16 z, s16 h) {
		p = v3s16(x, y, z);
		height = h;
	}
};
#endif

class Decoration {
public:
	INodeDefManager *ndef;

	int mapseed;
	std::vector<content_t> c_place_on;
	s16 sidelen;
	float fill_ratio;
	NoiseParams *np;

	std::set<u8> biomes;
	//std::list<CutoffData> cutoffs;
	//JMutex cutoff_mutex;

	Decoration();
	virtual ~Decoration();

	void placeDeco(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
	void placeCutoffs(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);

	virtual void generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p) = 0;
	virtual int getHeight() = 0;
	virtual std::string getName() = 0;
};

class DecoSimple : public Decoration {
public:
	std::vector<content_t> c_decos;
	std::vector<content_t> c_spawnby;
	s16 deco_height;
	s16 deco_height_max;
	s16 nspawnby;

	~DecoSimple() {}

	virtual void generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p);
	virtual int getHeight();
	virtual std::string getName();
};

#define MTSCHEM_FILE_SIGNATURE 0x4d54534d // 'MTSM'
#define MTSCHEM_FILE_VER_HIGHEST_READ  3
#define MTSCHEM_FILE_VER_HIGHEST_WRITE 3

#define MTSCHEM_PROB_NEVER  0x00
#define MTSCHEM_PROB_ALWAYS 0xFF

class DecoSchematic : public Decoration {
public:
	std::string filename;

	std::vector<content_t> c_nodes;

	u32 flags;
	Rotation rotation;
	v3s16 size;
	MapNode *schematic;
	u8 *slice_probs;

	DecoSchematic();
	~DecoSchematic();

	void updateContentIds();
	virtual void generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p);
	virtual int getHeight();
	virtual std::string getName();

	void blitToVManip(v3s16 p, ManualMapVoxelManipulator *vm,
					Rotation rot, bool force_placement);

	bool loadSchematicFile(NodeResolver *resolver,
		std::map<std::string, std::string> &replace_names);
	void saveSchematicFile(INodeDefManager *ndef);

	bool getSchematicFromMap(Map *map, v3s16 p1, v3s16 p2);
	void placeStructure(Map *map, v3s16 p, bool force_placement);
	void applyProbabilities(v3s16 p0,
		std::vector<std::pair<v3s16, u8> > *plist,
		std::vector<std::pair<s16, u8> > *splist);
};

void build_nnlist_and_update_ids(MapNode *nodes, u32 nodecount,
					std::vector<content_t> *usednodes);

/*
class DecoLSystem : public Decoration {
public:
	virtual void generate(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
};
*/

Decoration *createDecoration(DecorationType type);

#endif

