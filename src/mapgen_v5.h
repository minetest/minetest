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

#ifndef MAPGEN_V5_HEADER
#define MAPGEN_V5_HEADER

#include "mapgen.h"

#define MGV5_LARGE_CAVE_DEPTH -256

class BiomeManager;

extern FlagDesc flagdesc_mapgen_v5[];


struct MapgenV5Params : public MapgenSpecificParams {
	u32 spflags;
	float cave_width;
	NoiseParams np_filler_depth;
	NoiseParams np_factor;
	NoiseParams np_height;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_ground;

	MapgenV5Params();
	~MapgenV5Params() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};


class MapgenV5 : public Mapgen {
public:
	EmergeManager *m_emerge;
	BiomeManager *bmgr;

	int ystride;
	int zstride_1d;

	v3s16 node_min;
	v3s16 node_max;
	v3s16 full_node_min;
	v3s16 full_node_max;

	u32 spflags;
	float cave_width;
	Noise *noise_filler_depth;
	Noise *noise_factor;
	Noise *noise_height;
	Noise *noise_cave1;
	Noise *noise_cave2;
	Noise *noise_ground;

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

	MapgenV5(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	~MapgenV5();

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	void calculateNoise();
	int generateBaseTerrain();
	MgStoneType generateBiomes(float *heat_map, float *humidity_map);
	void generateCaves(int max_stone_y);
	void dustTopNodes();
};


struct MapgenFactoryV5 : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge)
	{
		return new MapgenV5(mgid, params, emerge);
	};

	MapgenSpecificParams *createMapgenParams()
	{
		return new MapgenV5Params();
	};
};

#endif
