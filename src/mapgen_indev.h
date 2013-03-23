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

#ifndef MAPGENINDEV_HEADER
#define MAPGENINDEV_HEADER

#include "mapgen.h"
#include "mapgen_v6.h"

float farscale(float scale, float x, float y, float z);

struct NoiseIndevParams : public NoiseParams {
	float farscale;

	NoiseIndevParams(){}
	NoiseIndevParams(float offset_, float scale_, v3f spread_, int seed_, int octaves_, float persist_, float farscale_) 
	    //:NoiseParams(offset_, scale_, spread_, seed_, octaves_,  persist_)
	{
	//NoiseParams(float offset_, float scale_, v3f spread_, int seed_, int octaves_, float persist_) {
		offset = offset_;
		scale = scale_;
		spread = spread_;
		seed = seed_;
		octaves = octaves_;
		persist = persist_;
	//}
		farscale = farscale_;
	}

};

#define getNoiseIndevParams(x) getStruct<NoiseIndevParams>((x), "f,f,v3,s32,s32,f,f")
#define setNoiseIndevParams(x, y) setStruct((x), "f,f,v3,s32,s32,f,f", (y))

class NoiseIndev : public Noise {
    public:
	NoiseIndevParams *npindev;

	//NoiseIndev() {};
	NoiseIndev(NoiseIndevParams *np, int seed, int sx, int sy);
	NoiseIndev(NoiseIndevParams *np, int seed, int sx, int sy, int sz);
	void init(NoiseIndevParams *np, int seed, int sx, int sy, int sz);
	void transformNoiseMapFarScale(float xx = 0, float yy = 0, float zz = 0);
};

extern NoiseIndevParams nparams_indev_def_terrain_base;
extern NoiseIndevParams nparams_indev_def_terrain_higher;
extern NoiseIndevParams nparams_indev_def_steepness;
//extern NoiseIndevParams nparams_indev_def_height_select;
//extern NoiseIndevParams nparams_indev_def_trees;
extern NoiseIndevParams nparams_indev_def_mud;
//extern NoiseIndevParams nparams_indev_def_beach;
//extern NoiseIndevParams nparams_indev_def_biome;
//extern NoiseIndevParams nparams_indev_def_cave;


struct MapgenIndevParams : public MapgenV6Params {
	NoiseIndevParams *npindev_terrain_base;
	NoiseIndevParams *npindev_terrain_higher;
	NoiseIndevParams *npindev_steepness;
	//NoiseParams *np_height_select;
	//NoiseParams *np_trees;
	NoiseIndevParams *npindev_mud;
	//NoiseParams *np_beach;
	//NoiseParams *np_biome;
	//NoiseParams *np_cave;

	MapgenIndevParams() {
		//freq_desert       = 0.45;
		//freq_beach        = 0.15;
		npindev_terrain_base   = &nparams_indev_def_terrain_base;
		npindev_terrain_higher = &nparams_indev_def_terrain_higher;
		npindev_steepness      = &nparams_indev_def_steepness;
		//np_height_select  = &nparams_v6_def_height_select;
		//np_trees          = &nparams_v6_def_trees;
		npindev_mud            = &nparams_indev_def_mud;
		//np_beach          = &nparams_v6_def_beach;
		//np_biome          = &nparams_v6_def_biome;
		//np_cave           = &nparams_v6_def_cave;
	}

	bool readParams(Settings *settings);
	void writeParams(Settings *settings);
};

class MapgenIndev : public MapgenV6 {
    public:
	NoiseIndev *noiseindev_terrain_base;
	NoiseIndev *noiseindev_terrain_higher;
	NoiseIndev *noiseindev_steepness;
	//NoiseIndev *noise_height_select;
	//NoiseIndev *noise_trees;
	NoiseIndev *noiseindev_mud;
	//NoiseIndev *noise_beach;
	//NoiseIndev *noise_biome;
	//NoiseIndevParams *np_cave;

	MapgenIndev(int mapgenid, MapgenIndevParams *params);
	~MapgenIndev();
	void calculateNoise();

	float baseTerrainLevelFromNoise(v2s16 p);
	float baseTerrainLevelFromMap(int index);
	float getMudAmount(int index);
	void defineCave(Cave & cave, PseudoRandom ps, v3s16 node_min, bool large_cave);
};

struct MapgenFactoryIndev : public MapgenFactoryV6 {
	Mapgen *createMapgen(int mgid, MapgenParams *params, EmergeManager *emerge) {
		return new MapgenIndev(mgid, (MapgenIndevParams *)params);
	};

	MapgenParams *createMapgenParams() {
		return new MapgenIndevParams();
	};
};


#endif
