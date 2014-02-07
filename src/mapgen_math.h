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

#ifndef MAPGEN_MATH_HEADER
#define MAPGEN_MATH_HEADER

#include "mapgen.h"
#include "mapgen_v7.h"
#include "json/json.h"

struct MapgenMathParams : public MapgenV7Params {

	MapgenMathParams() {}
	~MapgenMathParams() {}

	Json::Value params;

	void readParams(Settings *settings);
	void writeParams(Settings *settings);
};

class MapgenMath : public MapgenV7 {
	public:
		MapgenMathParams * mg_params;

		MapgenMath(int mapgenid, MapgenParams *mg_params, EmergeManager *emerge);
		~MapgenMath();

		int generateTerrain();
		int getGroundLevelAtPoint(v2s16 p);

		bool invert;
		double size;
		double scale;
		v3f center;
		int iterations;
		double distance;
		double (*func)(double, double, double, double, int);

};

struct MapgenFactoryMath : public MapgenFactory {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge) {
		return new MapgenMath(mgid, params, emerge);
	};

	MapgenSpecificParams *createMapgenParams() {
		return new MapgenMathParams();
	};
};

#endif
