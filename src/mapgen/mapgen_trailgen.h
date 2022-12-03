/*
Minetest
Copyright (C) 2015-2020 paramat
Copyright (C) 2015-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2021-2022 cartmic, Michael Carter

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

/////// Mapgen Trailgen flags
#define MGTRAILGEN_CAVERNS 0x04

class BiomeManager;

extern FlagDesc flagdesc_mapgen_trailgen[];

struct MapgenTrailgenParams : public MapgenParams
{
	float cave_width = 0.09f;
	u16 small_cave_num_min = 0;
	u16 small_cave_num_max = 0;
	u16 large_cave_num_min = 0;
	u16 large_cave_num_max = 2;
	s16 large_cave_depth = -33;
	float large_cave_flooded = 0.5f;
	s16 cavern_limit = -256;
	s16 cavern_taper = 256;
	float cavern_threshold = 0.7f;
	u16 map_height_mod = 1;
	s16 dungeon_ymin = -31000;
	s16 dungeon_ymax = 31000;

	NoiseParams np_terrain;
	NoiseParams np_terrain_higher;
	NoiseParams np_steepness;
	NoiseParams np_height_select;
	NoiseParams np_filler_depth;
	NoiseParams np_cavern;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_dungeons;

	MapgenTrailgenParams();
	~MapgenTrailgenParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};

class MapgenTrailgen : public MapgenBasic
{
public:
	MapgenTrailgen(MapgenTrailgenParams *params, EmergeParams *emerge);
	~MapgenTrailgen();

	virtual MapgenType getType() const { return MAPGEN_TRAILGEN; }

	virtual void makeChunk(BlockMakeData *data);

	int getGroundLevelAtPoint(v2s16 p);
	int getSpawnLevelAtPoint(v2s16 p);

	float baseTerrainLevel(float terrain_base, float terrain_higher,
		float steepness, float height_select);

	virtual float baseTerrainLevelFromNoise(v2s16 p);
	virtual float baseTerrainLevelFromMap(v2s16 p);
	virtual float baseTerrainLevelFromMap(int index);

	s16 generateTerrain();

	virtual void calculateNoise();

private:
	Noise *noise_terrain;
	Noise *noise_terrain_higher;
	Noise *noise_steepness;
	Noise *noise_height_select;
};
