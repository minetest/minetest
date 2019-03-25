/*
Minetest
Copyright (C) 2014-2018 paramat
Copyright (C) 2014-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

///////// Mapgen V5 flags
#define MGV5_CAVERNS 0x01

class BiomeManager;

extern FlagDesc flagdesc_mapgen_v5[];

struct MapgenV5Params : public MapgenParams
{
	u32 spflags = MGV5_CAVERNS;
	float cave_width = 0.09f;
	s16 large_cave_depth = -256;
	s16 lava_depth = -256;
	s16 cavern_limit = -256;
	s16 cavern_taper = 256;
	float cavern_threshold = 0.7f;
	s16 dungeon_ymin = -31000;
	s16 dungeon_ymax = 31000;

	NoiseParams np_filler_depth;
	NoiseParams np_factor;
	NoiseParams np_height;
	NoiseParams np_ground;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;

	MapgenV5Params();
	~MapgenV5Params() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenV5 : public MapgenBasic
{
public:
	MapgenV5(MapgenV5Params *params, EmergeManager *emerge);
	~MapgenV5();

	virtual MapgenType getType() const { return MAPGEN_V5; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	int generateBaseTerrain();

private:
	s16 large_cave_depth;
	s16 dungeon_ymin;
	s16 dungeon_ymax;

	Noise *noise_factor;
	Noise *noise_height;
	Noise *noise_ground;
};
