/*
Minetest
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 paramat, Matt Gregory

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

#ifndef MAPGEN_FLAT_HEADER
#define MAPGEN_FLAT_HEADER

#include "mapgen.h"

/////// Mapgen Flat flags
#define MGFLAT_LAKES 0x01
#define MGFLAT_HILLS 0x02

class BiomeManager;

extern FlagDesc flagdesc_mapgen_flat[];

struct MapgenFlatParams : public MapgenParams
{
	u32 spflags;
	s16 ground_level;
	s16 large_cave_depth;
	float cave_width;
	float lake_threshold;
	float lake_steepness;
	float hill_threshold;
	float hill_steepness;
	NoiseParams np_terrain;
	NoiseParams np_filler_depth;
	NoiseParams np_cave1;
	NoiseParams np_cave2;

	MapgenFlatParams();
	~MapgenFlatParams() {}

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};

class MapgenFlat : public MapgenBasic
{
public:
	MapgenFlat(int mapgenid, MapgenFlatParams *params, EmergeManager *emerge);
	~MapgenFlat();

	virtual MapgenType getType() const { return MAPGEN_FLAT; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
	s16 generateTerrain();

private:
	s16 ground_level;
	s16 large_cave_depth;
	float lake_threshold;
	float lake_steepness;
	float hill_threshold;
	float hill_steepness;
	Noise *noise_terrain;
};

#endif
