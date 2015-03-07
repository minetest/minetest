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

struct NoiseParams;

enum BiomeType
{
	BIOME_TYPE_NORMAL,
	BIOME_TYPE_LIQUID,
	BIOME_TYPE_NETHER,
	BIOME_TYPE_AETHER,
	BIOME_TYPE_FLAT
};

class Biome : public GenElement, public NodeResolver {
public:
	u32 flags;

	content_t c_top;
	content_t c_filler;
	content_t c_shore_top;
	content_t c_shore_filler;
	content_t c_underwater;
	content_t c_stone;
	content_t c_water_top;
	content_t c_water;
	content_t c_dust;

	s16 depth_top;
	s16 depth_filler;
	s16 height_shore;
	s16 depth_water_top;

	s16 y_min;
	s16 y_max;
	float heat_point;
	float humidity_point;

	virtual void resolveNodeNames(NodeResolveInfo *nri);
};

class BiomeManager : public GenElementManager {
public:
	static const char *ELEMENT_TITLE;
	static const size_t ELEMENT_LIMIT = 0x100;

	BiomeManager(IGameDef *gamedef);
	~BiomeManager();

	Biome *create(int btt)
	{
		return new Biome;
	}

	void clear();

	void calcBiomes(s16 sx, s16 sy, float *heat_map, float *humidity_map,
		s16 *height_map, u8 *biomeid_map);
	Biome *getBiome(float heat, float humidity, s16 y);
};

#endif

