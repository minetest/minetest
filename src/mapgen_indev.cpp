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

#include "mapgen_indev.h"
#include "constants.h"
#include "log.h"

/////////////////// Mapgen Indev perlin noise default values

NoiseIndevParams nparams_indev_def_terrain_base
	(-AVERAGE_MUD_AMOUNT, 20.0, v3f(250.0, 250.0, 250.0), 82341, 5, 0.6, 1);
NoiseIndevParams nparams_indev_def_terrain_higher
	(20.0, 16.0, v3f(500.0, 500.0, 500.0), 85039, 5, 0.6, 1);
NoiseIndevParams nparams_indev_def_steepness
	(0.85, 0.5, v3f(125.0, 125.0, 125.0), -932, 5, 0.7, 1);
NoiseIndevParams nparams_indev_def_mud
	(AVERAGE_MUD_AMOUNT, 2.0, v3f(200.0, 200.0, 200.0), 91013, 3, 0.55, 1);

///////////////////////////////////////////////////////////////////////////////

void NoiseIndev::init(NoiseIndevParams *np, int seed, int sx, int sy, int sz) {
	Noise::init((NoiseParams*)np, seed, sx, sy, sz);
	this->npindev   = np;
}


NoiseIndev::NoiseIndev(NoiseIndevParams *np, int seed, int sx, int sy) : Noise(np, seed, sx, sy) {
        init(np, seed, sx, sy, 1);
}


NoiseIndev::NoiseIndev(NoiseIndevParams *np, int seed, int sx, int sy, int sz) : Noise(np, seed, sx, sy, sz) {
        init(np, seed, sx, sy, sz);
}


float farscale(float scale, float x, float y, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 3 - (fabs(x) + fabs(y) + fabs(z)) ) / (MAP_GENERATION_LIMIT * 3) ) * (scale - 1) );
}

void NoiseIndev::transformNoiseMapFarScale(float xx, float yy, float zz) {
        // more correct use distantion from 0,0,0 via pow, but + is faster
        //float farscale = ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 3 - (fabs(xx) + fabs(yy) + fabs(zz)) ) / (MAP_GENERATION_LIMIT * 3) ) * ((npindev)->farscale - 1) );
        // dstream << "TNM rs="  << farscale << " from=" << (npindev)->farscale << " x=" << xx << " y=" << yy <<" z=" << zz << std::endl;
        int i = 0;
        for (int z = 0; z != sz; z++) {
                for (int y = 0; y != sy; y++) {
                        for (int x = 0; x != sx; x++) {
                                //result[i] = result[i] * npindev->scale * farscale + npindev->offset;
                                result[i] = result[i] * npindev->scale * farscale(npindev->farscale,xx,yy,zz) + npindev->offset;
                                i++;
                        }
                }
        }
}

MapgenIndev::MapgenIndev(int mapgenid, MapgenIndevParams *params) : MapgenV6(mapgenid, params) {
        noiseindev_terrain_base   = new NoiseIndev(params->npindev_terrain_base,   seed, csize.X, csize.Y);
        noiseindev_terrain_higher = new NoiseIndev(params->npindev_terrain_higher, seed, csize.X, csize.Y);
        noiseindev_steepness      = new NoiseIndev(params->npindev_steepness,      seed, csize.X, csize.Y);
//        noise_height_select  = new Noise(params->np_height_select,  seed, csize.X, csize.Y);
//        noise_trees          = new Noise(params->np_trees,          seed, csize.X, csize.Y);
        noiseindev_mud            = new NoiseIndev(params->npindev_mud,            seed, csize.X, csize.Y);
//        noise_beach          = new Noise(params->np_beach,          seed, csize.X, csize.Y);
//        noise_biome          = new Noise(params->np_biome,          seed, csize.X, csize.Y);
}

MapgenIndev::~MapgenIndev() {
	delete noiseindev_terrain_base;
	delete noiseindev_terrain_higher;
	delete noiseindev_steepness;
	//delete noise_height_select;
	//delete noise_trees;
	delete noiseindev_mud;
	//delete noise_beach;
	//delete noise_biome;
}


void MapgenIndev::calculateNoise() {
	int x = node_min.X;
	int y = node_min.Y;
	int z = node_min.Z;
	// Need to adjust for the original implementation's +.5 offset...
	if (!(flags & MG_FLAT)) {
		noiseindev_terrain_base->perlinMap2D(
			x + 0.5 * noiseindev_terrain_base->npindev->spread.X,
			z + 0.5 * noiseindev_terrain_base->npindev->spread.Z);
		noiseindev_terrain_base->transformNoiseMapFarScale(x, y, z);
		//noise_terrain_base->transformNoiseMap();

		noiseindev_terrain_higher->perlinMap2D(
			x + 0.5 * noiseindev_terrain_higher->npindev->spread.X,
			z + 0.5 * noiseindev_terrain_higher->npindev->spread.Z);
		noiseindev_terrain_higher->transformNoiseMapFarScale(x, y, z);
		//noise_terrain_higher->transformNoiseMap();

		noiseindev_steepness->perlinMap2D(
			x + 0.5 * noiseindev_steepness->npindev->spread.X,
			z + 0.5 * noiseindev_steepness->npindev->spread.Z);
		noiseindev_steepness->transformNoiseMapFarScale(x, y, z);

		noise_height_select->perlinMap2D(
			x + 0.5 * noise_height_select->np->spread.X,
			z + 0.5 * noise_height_select->np->spread.Z);
	}
	
	if (!(flags & MG_FLAT)) {
		noiseindev_mud->perlinMap2D(
			x + 0.5 * noiseindev_mud->npindev->spread.X,
			z + 0.5 * noiseindev_mud->npindev->spread.Z);
		noiseindev_mud->transformNoiseMapFarScale(x, y, z);
	}
	noise_beach->perlinMap2D(
		x + 0.2 * noise_beach->np->spread.X,
		z + 0.7 * noise_beach->np->spread.Z);

	noise_biome->perlinMap2D(
		x + 0.6 * noise_biome->np->spread.X,
		z + 0.2 * noise_biome->np->spread.Z);
}

bool MapgenIndevParams::readParams(Settings *settings) {
        freq_desert = settings->getFloat("mgv6_freq_desert");
        freq_beach  = settings->getFloat("mgv6_freq_beach");

        npindev_terrain_base   = settings->getNoiseIndevParams("mgindev_np_terrain_base");
        npindev_terrain_higher = settings->getNoiseIndevParams("mgindev_np_terrain_higher");
        npindev_steepness      = settings->getNoiseIndevParams("mgindev_np_steepness");
        np_height_select  = settings->getNoiseParams("mgv6_np_height_select");
        np_trees          = settings->getNoiseParams("mgv6_np_trees");
        npindev_mud            = settings->getNoiseIndevParams("mgindev_np_mud");
        np_beach          = settings->getNoiseParams("mgv6_np_beach");
        np_biome          = settings->getNoiseParams("mgv6_np_biome");
        np_cave           = settings->getNoiseParams("mgv6_np_cave");

        bool success =
                npindev_terrain_base  && npindev_terrain_higher && npindev_steepness &&
                np_height_select && np_trees          && npindev_mud       &&
                np_beach         && np_biome          && np_cave;
        return success;
}


void MapgenIndevParams::writeParams(Settings *settings) {
        settings->setFloat("mgv6_freq_desert", freq_desert);
        settings->setFloat("mgv6_freq_beach",  freq_beach);

        settings->setNoiseIndevParams("mgindev_np_terrain_base",   npindev_terrain_base);
        settings->setNoiseIndevParams("mgindev_np_terrain_higher", npindev_terrain_higher);
        settings->setNoiseIndevParams("mgindev_np_steepness",      npindev_steepness);
        settings->setNoiseParams("mgv6_np_height_select",  np_height_select);
        settings->setNoiseParams("mgv6_np_trees",          np_trees);
        settings->setNoiseIndevParams("mgindev_np_mud",            npindev_mud);
        settings->setNoiseParams("mgv6_np_beach",          np_beach);
        settings->setNoiseParams("mgv6_np_biome",          np_biome);
        settings->setNoiseParams("mgv6_np_cave",           np_cave);
}


float MapgenIndev::baseTerrainLevelFromNoise(v2s16 p) {
	if (flags & MG_FLAT)
		return water_level;
		
	float terrain_base   = NoisePerlin2DPosOffset(noiseindev_terrain_base->npindev,
							p.X, 0.5, p.Y, 0.5, seed);
	float terrain_higher = NoisePerlin2DPosOffset(noiseindev_terrain_higher->npindev,
							p.X, 0.5, p.Y, 0.5, seed);
	float steepness      = NoisePerlin2DPosOffset(noiseindev_steepness->npindev,
							p.X, 0.5, p.Y, 0.5, seed);
	float height_select  = NoisePerlin2DNoTxfmPosOffset(noise_height_select->np,
							p.X, 0.5, p.Y, 0.5, seed);

	return baseTerrainLevel(terrain_base, terrain_higher,
							steepness,    height_select);
}

float MapgenIndev::baseTerrainLevelFromMap(int index) {
	if (flags & MG_FLAT)
		return water_level;
	
	float terrain_base   = noiseindev_terrain_base->result[index];
	float terrain_higher = noiseindev_terrain_higher->result[index];
	float steepness      = noiseindev_steepness->result[index];
	float height_select  = noise_height_select->result[index];
	
	return baseTerrainLevel(terrain_base, terrain_higher,
							steepness,    height_select);
}

float MapgenIndev::getMudAmount(int index)
{
	if (flags & MG_FLAT)
		return AVERAGE_MUD_AMOUNT;
		
	/*return ((float)AVERAGE_MUD_AMOUNT + 2.0 * noise2d_perlin(
			0.5+(float)p.X/200, 0.5+(float)p.Y/200,
			seed+91013, 3, 0.55));*/
	
	return noiseindev_mud->result[index];
}

void MapgenIndev::defineCave(Cave & cave, PseudoRandom ps, v3s16 node_min, bool large_cave) {
	cave.min_tunnel_diameter = 2;
	cave.max_tunnel_diameter = ps.range(2,6);
	cave.dswitchint = ps.range(1,14);
	//cave.tunnel_routepoints = 0;
	//cave.part_max_length_rs = 0;
	cave.flooded = large_cave && ps.range(0,4);
	if(large_cave){
		cave.part_max_length_rs = ps.range(2,4);
//dstream<<"try:"<<farscale(0.2, node_min.X,node_min.Y,node_min.Z)*30<<std::endl;
		if (node_min.Y < -500 && !ps.range(0, farscale(0.2, node_min.X,node_min.Y,node_min.Z)*30)) { //huge
			cave.flooded = ps.range(0, 1);
//dstream<<"HUGE:"<<cave.flooded<<std::endl;
			cave.tunnel_routepoints = ps.range(5, 1522);
			cave.min_tunnel_diameter = 30;
			cave.max_tunnel_diameter = ps.range(30, ps.range(80,120));
		} else {
//dstream<<"large:"<<cave.flooded<<std::endl;
			cave.tunnel_routepoints = ps.range(5, ps.range(15,30));
			cave.min_tunnel_diameter = 5;
			cave.max_tunnel_diameter = ps.range(7, ps.range(8,24));
		}
	} else {
		cave.part_max_length_rs = ps.range(2,9);
		cave.tunnel_routepoints = ps.range(10, ps.range(15,30));
	}
	cave.large_cave_is_flat = (ps.range(0,1) == 0);
};
