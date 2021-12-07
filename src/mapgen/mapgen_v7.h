/*
Minetest
Copyright (C) 2014-2020 paramat
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

#include "mapgen.h"

///////////// Mapgen V7 flags
#define MGV7_MOUNTAINS   0x01
#define MGV7_RIDGES      0x02
#define MGV7_FLOATLANDS  0x04
#define MGV7_CAVERNS     0x08
#define MGV7_BIOMEREPEAT 0x10 // Now unused

class BiomeManager;

extern FlagDesc flagdesc_mapgen_v7[];


struct MapgenV7Params : public MapgenParams {
	POS mount_zero_level = 0;
	POS floatland_ymin = 1024;
    POS floatland_ymax = 4096;
	POS floatland_taper = 256;
	float float_taper_exp = 2.0f;
	float floatland_density = -0.6f;
	POS floatland_ywater = -MAX_MAP_GENERATION_LIMIT;

	float cave_width = 0.09f;
	POS large_cave_depth = -33;
	u16 small_cave_num_min = 0;
	u16 small_cave_num_max = 0;
	u16 large_cave_num_min = 0;
	u16 large_cave_num_max = 2;
	float large_cave_flooded = 0.5f;
	POS cavern_limit = -256;
	POS cavern_taper = 256;
	float cavern_threshold = 0.7f;
	POS dungeon_ymin = -MAX_MAP_GENERATION_LIMIT;
	POS dungeon_ymax = MAX_MAP_GENERATION_LIMIT;

	NoiseParams np_terrain_base;
	NoiseParams np_terrain_alt;
	NoiseParams np_terrain_persist;
	NoiseParams np_height_select;
	NoiseParams np_filler_depth;
	NoiseParams np_mount_height;
	NoiseParams np_ridge_uwater;
	NoiseParams np_mountain;
	NoiseParams np_ridge;
	NoiseParams np_floatland;
	NoiseParams np_cavern;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_dungeons;

	MapgenV7Params();
	~MapgenV7Params() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};


class MapgenV7 : public MapgenBasic {
public:
	MapgenV7(MapgenV7Params *params, EmergeParams *emerge);
	~MapgenV7();

	virtual MapgenType getType() const { return MAPGEN_V7; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2POS p);

	float baseTerrainLevelAtPoint(POS x, POS z);
	float baseTerrainLevelFromMap(int index);
	bool getMountainTerrainAtPoint(POS x, POS y, POS z);
	bool getMountainTerrainFromMap(int idx_xyz, int idx_xz, POS y);
	bool getRiverChannelFromMap(int idx_xyz, int idx_xz, POS y);
	bool getFloatlandTerrainFromMap(int idx_xyz, float float_offset);

	int generateTerrain();

private:
	POS mount_zero_level;
	POS floatland_ymin;
	POS floatland_ymax;
	POS floatland_taper;
	float float_taper_exp;
	float floatland_density;
	POS floatland_ywater;

	float *float_offset_cache = nullptr;

	Noise *noise_terrain_base;
	Noise *noise_terrain_alt;
	Noise *noise_terrain_persist;
	Noise *noise_height_select;
	Noise *noise_mount_height;
	Noise *noise_ridge_uwater;
	Noise *noise_mountain;
	Noise *noise_ridge;
	Noise *noise_floatland;
};
