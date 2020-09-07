/*
Minetest
Copyright (C) 2010-2020 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2015-2020 paramat
Copyright (C) 2013-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#pragma once

#include "noise.h"
#include "nodedef.h"
#include "util/string.h"
#include "util/container.h"

#define MAPGEN_DEFAULT MAPGEN_V7
#define MAPGEN_DEFAULT_NAME "v7"

/////////////////// Mapgen flags
#define MG_TREES       0x01  // Obsolete. Moved into mgv6 flags
#define MG_CAVES       0x02
#define MG_DUNGEONS    0x04
#define MG_FLAT        0x08  // Obsolete. Moved into mgv6 flags
#define MG_LIGHT       0x10
#define MG_DECORATIONS 0x20
#define MG_BIOMES      0x40
#define MG_ORES        0x80

typedef u16 biome_t;  // copy from mg_biome.h to avoid an unnecessary include

class Settings;
class MMVManip;
class NodeDefManager;

extern FlagDesc flagdesc_mapgen[];
extern FlagDesc flagdesc_gennotify[];

class Biome;
class BiomeGen;
struct BiomeParams;
class BiomeManager;
class EmergeParams;
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

struct GenNotifyEvent {
	GenNotifyType type;
	v3s16 pos;
	u32 id;
};

class GenerateNotifier {
public:
	GenerateNotifier() = default;
	GenerateNotifier(u32 notify_on, const std::set<u32> *notify_on_deco_ids);

	void setNotifyOn(u32 notify_on);
	void setNotifyOnDecoIds(const std::set<u32> *notify_on_deco_ids);

	bool addEvent(GenNotifyType type, v3s16 pos, u32 id=0);
	void getEvents(std::map<std::string, std::vector<v3s16> > &event_map);
	void clearEvents();

private:
	u32 m_notify_on = 0;
	const std::set<u32> *m_notify_on_deco_ids;
	std::list<GenNotifyEvent> m_notify_events;
};

// Order must match the order of 'static MapgenDesc g_reg_mapgens[]' in mapgen.cpp
enum MapgenType {
	MAPGEN_V7,
	MAPGEN_VALLEYS,
	MAPGEN_CARPATHIAN,
	MAPGEN_V5,
	MAPGEN_FLAT,
	MAPGEN_FRACTAL,
	MAPGEN_SINGLENODE,
	MAPGEN_V6,
	MAPGEN_INVALID,
};

struct MapgenParams {
	MapgenParams() = default;
	virtual ~MapgenParams();

	MapgenType mgtype = MAPGEN_DEFAULT;
	s16 chunksize = 5;
	u64 seed = 0;
	s16 water_level = 1;
	s16 mapgen_limit = MAX_MAP_GENERATION_LIMIT;
	// Flags set in readParams
	u32 flags = 0;
	u32 spflags = 0;

	BiomeParams *bparams = nullptr;

	s16 mapgen_edge_min = -MAX_MAP_GENERATION_LIMIT;
	s16 mapgen_edge_max = MAX_MAP_GENERATION_LIMIT;

	virtual void readParams(const Settings *settings);
	virtual void writeParams(Settings *settings) const;
	// Default settings for g_settings such as flags
	virtual void setDefaultSettings(Settings *settings) {};

	s32 getSpawnRangeMax();

private:
	void calcMapgenEdges();
	bool m_mapgen_edges_calculated = false;
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
	s32 seed = 0;
	int water_level = 0;
	int mapgen_limit = 0;
	u32 flags = 0;
	bool generating = false;
	int id = -1;

	MMVManip *vm = nullptr;
	const NodeDefManager *ndef = nullptr;

	u32 blockseed;
	s16 *heightmap = nullptr;
	biome_t *biomemap = nullptr;
	v3s16 csize;

	BiomeGen *biomegen = nullptr;
	GenerateNotifier gennotify;

	Mapgen() = default;
	Mapgen(int mapgenid, MapgenParams *params, EmergeParams *emerge);
	virtual ~Mapgen() = default;
	DISABLE_CLASS_COPY(Mapgen);

	virtual MapgenType getType() const { return MAPGEN_INVALID; }

	static u32 getBlockSeed(v3s16 p, s32 seed);
	static u32 getBlockSeed2(v3s16 p, s32 seed);
	s16 findGroundLevelFull(v2s16 p2d);
	s16 findGroundLevel(v2s16 p2d, s16 ymin, s16 ymax);
	s16 findLiquidSurface(v2s16 p2d, s16 ymin, s16 ymax);
	void updateHeightmap(v3s16 nmin, v3s16 nmax);
	void getSurfaces(v2s16 p2d, s16 ymin, s16 ymax,
		std::vector<s16> &floors, std::vector<s16> &ceilings);

	void updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax);

	void setLighting(u8 light, v3s16 nmin, v3s16 nmax);
	void lightSpread(VoxelArea &a, std::queue<std::pair<v3s16, u8>> &queue,
		const v3s16 &p, u8 light);
	void calcLighting(v3s16 nmin, v3s16 nmax, v3s16 full_nmin, v3s16 full_nmax,
		bool propagate_shadow = true);
	void propagateSunlight(v3s16 nmin, v3s16 nmax, bool propagate_shadow);
	void spreadLight(const v3s16 &nmin, const v3s16 &nmax);

	virtual void makeChunk(BlockMakeData *data) {}
	virtual int getGroundLevelAtPoint(v2s16 p) { return 0; }

	// getSpawnLevelAtPoint() is a function within each mapgen that returns a
	// suitable y co-ordinate for player spawn ('suitable' usually meaning
	// within 16 nodes of water_level). If a suitable spawn level cannot be
	// found at the specified (X, Z) 'MAX_MAP_GENERATION_LIMIT' is returned to
	// signify this and to cause Server::findSpawnPos() to try another (X, Z).
	virtual int getSpawnLevelAtPoint(v2s16 p) { return 0; }

	// Mapgen management functions
	static MapgenType getMapgenType(const std::string &mgname);
	static const char *getMapgenName(MapgenType mgtype);
	static Mapgen *createMapgen(MapgenType mgtype, MapgenParams *params,
		EmergeParams *emerge);
	static MapgenParams *createMapgenParams(MapgenType mgtype);
	static void getMapgenNames(std::vector<const char *> *mgnames, bool include_hidden);
	static void setDefaultSettings(Settings *settings);

private:
	// isLiquidHorizontallyFlowable() is a helper function for updateLiquid()
	// that checks whether there are floodable nodes without liquid beneath
	// the node at index vi.
	inline bool isLiquidHorizontallyFlowable(u32 vi, v3s16 em);
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
	MapgenBasic(int mapgenid, MapgenParams *params, EmergeParams *emerge);
	virtual ~MapgenBasic();

	virtual void generateBiomes();
	virtual void dustTopNodes();
	virtual void generateCavesNoiseIntersection(s16 max_stone_y);
	virtual void generateCavesRandomWalk(s16 max_stone_y, s16 large_cave_ymax);
	virtual bool generateCavernsNoise(s16 max_stone_y);
	virtual void generateDungeons(s16 max_stone_y);

protected:
	EmergeParams *m_emerge;
	BiomeManager *m_bmgr;

	Noise *noise_filler_depth;

	v3s16 node_min;
	v3s16 node_max;
	v3s16 full_node_min;
	v3s16 full_node_max;

	content_t c_stone;
	content_t c_water_source;
	content_t c_river_water_source;
	content_t c_lava_source;
	content_t c_cobble;

	int ystride;
	int zstride;
	int zstride_1d;
	int zstride_1u1d;

	u32 spflags;

	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;
	NoiseParams np_dungeons;
	float cave_width;
	float cavern_limit;
	float cavern_taper;
	float cavern_threshold;
	int small_cave_num_min;
	int small_cave_num_max;
	int large_cave_num_min;
	int large_cave_num_max;
	float large_cave_flooded;
	s16 large_cave_depth;
	s16 dungeon_ymin;
	s16 dungeon_ymax;
};
