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
#include "map.h"
#include "main.h"
#include "log.h"

/////////////////// Mapgen Indev perlin noise (default values - not used, from config or defaultsettings)

NoiseIndevParams nparams_indev_def;

///////////////////////////////////////////////////////////////////////////////

NoiseIndev::NoiseIndev(NoiseIndevParams *np,
                       float xShift, float yShift, float zShift)
{
	m_baseNoise = createDefaultBaseNoise();
	m_noise = new ParameterizedNoise(m_baseNoise, *np);
	m_xShift = xShift;
	m_yShift = yShift;
	m_zShift = zShift;
	m_yPlane = 0.0f;

	this->npindev = np;
}

inline static float farscale(float scale, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 1 - (fabs(z))                     ) / (MAP_GENERATION_LIMIT * 1) ) * (scale - 1) );
}

inline static float farscale(float scale, float x, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 2 - (fabs(x) + fabs(z))           ) / (MAP_GENERATION_LIMIT * 2) ) * (scale - 1) );
}

inline static float farscale(float scale, float x, float y, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 3 - (fabs(x) + fabs(y) + fabs(z)) ) / (MAP_GENERATION_LIMIT * 3) ) * (scale - 1) );
}

inline static float farScaleTransform(float value,
                                      float scale,
                                      float farScale,
                                      float offset,
                                      float xx, float yy, float zz) {
	return value*scale*farscale(farScale, xx, yy, zz) + offset;
}

float NoiseIndev::noise(int seed, float x, float y) {
	return farScaleTransform(
	          m_noise->noise(seed, x+m_xShift, y+m_yShift),
	          npindev->scale,
	          npindev->farscale,
	          npindev->offset,
	          x, m_yPlane, y);
}

float NoiseIndev::noise(int seed, float x, float y, float z) {
	return farScaleTransform(
	          m_noise->noise(seed, x+m_xShift, y+m_yShift, z+m_zShift),
	          npindev->scale,
	          npindev->farscale,
	          npindev->offset,
	          x, y, z);
}

void NoiseIndev::noiseBlock(int seed,
                            int sx, float x0, float dx,
                            int sy, float y0, float dy,
                            float* results) {
	float scale = farscale(npindev->farspread, x0, m_yPlane);
	float x = x0 + m_xShift*npindev->spread.X*scale;
	float y = y0 + m_yShift*npindev->spread.Z*scale;

	m_noise->noiseBlock(seed,
	                    sx, x, dx,
	                    sy, y, dy,
	                    results);

	int n = sx*sy;
	for (int i = 0; i < n; ++i) {
		results[i] = farScaleTransform(results[i],
		                               npindev->scale,
		                               npindev->farscale,
		                               npindev->offset,
		                               x, m_yPlane, y);
	}
}

void NoiseIndev::noiseBlock(int seed,
                            int sx, float x0, float dx,
                            int sy, float y0, float dy,
                            int sz, float z0, float dz,
                            float* results) {
	float scale = farscale(npindev->farspread, x0, y0, z0);
	float x = x0 + m_xShift*npindev->spread.X*scale;
	float y = y0 + m_yShift*npindev->spread.Y*scale;
	float z = z0 + m_zShift*npindev->spread.Z*scale;

	m_noise->noiseBlock(seed,
	                    sx, x, dx,
	                    sy, y, dy,
	                    sz, z, dz,
	                    results);

	int n = sx*sy*sz;
	for (int i = 0; i < n; ++i) {
		results[i] = farScaleTransform(results[i],
		                               npindev->scale,
		                               npindev->farscale,
		                               npindev->offset,
		                               x, y, z);
	}
}

MapgenIndev::MapgenIndev(int mapgenid, MapgenIndevParams *params, EmergeManager *emerge) 
	: MapgenV6(mapgenid, params, emerge)
{
	noiseindev_terrain_base = new NoiseIndev(params->npindev_terrain_base, 0.5f, 0.5f);
	noiseindev_terrain_higher = new NoiseIndev(params->npindev_terrain_higher, 0.5f, 0.5f);
	noiseindev_steepness = new NoiseIndev(params->npindev_steepness, 0.5f, 0.5f);
	noiseindev_mud = new NoiseIndev(params->npindev_mud, 0.5f, 0.5f);
	noiseindev_float_islands1 = new NoiseIndev(params->npindev_float_islands1, 0.33f, 0.33f, 0.33f);
	noiseindev_float_islands2 = new NoiseIndev(params->npindev_float_islands2, 0.33f, 0.33f, 0.33f);
	noiseindev_float_islands3  = new NoiseIndev(params->npindev_float_islands3, 0.5f, 0.5f);

	int size2d = csize.X*csize.Z;
	int size3d = csize.X*csize.Y*csize.Z;
	noiseindev_results_terrain_base = new float[size2d];
	noiseindev_results_terrain_higher = new float[size2d];
	noiseindev_results_steepness      = new float[size2d];
	noiseindev_results_mud            = new float[size2d];
	noiseindev_results_float_islands1  = new float[size3d];
	noiseindev_results_float_islands2  = new float[size3d];
	noiseindev_results_float_islands3  = new float[size2d];
	noiseindev_results_biome          = new float[size2d];
}

MapgenIndev::~MapgenIndev() {
	delete[] noiseindev_results_terrain_base;
	delete[] noiseindev_results_terrain_higher;
	delete[] noiseindev_results_steepness;
	delete[] noiseindev_results_mud;
	delete[] noiseindev_results_float_islands1;
	delete[] noiseindev_results_float_islands2;
	delete[] noiseindev_results_float_islands3;
	delete noiseindev_terrain_base;
	delete noiseindev_terrain_higher;
	delete noiseindev_steepness;
	delete noiseindev_mud;
	delete noiseindev_float_islands1;
	delete noiseindev_float_islands2;
	delete noiseindev_float_islands3;
}

void MapgenIndev::calculateNoise() {
	int x = node_min.X;
	int y = node_min.Y;
	int z = node_min.Z;
	// Need to adjust for the original implementation's +.5 offset...
	if (!(flags & MG_FLAT)) {
		noiseindev_terrain_base->setYPlane(y);
		noiseindev_terrain_base->noiseBlock(seed,
		                                    csize.X, x, 1.0f,
		                                    csize.Z, z, 1.0f,
		                                    noiseindev_results_terrain_base);

		noiseindev_terrain_higher->setYPlane(y);
		noiseindev_terrain_higher->noiseBlock(seed,
		                                      csize.X, x, 1.0f,
		                                      csize.Z, z, 1.0f,
		                                      noiseindev_results_terrain_higher);

		noiseindev_steepness->setYPlane(y);
		noiseindev_steepness->noiseBlock(seed,
		                                 csize.X, x, 1.0f,
		                                 csize.Z, z, 1.0f,
		                                 noiseindev_results_steepness);

		noiseindev_float_islands1->noiseBlock(seed,
		                                      csize.X, x, 1.0f,
		                                      csize.Y, y, 1.0f,
		                                      csize.Z, z, 1.0f,
		                                      noiseindev_results_float_islands1);

		noiseindev_float_islands2->noiseBlock(seed,
		                                      csize.X, x, 1.0f,
		                                      csize.Y, y, 1.0f,
		                                      csize.Z, z, 1.0f,
		                                      noiseindev_results_float_islands2);

		noiseindev_float_islands2->setYPlane(y);
		noiseindev_float_islands2->noiseBlock(seed,
		                                      csize.X, x, 1.0f,
		                                      csize.Z, z, 1.0f,
		                                      noiseindev_results_float_islands2);

	}
	
	if (!(flags & MG_FLAT)) {
		noiseindev_mud->setYPlane(y);
		noiseindev_mud->noiseBlock(seed,
		                           csize.X, x, 1.0f,
		                           csize.Z, z, 1.0f,
		                           noiseindev_results_mud);
	}
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
        npindev_biome     = settings->getNoiseIndevParams("mgindev_np_biome");
        np_cave           = settings->getNoiseParams("mgv6_np_cave");
        npindev_float_islands1  = settings->getNoiseIndevParams("mgindev_np_float_islands1");
        npindev_float_islands2  = settings->getNoiseIndevParams("mgindev_np_float_islands2");
        npindev_float_islands3  = settings->getNoiseIndevParams("mgindev_np_float_islands3");

        bool success =
                npindev_terrain_base  && npindev_terrain_higher && npindev_steepness &&
                np_height_select && np_trees          && npindev_mud       &&
                np_beach         && np_biome          && np_cave &&
                npindev_float_islands1 && npindev_float_islands2 && npindev_float_islands3;
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
        settings->setNoiseIndevParams("mgindev_np_biome",          npindev_biome);
        settings->setNoiseParams("mgv6_np_cave",           np_cave);
        settings->setNoiseIndevParams("mgindev_np_float_islands1",  npindev_float_islands1);
        settings->setNoiseIndevParams("mgindev_np_float_islands2",  npindev_float_islands2);
        settings->setNoiseIndevParams("mgindev_np_float_islands3",  npindev_float_islands3);
}


float MapgenIndev::baseTerrainLevelFromNoise(v2s16 p) {
	if (flags & MG_FLAT)
		return water_level;

	float terrain_base = noiseindev_terrain_base->noise(seed, p.X, p.Y);
	float terrain_higher = noiseindev_terrain_higher->noise(seed, p.X, p.Y);
	float steepness = noiseindev_steepness->noise(seed, p.X, p.Y);

	return baseTerrainLevel(terrain_base, terrain_higher,
							steepness,    0.0f);
}

float MapgenIndev::baseTerrainLevelFromMap(int index) {
	if (flags & MG_FLAT)
		return water_level;
	
	float terrain_base = noiseindev_results_terrain_base[index];
	float terrain_higher = noiseindev_results_terrain_higher[index];
	float steepness      = noiseindev_results_steepness[index];

	return baseTerrainLevel(terrain_base, terrain_higher,
							steepness,    0.0f);
}

float MapgenIndev::getMudAmount(int index) {
	if (flags & MG_FLAT)
		return AVERAGE_MUD_AMOUNT;

	return noiseindev_results_mud[index];
}

void MapgenIndev::defineCave(Cave & cave, PseudoRandom ps, v3s16 node_min, bool large_cave) {
	cave.min_tunnel_diameter = 2;
	cave.max_tunnel_diameter = ps.range(2,6);
	cave.dswitchint = ps.range(1,14);
	cave.flooded = large_cave && ps.range(0,4);
	if(large_cave){
		cave.part_max_length_rs = ps.range(2,4);
		if (node_min.Y < -100 && !ps.range(0, farscale(0.2, node_min.X,node_min.Y,node_min.Z)*30)) { //huge
			cave.flooded = !ps.range(0, 3);
			cave.tunnel_routepoints = ps.range(5, 20);
			cave.min_tunnel_diameter = 30;
			cave.max_tunnel_diameter = ps.range(40, ps.range(80,120));
		} else {
			cave.tunnel_routepoints = ps.range(5, ps.range(15,30));
			cave.min_tunnel_diameter = 5;
			cave.max_tunnel_diameter = ps.range(7, ps.range(8,24));
		}
	} else {
		cave.part_max_length_rs = ps.range(2,9);
		cave.tunnel_routepoints = ps.range(10, ps.range(15,30));
	}
	cave.large_cave_is_flat = (ps.range(0,1) == 0);
}

void MapgenIndev::generateFloatIslands(int min_y) {
	if (node_min.Y < min_y) return;
	PseudoRandom pr(blockseed + 985);
	// originally from http://forum.minetest.net/viewtopic.php?id=4776
	float RAR = 0.8 * farscale(0.4, node_min.Y); // 0.4; // Island rarity in chunk layer. -0.4 = thick layer with holes, 0 = 50%, 0.4 = desert rarity, 0.7 = very rare.
	float AMPY = 24; // 24; // Amplitude of island centre y variation.
	float TGRAD = 24; // 24; // Noise gradient to create top surface. Tallness of island top.
	float BGRAD = 24; // 24; // Noise gradient to create bottom surface. Tallness of island bottom.

	v3s16 p0(node_min.X, node_min.Y, node_min.Z);
	MapNode n1(c_stone);

	float xl = node_max.X - node_min.X;
	float yl = node_max.Y - node_min.Y;
	float zl = node_max.Z - node_min.Z;
	float midy = node_min.Y + yl * 0.5;
	u32 index = 0, index2d = 0;
	for (int x1 = 0; x1 <= xl; ++x1)
	{
		for (int z1 = 0; z1 <= zl; ++z1, ++index2d)
		{
			float noise3 = noiseindev_results_float_islands3[index2d];
			float pmidy = midy + noise3 / 1.5 * AMPY;
			for (int y1 = 0; y1 <= yl; ++y1, ++index)
			{
				int y = y1 + node_min.Y;
				float noise1 = noiseindev_results_float_islands1[index];
				float offset = y > pmidy ? (y - pmidy) / TGRAD : (pmidy - y) / BGRAD;
				float noise1off = noise1 - offset - RAR;
				if (noise1off > 0 && noise1off < 0.7) {
					float noise2 = noiseindev_results_float_islands2[index];
					if (noise2 - noise1off > -0.7){
						v3s16 p = p0 + v3s16(x1, y1, z1);
						u32 i = vm->m_area.index(p);
						if (!vm->m_area.contains(i))
							continue;
						// Cancel if not  air
						if (vm->m_data[i].getContent() != CONTENT_AIR)
							continue;
						vm->m_data[i] = n1;
					}
				}
			}
		}
	}
}

void MapgenIndev::generateSomething() {
	int float_islands = g_settings->getS16("mgindev_float_islands");
	if(float_islands) generateFloatIslands(float_islands);
}
