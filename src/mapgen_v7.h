/*
Minetest
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 paramat, Matt Gregory

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

#ifndef MAPGEN_V7_HEADER
#define MAPGEN_V7_HEADER

#include "mapgen.h"

/////////////////// Mapgen V7 flags
#define MGV7_MOUNTAINS   0x01
#define MGV7_RIDGES      0x02

class BiomeManager;

extern FlagDesc flagdesc_mapgen_v7[];


struct MapgenV7Params : public MapgenSpecificParams {
	u32 spflags;
	float cave_width;
	NoiseParams np_terrain_base;
	NoiseParams np_terrain_alt;
	NoiseParams np_terrain_persist;
	NoiseParams np_height_select;
	NoiseParams np_filler_depth;
	NoiseParams np_mount_height;
	NoiseParams np_ridge_uwater;
	NoiseParams np_mountain;
	NoiseParams np_ridge;
	NoiseParams np_cave1;
	NoiseParams np_cave2;

	MapgenV7Params();
	~MapgenV7Params() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenV7 : public Mapgen {
public:
	EmergeManager *m_emerge;
	BiomeManager *bmgr;

	int ystride;
	int zstride_1u1d;
	int zstride_1d;

	v3s16 node_min;
	v3s16 node_max;
	v3s16 full_node_min;
	v3s16 full_node_max;

	s16 *ridge_heightmap;

	u32 spflags;
	float cave_width;
	Noise *noise_terrain_base;
	Noise *noise_terrain_alt;
	Noise *noise_terrain_persist;
	Noise *noise_height_select;
	Noise *noise_filler_depth;
	Noise *noise_mount_height;
	Noise *noise_ridge_uwater;
	Noise *noise_mountain;
	Noise *noise_ridge;
	Noise *noise_cave1;
	Noise *noise_cave2;

	Noise *noise_heat;
	Noise *noise_humidity;
	Noise *noise_heat_blend;
	Noise *noise_humidity_blend;

	content_t c_stone;
	content_t c_water_source;
	content_t c_lava_source;
	content_t c_desert_stone;
	content_t c_ice;
	content_t c_sandstone;

	content_t c_cobble;
	content_t c_stair_cobble;
	content_t c_mossycobble;
	content_t c_sandstonebrick;
	content_t c_stair_sandstonebrick;

	MapgenV7(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	~MapgenV7();

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	Biome *getBiomeAtPoint(v3s16 p);

	float baseTerrainLevelAtPoint(s16 x, s16 z);
	float baseTerrainLevelFromMap(int index);
	bool getMountainTerrainAtPoint(s16 x, s16 y, s16 z);
	bool getMountainTerrainFromMap(int idx_xyz, int idx_xz, s16 y);

	void calculateNoise();

	int generateTerrain();
	void generateRidgeTerrain();

	MgStoneType generateBiomes(float *heat_map, float *humidity_map);
	void dustTopNodes();

	void generateCaves(s16 max_stone_y);
};

struct MapgenFactoryV7 : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge)
	{
		return new MapgenV7(mgid, params, emerge);
	};

	MapgenSpecificParams *createMapgenParams()
	{
		return new MapgenV7Params();
	};
};

#endif
