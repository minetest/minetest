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
	BIOME_NORMAL,
	BIOME_LIQUID,
	BIOME_NETHER,
	BIOME_AETHER,
	BIOME_FLAT
};

class Biome : public ObjDef, public NodeResolver {
public:
	u32 flags;

	content_t c_top;
	content_t c_filler;
	content_t c_stone;
	content_t c_water_top;
	content_t c_water;
	content_t c_dust;

	s16 depth_top;
	s16 depth_filler;
	s16 depth_water_top;

	s16 y_min;
	s16 y_max;
	float heat_point;
	float humidity_point;

	virtual void resolveNodeNames();
};

class BiomeManager : public ObjDefManager {
public:
	static const char *OBJECT_TITLE;

	BiomeManager(IGameDef *gamedef);
	~BiomeManager();

	const char *getObjectTitle() const
	{
		return "biome";
	}

	static Biome *create(BiomeType type)
	{
		return new Biome;
	}

	void clear();

	void calcBiomes(s16 sx, s16 sy, float *heat_map, float *humidity_map,
		s16 *height_map, u8 *biomeid_map);
	Biome *getBiome(float heat, float humidity, s16 y);

private:
	IGameDef *m_gamedef;
};

#endif

