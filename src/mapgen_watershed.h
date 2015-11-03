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

#ifndef MAPGEN_WATERSHED_HEADER
#define MAPGEN_WATERSHED_HEADER

#include "mapgen.h"

#define MGWATERSHED_TUNNEL_WIDTH 0.05
#define MGWATERSHED_CAVE_THRESHOLD 1.0
#define MGWATERSHED_LAVA_CAVE_THRESHOLD 0.0
#define MGWATERSHED_LAVA_LEVEL -767
#define MGWATERSHED_LAVA_CAVE_LEVEL -743

class BiomeManager;

extern FlagDesc flagdesc_mapgen_watershed[];


struct MapgenWatershedParams : public MapgenSpecificParams {
	u32 spflags;
	NoiseParams np_ridge;
	NoiseParams np_valley_base;
	NoiseParams np_valley;
	NoiseParams np_valley_amp;
	NoiseParams np_plateau;
	NoiseParams np_mountain_amp;
	NoiseParams np_lake;
	NoiseParams np_mountain;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cave3;

	MapgenWatershedParams();
	~MapgenWatershedParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenWatershed : public Mapgen {
public:
	EmergeManager *m_emerge;
	BiomeManager *bmgr;

	int ystride;
	int zstride;
	u32 spflags;

	v3s16 node_min;
	v3s16 node_max;
	v3s16 full_node_min;
	v3s16 full_node_max;

	Noise *noise_ridge;
	Noise *noise_valley_base;
	Noise *noise_valley;
	Noise *noise_valley_amp;
	Noise *noise_plateau;
	Noise *noise_mountain_amp;
	Noise *noise_lake;
	Noise *noise_mountain;
	Noise *noise_cave1;
	Noise *noise_cave2;
	Noise *noise_cave3;

	Noise *noise_heat;
	Noise *noise_humidity;
	Noise *noise_heat_blend;
	Noise *noise_humidity_blend;

	content_t c_stone;
	content_t c_sand;
	content_t c_water_source;
	content_t c_river_water_source;
	content_t c_lava_source;
	content_t c_desert_stone;
	content_t c_ice;
	content_t c_sandstone;

	content_t c_cobble;
	content_t c_stair_cobble;
	content_t c_mossycobble;
	content_t c_sandstonebrick;
	content_t c_stair_sandstonebrick;

	MapgenWatershed(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	~MapgenWatershed();

	virtual void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p);

	void calculateNoise();
	int generateTerrain();
	MgStoneType generateBiomes(float *heat_map, float *humidity_map);
	void dustTopNodes();
	void generateCaves(int max_stone_y);
};

struct MapgenFactoryWatershed : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge)
	{
		return new MapgenWatershed(mgid, params, emerge);
	};

	MapgenSpecificParams *createMapgenParams()
	{
		return new MapgenWatershedParams();
	};
};

#endif
