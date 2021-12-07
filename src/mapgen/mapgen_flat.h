/*
Minetest
Copyright (C) 2015-2020 paramat
Copyright (C) 2015-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

/////// Mapgen Flat flags
#define MGFLAT_LAKES 0x01
#define MGFLAT_HILLS 0x02
#define MGFLAT_CAVERNS 0x04

class BiomeManager;

extern FlagDesc flagdesc_mapgen_flat[];

struct MapgenFlatParams : public MapgenParams
{
	pos_t ground_level = 8;
	float lake_threshold = -0.45f;
	float lake_steepness = 48.0f;
	float hill_threshold = 0.45f;
	float hill_steepness = 64.0f;

	float cave_width = 0.09f;
	u16 small_cave_num_min = 0;
	u16 small_cave_num_max = 0;
	u16 large_cave_num_min = 0;
	u16 large_cave_num_max = 2;
	pos_t large_cave_depth = -33;
	float large_cave_flooded = 0.5f;
	pos_t cavern_limit = -256;
	pos_t cavern_taper = 256;
	float cavern_threshold = 0.7f;
	pos_t dungeon_ymin = -MAX_MAP_GENERATION_LIMIT;
	pos_t dungeon_ymax = MAX_MAP_GENERATION_LIMIT;

	NoiseParams np_terrain;
	NoiseParams np_filler_depth;
	NoiseParams np_cavern;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_dungeons;

	MapgenFlatParams();
	~MapgenFlatParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenFlat : public MapgenBasic
{
public:
	MapgenFlat(MapgenFlatParams *params, EmergeParams *emerge);
	~MapgenFlat();

	virtual MapgenType getType() const { return MAPGEN_FLAT; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2pos_t p);
	pos_t generateTerrain();

private:
	pos_t ground_level;
	float lake_threshold;
	float lake_steepness;
	float hill_threshold;
	float hill_steepness;

	Noise *noise_terrain;
};
