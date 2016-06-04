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

#include "noise.h"
#include "nodedef.h"
#include "mapnode.h"
#include "util/string.h"
#include "util/container.h"

#define DEFAULT_MAPGEN "v6"

/////////////////// Mapgen flags
#define MG_TREES       0x01
#define MG_CAVES       0x02
#define MG_DUNGEONS    0x04
#define MG_FLAT        0x08
#define MG_LIGHT       0x10
#define MG_DECORATIONS 0x20

typedef u8 biome_t;  // copy from mg_biome.h to avoid an unnecessary include

class Settings;
class MMVManip;
class INodeDefManager;

extern FlagDesc flagdesc_mapgen[];
extern FlagDesc flagdesc_gennotify[];

class Biome;
class BiomeGen;
struct BiomeParams;
class BiomeManager;
class EmergeManager;
class MapBlock;
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

enum GenNotifyType {
	GENNOTIFY_DUNGEON,
	GENNOTIFY_TEMPLE,
	GENNOTIFY_CAVE_BEGIN,
	GENNOTIFY_CAVE_END,
	GENNOTIFY_LARGECAVE_BEGIN,
	GENNOTIFY_LARGECAVE_END,
	GENNOTIFY_DECORATION,
	NUM_GENNOTIFY_TYPES
};

// TODO(hmmmm/paramat): make stone type selection dynamic
enum MgStoneType {
	MGSTONE_STONE,
	MGSTONE_DESERT_STONE,
	MGSTONE_SANDSTONE,
};

struct GenNotifyEvent {
	GenNotifyType type;
	v3s16 pos;
	u32 id;
};

class GenerateNotifier {
public:
	GenerateNotifier();
	GenerateNotifier(u32 notify_on, std::set<u32> *notify_on_deco_ids);

	void setNotifyOn(u32 notify_on);
	void setNotifyOnDecoIds(std::set<u32> *notify_on_deco_ids);

	bool addEvent(GenNotifyType type, v3s16 pos, u32 id=0);
	void getEvents(std::map<std::string, std::vector<v3s16> > &event_map,
		bool peek_events=false);

private:
	u32 m_notify_on;
	std::set<u32> *m_notify_on_deco_ids;
	std::list<GenNotifyEvent> m_notify_events;
};

struct MapgenSpecificParams {
	virtual void readParams(const Settings *settings) = 0;
	virtual void writeParams(Settings *settings) const = 0;
	virtual ~MapgenSpecificParams() {}
};

struct MapgenParams {
	std::string mg_name;
	s16 chunksize;
	u64 seed;
	s16 water_level;
	u32 flags;

	BiomeParams *bparams;
	MapgenSpecificParams *sparams;

	MapgenParams() :
		mg_name(DEFAULT_MAPGEN),
		chunksize(5),
		seed(0),
		water_level(1),
		flags(MG_CAVES | MG_LIGHT | MG_DECORATIONS),
		bparams(NULL),
		sparams(NULL)
	{
	}

	virtual ~MapgenParams();

	void load(const Settings &settings);
	void save(Settings &settings) const;
};


/*
	Generic interface for map generators.  All mapgens must inherit this class.
	If a feature exposed by a public member pointer is not supported by a
	certain mapgen, it must be set to NULL.

	Apart from makeChunk, getGroundLevelAtPoint, and getSpawnLevelAtPoint, all
	methods can be used by constructing a Mapgen base class and setting the
	appropriate public members (e.g. vm, ndef, and so on).
*/
class Mapgen {
public:
	s32 seed;
	int water_level;
	u32 flags;
	bool generating;
	int id;

	MMVManip *vm;
	INodeDefManager *ndef;

	u32 blockseed;
	s16 *heightmap;
	biome_t *biomemap;
	v3s16 csize;

	BiomeGen *biomegen;
	GenerateNotifier gennotify;

	Mapgen();
	Mapgen(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	virtual ~Mapgen();

	static u32 getBlockSeed(v3s16 p, s32 seed);
	static u32 getBlockSeed2(v3s16 p, s32 seed);
	s16 findGroundLevelFull(v2s16 p2d);
	s16 findGroundLevel(v2s16 p2d, s16 ymin, s16 ymax);
	s16 findLiquidSurface(v2s16 p2d, s16 ymin, s16 ymax);
	void updateHeightmap(v3s16 nmin, v3s16 nmax);
	void updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax);

	void setLighting(u8 light, v3s16 nmin, v3s16 nmax);
	void lightSpread(VoxelArea &a, v3s16 p, u8 light);
	void calcLighting(v3s16 nmin, v3s16 nmax, v3s16 full_nmin, v3s16 full_nmax,
		bool propagate_shadow = true);
	void propagateSunlight(v3s16 nmin, v3s16 nmax, bool propagate_shadow);
	void spreadLight(v3s16 nmin, v3s16 nmax);

	virtual void makeChunk(BlockMakeData *data) {}
	virtual int getGroundLevelAtPoint(v2s16 p) { return 0; }

	// getSpawnLevelAtPoint() is a function within each mapgen that returns a
	// suitable y co-ordinate for player spawn ('suitable' usually meaning
	// within 16 nodes of water_level). If a suitable spawn level cannot be
	// found at the specified (X, Z) 'MAX_MAP_GENERATION_LIMIT' is returned to
	// signify this and to cause Server::findSpawnPos() to try another (X, Z).
	virtual int getSpawnLevelAtPoint(v2s16 p) { return 0; }

private:
	// isLiquidHorizontallyFlowable() is a helper function for updateLiquid()
	// that checks whether there are floodable nodes without liquid beneath
	// the node at index vi.
	inline bool isLiquidHorizontallyFlowable(u32 vi, v3s16 em);
	DISABLE_CLASS_COPY(Mapgen);
};

/*
	MapgenBasic is a Mapgen implementation that handles basic functionality
	the majority of conventional mapgens will probably want to use, but isn't
	generic enough to be included as part of the base Mapgen class (such as
	generating biome terrain over terrain node skeletons, generating caves,
	dungeons, etc.)

	Inherit MapgenBasic instead of Mapgen to add this basic functionality to
	your mapgen without having to reimplement it.  Feel free to override any of
	these methods if you desire different or more advanced behavior.

	Note that you must still create your own generateTerrain implementation when
	inheriting MapgenBasic.
*/
class MapgenBasic : public Mapgen {
public:
	MapgenBasic(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	virtual ~MapgenBasic();

	virtual void generateCaves(s16 max_stone_y, s16 large_cave_depth);
	virtual void generateDungeons(s16 max_stone_y, MgStoneType stone_type);
	virtual MgStoneType generateBiomes();
	virtual void dustTopNodes();

protected:
	EmergeManager *m_emerge;
	BiomeManager *m_bmgr;

	Noise *noise_filler_depth;

	v3s16 node_min;
	v3s16 node_max;
	v3s16 full_node_min;
	v3s16 full_node_max;

	// Content required for generateBiomes
	content_t c_stone;
	content_t c_water_source;
	content_t c_river_water_source;
	content_t c_desert_stone;
	content_t c_sandstone;

	// Content required for generateDungeons
	content_t c_cobble;
	content_t c_stair_cobble;
	content_t c_mossycobble;
	content_t c_sandstonebrick;
	content_t c_stair_sandstonebrick;

	int ystride;
	int zstride;
	int zstride_1d;
	int zstride_1u1d;

	u32 spflags;

	NoiseParams np_cave1;
	NoiseParams np_cave2;
	float cave_width;
};

struct MapgenFactory {
	virtual Mapgen *createMapgen(int mgid, MapgenParams *params,
		EmergeManager *emerge) = 0;
	virtual MapgenSpecificParams *createMapgenParams() = 0;
	virtual ~MapgenFactory() {}
};

#endif
