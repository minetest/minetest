/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#ifndef MG_BIOME_HEADER
#define MG_BIOME_HEADER

#include "mapgen.h"
#include "noise.h"

//#include <string>
//#include "nodedef.h"
//#include "gamedef.h"
//#include "mapnode.h"

enum BiomeTerrainType
{
	BIOME_TERRAIN_NORMAL,
	BIOME_TERRAIN_LIQUID,
	BIOME_TERRAIN_NETHER,
	BIOME_TERRAIN_AETHER,
	BIOME_TERRAIN_FLAT
};

extern NoiseParams nparams_biome_def_heat;
extern NoiseParams nparams_biome_def_humidity;


struct BiomeNoiseInput {
	v2s16 mapsize;
	float *heat_map;
	float *humidity_map;
	s16 *height_map;
};

class Biome : public GenElement {
public:
	u32 flags;

	content_t c_top;
	content_t c_filler;
	content_t c_water;
	content_t c_dust;
	content_t c_dust_water;

	s16 depth_top;
	s16 depth_filler;

	s16 height_min;
	s16 height_max;
	float heat_point;
	float humidity_point;
};

class BiomeManager : public GenElementManager {
public:
	static const char *ELEMENT_TITLE;
	static const size_t ELEMENT_LIMIT = 0x100;

	NoiseParams *np_heat;
	NoiseParams *np_humidity;

	BiomeManager(IGameDef *gamedef);
	~BiomeManager();

	Biome *create(int btt)
	{
		return new Biome;
	}

	void calcBiomes(BiomeNoiseInput *input, u8 *biomeid_map);
	Biome *getBiome(float heat, float humidity, s16 y);
};

#endif
