/*
Minetest
Copyright (C) 2013-2018 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2018 paramat
Copyright (C) 2021 Vladislav Tsendrovskii <ctcendrovskii@gmail.com>

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
#include <list>
#include "diamond_square/diamond_square.h"

struct MapgenDSParams : public MapgenParams
{
	NoiseParams np_filler_depth;

	MapgenDSParams();
	~MapgenDSParams();

	void readParams(const Settings *settings) {}
	void writeParams(Settings *settings) const {}
};


class MapgenDS : public MapgenBasic
{
public:
	content_t c_node;
	u8 set_light;

	MapgenDS(MapgenDSParams *params, EmergeParams *emerge);
	~MapgenDS();

    void AddMountainRange(double height, int z1, int x1, int z2, int x2)
    void AddVolcanicIsland(double height, int z, int x);

	virtual MapgenType getType() const { return MAPGEN_DIAMOND_SQUARE; }

    s16 generateBaseTerrain();
	void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
private:
    PseudoRandom prandom;
	MountainLandscape *landscape;
	int offset_x;
	int offset_z;
};

