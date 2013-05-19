/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

extern NoiseParams nparams_v7_def_terrain_base;
extern NoiseParams nparams_v7_def_terrain_alt;
extern NoiseParams nparams_v7_def_terrain_mod;
extern NoiseParams nparams_v7_def_terrain_persist;
extern NoiseParams nparams_v7_def_height_select;
extern NoiseParams nparams_v7_def_ridge;

struct MapgenV7Params : public MapgenParams {
	NoiseParams np_terrain_base;
	NoiseParams np_terrain_alt;
	NoiseParams np_terrain_mod;
	NoiseParams np_terrain_persist;
	NoiseParams np_height_select;
	NoiseParams np_ridge;
	
	MapgenV7Params() {
		np_terrain_base    = nparams_v7_def_terrain_base;
		np_terrain_alt     = nparams_v7_def_terrain_alt;
		np_terrain_mod     = nparams_v7_def_terrain_mod;
		np_terrain_persist = nparams_v7_def_terrain_persist;
		np_height_select   = nparams_v7_def_height_select;
		np_ridge           = nparams_v7_def_ridge;
	}
	
	~MapgenV7Params() {}
	
	bool readParams(Settings *settings);
	void writeParams(Settings *settings);
};

class MapgenV7 : public Mapgen {
public:
	EmergeManager *emerge;
	BiomeDefManager *bmgr;

	int ystride;
	v3s16 csize;
	u32 flags;

	u32 blockseed;
	v3s16 node_min;
	v3s16 node_max;
	v3s16 full_node_min;
	v3s16 full_node_max;
	
	s16 *heightmap;
	s16 *ridge_heightmap;
	u8 *biomemap;
	
	Noise *noise_terrain_base;
	Noise *noise_terrain_alt;
	Noise *noise_terrain_mod;
	Noise *noise_terrain_persist;
	Noise *noise_height_select;
	
	Noise *noise_ridge;
	
	Noise *noise_heat;
	Noise *noise_humidity;
	
	content_t c_stone;
	content_t c_dirt;
	content_t c_dirt_with_grass;
	content_t c_sand;
	content_t c_water_source;
	content_t c_lava_source;
	content_t c_gravel;
	content_t c_cobble;
	content_t c_desert_sand;
	content_t c_desert_stone;

	MapgenV7(int mapgenid, MapgenV7Params *params, EmergeManager *emerge);
	~MapgenV7();
	
	void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p);
	Biome *getBiomeAtPoint(v3s16 p);

	float baseTerrainLevelAtPoint(int x, int z);
	float baseTerrainLevelFromMap(int index);
	void calculateNoise();
	int calcHeightMap();
	
	void generateTerrain();
	void carveRidges();
	//void carveRivers(); //experimental
	
	void testBiomes();
	void addTopNodes();
	
	void generateCaves(int max_stone_y);
};

struct MapgenFactoryV7 : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge) {
		return new MapgenV7(mgid, (MapgenV7Params *)params, emerge);
	};
	
	MapgenParams *createMapgenParams() {
		return new MapgenV7Params();
	};
};

#endif
