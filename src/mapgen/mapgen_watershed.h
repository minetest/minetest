/*
Minetest
Copyright (C) 2010-2018 paramat
Copyright (C) 2010-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

////////// Mapgen Watershed flags
#define MGWATERSHED_VOLCANOS 0x01

class BiomeManager;

extern FlagDesc flagdesc_mapgen_watershed[];


struct MapgenWatershedParams : public MapgenParams {
	u32 spflags;

	float map_scale;
	float vertical_scale;
	s16 coast_level;
	float river_width;
	float river_depth;
	float river_bank;

	float cave_width;
	s16 large_cave_depth;
	s16 lava_depth;
	s16 cavern_limit;
	s16 cavern_taper;
	float cavern_threshold;

	NoiseParams np_vent;
	NoiseParams np_base;
	NoiseParams np_valley;
	NoiseParams np_amplitude;
	NoiseParams np_blend;
	NoiseParams np_rough_2d;
	NoiseParams np_rough_3d;
	NoiseParams np_rough_blend;
	NoiseParams np_highland;
	NoiseParams np_select;

	NoiseParams np_filler_depth;
	NoiseParams np_cave1;
	NoiseParams np_cave2;
	NoiseParams np_cavern;

	MapgenWatershedParams();
	~MapgenWatershedParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenWatershed : public MapgenBasic {
public:
	MapgenWatershed(int mapgenid, MapgenWatershedParams *params,
		EmergeManager *emerge);
	~MapgenWatershed();

	virtual MapgenType getType() const { return MAPGEN_WATERSHED; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	void generateTerrain(s16 *stone_surface_max_y, bool *volcanic_placed);

private:
	float div;
	float vertical_scale;
	s16 coast_level;
	float river_width;
	float river_depth;
	float river_bank;
	s16 large_cave_depth;

	NoiseParams np_vent_div;
	NoiseParams np_base_div;
	NoiseParams np_valley_div;
	NoiseParams np_amplitude_div;
	NoiseParams np_blend_div;
	NoiseParams np_rough_2d_div;
	NoiseParams np_rough_3d_div;
	NoiseParams np_rough_blend_div;
	NoiseParams np_highland_div;
	NoiseParams np_select_div;

	Noise *noise_vent;
	Noise *noise_base;
	Noise *noise_valley;
	Noise *noise_amplitude;
	Noise *noise_blend;
	Noise *noise_rough_2d;
	Noise *noise_rough_3d;
	Noise *noise_rough_blend;
	Noise *noise_highland;
	Noise *noise_select;

	content_t c_volcanic_rock;
	content_t c_obsidianbrick;
	content_t c_stair_obsidian_block;
};
