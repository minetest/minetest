/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Additional development and fractal code by paramat

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

#ifndef MAPGEN_FRACTAL_HEADER
#define MAPGEN_FRACTAL_HEADER

#include "mapgen.h"

/////////////////// Mapgen Fractal flags
#define MGFRACTAL_JULIA 0x01

class BiomeManager;

extern FlagDesc flagdesc_mapgen_fractal[];


struct MapgenFractalParams : public MapgenSpecificParams {
	u32 spflags;

	u16 iterations;
	float scale_x;
	float scale_y;
	float scale_z;
	float offset_x;
	float offset_y;
	float offset_z;
	float slice_w;
	float julia_x;
	float julia_y;
	float julia_z;
	float julia_w;

	NoiseParams np_cave1;
	NoiseParams np_cave2;

	MapgenFractalParams();
	~MapgenFractalParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenFractal : public Mapgen {
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

	u16 iterations;
	float scale_x;
	float scale_y;
	float scale_z;
	float offset_x;
	float offset_y;
	float offset_z;
	float slice_w;
	float julia_x;
	float julia_y;
	float julia_z;
	float julia_w;

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

	MapgenFractal(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	~MapgenFractal();

	virtual void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p);
	void calculateNoise();
	bool getTerrainAtPoint(s16 x, s16 y, s16 z);
	s16 generateTerrain();
	MgStoneType generateBiomes(float *heat_map, float *humidity_map);
	void dustTopNodes();
	void generateCaves(s16 max_stone_y);
};

struct MapgenFactoryFractal : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge)
	{
		return new MapgenFractal(mgid, params, emerge);
	};

	MapgenSpecificParams *createMapgenParams()
	{
		return new MapgenFractalParams();
	};
};

#endif
