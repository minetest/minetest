/*
Minetest Valleys C
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 paramat, Matt Gregory
Copyright (C) 2016 Duane Robertson <duane@duanerobertson.com>

Based on Valleys Mapgen by Gael de Sailly
 (https://forum.minetest.net/viewtopic.php?f=9&t=11430)
and mapgen_v7 by kwolekr and paramat.

Licensing changed by permission of Gael de Sailly.

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

#ifndef MAPGEN_VALLEYS_HEADER
#define MAPGEN_VALLEYS_HEADER

#include "mapgen.h"

////////////// Mapgen Valleys flags
#define MGVALLEYS_ALT_CHILL    0x01
#define MGVALLEYS_HUMID_RIVERS 0x02

// Feed only one variable into these.
#define MYSQUARE(x) (x) * (x)
#define MYCUBE(x) (x) * (x) * (x)

class BiomeManager;
class BiomeGenOriginal;

// Global profiler
//class Profiler;
//extern Profiler *mapgen_profiler;


struct MapgenValleysParams : public MapgenParams {
	u32 spflags;
	s16 large_cave_depth;
	s16 massive_cave_depth;
	u16 altitude_chill;
	u16 lava_features;
	u16 river_depth;
	u16 river_size;
	u16 water_features;
	float cave_width;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_filler_depth;
	NoiseParams np_inter_valley_fill;
	NoiseParams np_inter_valley_slope;
	NoiseParams np_rivers;
	NoiseParams np_massive_caves;
	NoiseParams np_terrain_height;
	NoiseParams np_valley_depth;
	NoiseParams np_valley_profile;

	MapgenValleysParams();
	~MapgenValleysParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

struct TerrainNoise {
	s16 x;
	s16 z;
	float terrain_height;
	float *rivers;
	float *valley;
	float valley_profile;
	float *slope;
	float inter_valley_fill;
};

class MapgenValleys : public MapgenBasic {
public:

	MapgenValleys(int mapgenid, MapgenValleysParams *params, EmergeManager *emerge);
	~MapgenValleys();

	virtual MapgenType getType() const { return MAPGEN_VALLEYS; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);

	s16 large_cave_depth;

private:
	BiomeGenOriginal *m_bgen;

	bool humid_rivers;
	bool use_altitude_chill;
	float humidity_adjust;
	s16 cave_water_max_height;
	s16 lava_max_height;

	float altitude_chill;
	s16 lava_features_lim;
	s16 massive_cave_depth;
	float river_depth_bed;
	float river_size_factor;
	float *tcave_cache;
	s16 water_features_lim;
	Noise *noise_inter_valley_fill;
	Noise *noise_inter_valley_slope;
	Noise *noise_rivers;
	Noise *noise_cave1;
	Noise *noise_cave2;
	Noise *noise_massive_caves;
	Noise *noise_terrain_height;
	Noise *noise_valley_depth;
	Noise *noise_valley_profile;

	float terrainLevelAtPoint(s16 x, s16 z);

	void calculateNoise();

	virtual int generateTerrain();
	float terrainLevelFromNoise(TerrainNoise *tn);
	float adjustedTerrainLevelFromNoise(TerrainNoise *tn);

	virtual void generateCaves(s16 max_stone_y, s16 large_cave_depth);
};

#endif
