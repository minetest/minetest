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


struct MapgenDSParams : public MapgenParams
{
	NoiseParams np_filler_depth;

	MapgenDSParams();
	~MapgenDSParams();

	void readParams(const Settings *settings) {}
	void writeParams(Settings *settings) const {}
};

class DiamondSquareMountain
{
	
public:
	DiamondSquareMountain(int n, double left_top, double right_top, double left_bottom, double right_bottom, double center);
	~DiamondSquareMountain();

	void process();
	double get_value(int x, int z);
	void   set_value(int z, int x, double h);

	double **height_map;
	int size;
	int n;

	// random divisor
	double big_rnd;

	// random divisor for small cells
	double small_rnd;
	int small_rnd_size;

	double minimal;
};

class MapgenDS : public MapgenBasic
{
public:
	content_t c_node;
	u8 set_light;

	MapgenDS(MapgenDSParams *params, EmergeParams *emerge);
	~MapgenDS();

	virtual MapgenType getType() const { return MAPGEN_DIAMOND_SQUARE; }

	void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
private:
	DiamondSquareMountain *m;
	int offset_x;
	int offset_z;
};

