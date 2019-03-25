/*
Minetest
Copyright (C) 2017-2018 vlapsley, Vaughan Lapsley <vlapsley@gmail.com>
Copyright (C) 2010-2018 paramat
Copyright (C) 2010-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

///////// Mapgen Carpathian flags
#define MGCARPATHIAN_CAVERNS 0x01

class BiomeManager;

extern FlagDesc flagdesc_mapgen_carpathian[];


struct MapgenCarpathianParams : public MapgenParams
{
	float base_level       = 12.0f;

	u32 spflags            = MGCARPATHIAN_CAVERNS;
	float cave_width       = 0.09f;
	s16 large_cave_depth   = -33;
	s16 lava_depth         = -256;
	s16 cavern_limit       = -256;
	s16 cavern_taper       = 256;
	float cavern_threshold = 0.7f;
	s16 dungeon_ymin       = -31000;
	s16 dungeon_ymax       = 31000;

	NoiseParams np_filler_depth;
	NoiseParams np_height1;
	NoiseParams np_height2;
	NoiseParams np_height3;
	NoiseParams np_height4;
	NoiseParams np_hills_terrain;
	NoiseParams np_ridge_terrain;
	NoiseParams np_step_terrain;
	NoiseParams np_hills;
	NoiseParams np_ridge_mnt;
	NoiseParams np_step_mnt;
	NoiseParams np_mnt_var;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;

	MapgenCarpathianParams();
	~MapgenCarpathianParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenCarpathian : public MapgenBasic
{
public:
	MapgenCarpathian(MapgenCarpathianParams *params, EmergeManager *emerge);
	~MapgenCarpathian();

	virtual MapgenType getType() const { return MAPGEN_CARPATHIAN; }

	float getSteps(float noise);
	inline float getLerp(float noise1, float noise2, float mod);

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);

private:
	float base_level;
	s32 grad_wl;

	s16 large_cave_depth;
	s16 dungeon_ymin;
	s16 dungeon_ymax;

	Noise *noise_height1;
	Noise *noise_height2;
	Noise *noise_height3;
	Noise *noise_height4;
	Noise *noise_hills_terrain;
	Noise *noise_ridge_terrain;
	Noise *noise_step_terrain;
	Noise *noise_hills;
	Noise *noise_ridge_mnt;
	Noise *noise_step_mnt;
	Noise *noise_mnt_var;

	float terrainLevelAtPoint(s16 x, s16 z);
	int generateTerrain();
};
