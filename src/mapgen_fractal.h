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

#ifndef MAPGEN_FRACTAL_HEADER
#define MAPGEN_FRACTAL_HEADER

#include "mapgen.h"

#define MGFRACTAL_LARGE_CAVE_DEPTH -32

/////////////////// Mapgen Fractal flags
#define MGFRACTAL_JULIA 0x01

class BiomeManager;

extern FlagDesc flagdesc_mapgen_fractal[];


struct MapgenFractalParams : public MapgenSpecificParams {
	u32 spflags;

	u16 m_iterations;
	v3f m_scale;
	v3f m_offset;
	float m_slice_w;

	u16 j_iterations;
	v3f j_scale;
	v3f j_offset;
	float j_slice_w;
	float julia_x;
	float julia_y;
	float julia_z;
	float julia_w;

	NoiseParams np_seabed;
	NoiseParams np_filler_depth;
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

	u16 m_iterations;
	v3f m_scale;
	v3f m_offset;
	float m_slice_w;

	u16 j_iterations;
	v3f j_scale;
	v3f j_offset;
	float j_slice_w;
	float julia_x;
	float julia_y;
	float julia_z;
	float julia_w;

	Noise *noise_seabed;
	Noise *noise_filler_depth;
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
	bool getFractalAtPoint(s16 x, s16 y, s16 z);
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
