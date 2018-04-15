/*
Minetest
Copyright (C) 2018 paramat

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

class BiomeManager;

extern FlagDesc flagdesc_mapgen_planet[];


struct MapgenPlanetParams : public MapgenParams
{
	u32 spflags            = 0;
	float cave_width       = 0.09f;
	s16 large_cave_depth   = -33;
	s16 lava_depth         = -33;
	s16 dungeon_ymin       = -31000;
	s16 dungeon_ymax       = 31000;

	NoiseParams np_filler_depth;
	NoiseParams np_terrain;
	NoiseParams np_cave1;
	NoiseParams np_cave2;

	MapgenPlanetParams();
	~MapgenPlanetParams() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
};


class MapgenPlanet : public MapgenBasic
{
public:
	MapgenPlanet(int mapgenid, MapgenPlanetParams *params,
			EmergeManager *emerge);
	~MapgenPlanet();

	virtual MapgenType getType() const { return MAPGEN_PLANET; }

	virtual void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);

private:
	s16 large_cave_depth;
	s16 dungeon_ymin;
	s16 dungeon_ymax;

	Noise *noise_terrain;

	int generateAir();
	int generateTerrain();
};
