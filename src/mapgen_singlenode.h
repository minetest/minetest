/*
Minetest
Copyright (C) 2013-2015 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2017 paramat

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

#ifndef MAPGEN_SINGLENODE_HEADER
#define MAPGEN_SINGLENODE_HEADER

#include "mapgen.h"

struct MapgenSinglenodeParams : public MapgenParams
{
	MapgenSinglenodeParams() {}
	~MapgenSinglenodeParams() {}

	void readParams(const Settings *settings) {}
	void writeParams(Settings *settings) const {}
};

class MapgenSinglenode : public Mapgen
{
public:
	u32 flags;
	content_t c_node;
	u8 set_light;

	MapgenSinglenode(int mapgenid, MapgenParams *params, EmergeManager *emerge);
	~MapgenSinglenode();

	virtual MapgenType getType() const { return MAPGEN_SINGLENODE; }

	void makeChunk(BlockMakeData *data);
	int getSpawnLevelAtPoint(v2s16 p);
};

#endif
