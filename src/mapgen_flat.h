/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef MAPGENFLAT_HEADER
#define MAPGENFLAT_HEADER

#include "mapgen.h"

class Settings;

struct MapgenFlatParams : public MapgenParams {
	bool generate_trees;

	MapgenFlatParams(){
		generate_trees = false;
	};

	bool readParams(Settings *settings){
		settings->setDefault("mgflat_generate_trees", "true");
		generate_trees = settings->getBool("mgflat_generate_trees");
		return true;
	};

	void writeParams(Settings *settings){
		settings->set("mgflat_generate_trees", "true");
	};

};

class MapgenFlat : public Mapgen {
public:
	MapgenFlat(MapgenFlatParams *params);
	void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2s16 p) {
		return 0;
	}
	
private:
	bool generate_trees;
	INodeDefManager *ndef;
};

struct MapgenFactoryFlat : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge) {
		return new MapgenFlat((MapgenFlatParams *)params);
	};
	MapgenParams *createMapgenParams() {
		return new MapgenFlatParams();
	};
};

#endif
