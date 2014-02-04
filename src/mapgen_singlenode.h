/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

struct MapgenSinglenodeParams : public MapgenSpecificParams {
	
	MapgenSinglenodeParams() {}
	~MapgenSinglenodeParams() {}
	
	void readParams(Settings *settings);
	void writeParams(Settings *settings);
};

class MapgenSinglenode : public Mapgen {
public:
	u32 flags;

	MapgenSinglenode(int mapgenid, MapgenParams *params);
	~MapgenSinglenode();
	
	void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p);
};

struct MapgenFactorySinglenode : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge) {
		return new MapgenSinglenode(mgid, params);
	};
	
	MapgenSpecificParams *createMapgenParams() {
		return new MapgenSinglenodeParams();
	};
};

#endif
