/*
Minetest
Copyright (C) 2013-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2014-2018 paramat

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
	u32 spflags = MGV7_MOUNTAINS | MGV7_RIDGES | MGV7_CAVERNS;
	s16 mount_zero_level = 0;
	float float_mount_density = 0.6f;
	float float_mount_height = 128.0f;
	float float_mount_exponent = 0.75f;
	s16 floatland_level = 1280;
	s16 shadow_limit = 1024;

	float cave_width = 0.09f;
	s16 large_cave_depth = -33;
	s16 lava_depth = -256;
	s16 cavern_limit = -256;
	s16 cavern_taper = 256;
	float cavern_threshold = 0.7f;
	s16 dungeon_ymin = -31000;
	s16 dungeon_ymax = 31000;

	NoiseParams np_terrain_base;
	NoiseParams np_terrain_alt;
	NoiseParams np_terrain_persist;
	NoiseParams np_height_select;
	NoiseParams np_filler_depth;
	NoiseParams np_mount_height;
	NoiseParams np_ridge_uwater;
	NoiseParams np_floatland_base;
	NoiseParams np_float_base_height;
	NoiseParams np_mountain;
	NoiseParams np_ridge;
	NoiseParams np_cavern;
	NoiseParams np_cave1;
	NoiseParams np_cave2;

	MapgenV7Params();
	~MapgenV7Params() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};


class MapgenV7 : public MapgenBasic {
public:
	MapgenV7(MapgenV7Params *params, EmergeManager *emerge);
	~MapgenV7();

	virtual MapgenType getType() const { return MAPGEN_V7; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);

	float baseTerrainLevelAtPoint(s16 x, s16 z);
	float baseTerrainLevelFromMap(int index);
	bool getMountainTerrainAtPoint(s16 x, s16 y, s16 z);
	bool getMountainTerrainFromMap(int idx_xyz, int idx_xz, s16 y);
	bool getFloatlandMountainFromMap(int idx_xyz, int idx_xz, s16 y);
	void floatBaseExtentFromMap(s16 *float_base_min, s16 *float_base_max, int idx_xz);

	int generateTerrain();
	void generateRidgeTerrain();

private:
	s16 mount_zero_level;
	float float_mount_density;
	float float_mount_height;
	float float_mount_exponent;
	s16 floatland_level;
	s16 shadow_limit;

	s16 large_cave_depth;
	s16 dungeon_ymin;
	s16 dungeon_ymax;

	Noise *noise_terrain_base;
	Noise *noise_terrain_alt;
	Noise *noise_terrain_persist;
	Noise *noise_height_select;
	Noise *noise_mount_height;
	Noise *noise_ridge_uwater;
	Noise *noise_floatland_base;
	Noise *noise_float_base_height;
	Noise *noise_mountain;
	Noise *noise_ridge;
};
