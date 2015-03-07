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

#ifndef MAPGEN_V5_HEADER
#define MAPGEN_V5_HEADER

#include "mapgen.h"

/////////////////// Mapgen V5 flags
#define MGV5_BLOBS 0x01

extern FlagDesc flagdesc_mapgen_v5[];


struct MapgenV5Params : public MapgenSpecificParams {
	u32 spflags;
	NoiseParams np_filler_depth;
	NoiseParams np_factor;
	NoiseParams np_height;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_ground;
	NoiseParams np_crumble;
	NoiseParams np_wetness;

	MapgenV5Params();
	~MapgenV5Params() {}

	void readParams(Settings *settings);
	void writeParams(Settings *settings);
};


class MapgenV5 : public Mapgen {
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

	Noise *noise_filler_depth;
	Noise *noise_factor;
	Noise *noise_height;
	Noise *noise_cave1;
	Noise *noise_cave2;
	Noise *noise_ground;
	Noise *noise_crumble;
	Noise *noise_wetness;
	Noise *noise_heat;
	Noise *noise_humidity;

	content_t c_stone;
	content_t c_dirt;
	content_t c_dirt_with_grass;
	content_t c_sand;
	content_t c_water_source;
	content_t c_lava_source;
	content_t c_ice;
	content_t c_gravel;
	content_t c_cobble;
	content_t c_desert_sand;
	content_t c_desert_stone;
	content_t c_mossycobble;
	content_t c_sandbrick;
	content_t c_stair_cobble;
	content_t c_stair_sandstone;

	MapgenV5(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	~MapgenV5();

	virtual void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p);
	void calculateNoise();
	int generateBaseTerrain();
	void generateBiomes();
	void generateCaves();
	void generateBlobs();
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
