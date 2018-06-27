/*
Minetest
Copyright (C) 2016-2018 Duane Robertson <duane@duanerobertson.com>
Copyright (C) 2016-2018 paramat

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

#pragma once

#include "mapgen.h"

////////////// Mapgen Valleys flags
#define MGVALLEYS_ALT_CHILL    0x01
#define MGVALLEYS_HUMID_RIVERS 0x02

// Feed only one variable into these
#define MYSQUARE(x) (x) * (x)
#define MYCUBE(x) (x) * (x) * (x)

class BiomeManager;
class BiomeGenOriginal;


struct MapgenValleysParams : public MapgenParams {
	u32 spflags = MGVALLEYS_HUMID_RIVERS | MGVALLEYS_ALT_CHILL;
	u16 altitude_chill = 90; // The altitude at which temperature drops by 20C
	u16 river_depth = 4; // How deep to carve river channels
	u16 river_size = 5; // How wide to make rivers

	float cave_width = 0.09f;
	s16 large_cave_depth = -33;
	s16 lava_depth = 1;
	s16 cavern_limit = -256;
	s16 cavern_taper = 192;
	float cavern_threshold = 0.6f;
	s16 dungeon_ymin = -31000;
	s16 dungeon_ymax = 63; // No higher than surface mapchunks

	NoiseParams np_filler_depth;
	NoiseParams np_inter_valley_fill;
	NoiseParams np_inter_valley_slope;
	NoiseParams np_rivers;
	NoiseParams np_terrain_height;
	NoiseParams np_valley_depth;
	NoiseParams np_valley_profile;

	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;

	MapgenValleysParams();
	~MapgenValleysParams() = default;

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

private:
	BiomeGenOriginal *m_bgen;

	float altitude_chill;
	bool humid_rivers;
	bool use_altitude_chill;
	float humidity_adjust;
	float river_depth_bed;
	float river_size_factor;

	s16 large_cave_depth;
	s16 dungeon_ymin;
	s16 dungeon_ymax;

	Noise *noise_inter_valley_fill;
	Noise *noise_inter_valley_slope;
	Noise *noise_rivers;
	Noise *noise_terrain_height;
	Noise *noise_valley_depth;
	Noise *noise_valley_profile;

	float terrainLevelAtPoint(s16 x, s16 z);
	void calculateNoise();
	virtual int generateTerrain();
	float terrainLevelFromNoise(TerrainNoise *tn);
	float adjustedTerrainLevelFromNoise(TerrainNoise *tn);
};
