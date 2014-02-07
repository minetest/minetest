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
#include "util/numeric.h"
#include "log.h"

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

float farscale(float scale, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 1 - (fabs(z))                     ) / (MAP_GENERATION_LIMIT * 1) ) * (scale - 1) );
}

float farscale(float scale, float x, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 2 - (fabs(x) + fabs(z))           ) / (MAP_GENERATION_LIMIT * 2) ) * (scale - 1) );
}

float farscale(float scale, float x, float y, float z) {
	return ( 1 + ( 1 - (MAP_GENERATION_LIMIT * 3 - (fabs(x) + fabs(y) + fabs(z)) ) / (MAP_GENERATION_LIMIT * 3) ) * (scale - 1) );
}

void NoiseIndev::transformNoiseMapFarScale(float xx, float yy, float zz) {
        int i = 0;
        for (int z = 0; z != sz; z++) {
                for (int y = 0; y != sy; y++) {
                        for (int x = 0; x != sx; x++) {
                                result[i] = result[i] * npindev->scale * farscale(npindev->farscale, xx, yy, zz) + npindev->offset;
                                i++;
                        }
                }
        }
}

MapgenIndev::MapgenIndev(int mapgenid, MapgenParams *params, EmergeManager *emerge) 
	: MapgenV6(mapgenid, params, emerge)
{
	MapgenIndevParams *sp = (MapgenIndevParams *)params->sparams;

	float_islands = sp->float_islands;

	noiseindev_terrain_base    = new NoiseIndev(&sp->npindev_terrain_base,   seed, csize.X, csize.Z);
	noiseindev_terrain_higher  = new NoiseIndev(&sp->npindev_terrain_higher, seed, csize.X, csize.Z);
	noiseindev_steepness       = new NoiseIndev(&sp->npindev_steepness,      seed, csize.X, csize.Z);
	noiseindev_mud             = new NoiseIndev(&sp->npindev_mud,            seed, csize.X, csize.Z);
	noiseindev_float_islands1  = new NoiseIndev(&sp->npindev_float_islands1, seed, csize.X, csize.Y, csize.Z);
	noiseindev_float_islands2  = new NoiseIndev(&sp->npindev_float_islands2, seed, csize.X, csize.Y, csize.Z);
	noiseindev_float_islands3  = new NoiseIndev(&sp->npindev_float_islands3, seed, csize.X, csize.Z);
	noiseindev_biome           = new NoiseIndev(&sp->npindev_biome,          seed, csize.X, csize.Z);
}

MapgenIndev::~MapgenIndev() {
	delete noiseindev_terrain_base;
	delete noiseindev_terrain_higher;
	delete noiseindev_steepness;
	delete noiseindev_mud;
	delete noiseindev_float_islands1;
	delete noiseindev_float_islands2;
	delete noiseindev_float_islands3;
	delete noiseindev_biome;
}

void MapgenIndev::calculateNoise() {
	int x = node_min.X;
	int y = node_min.Y;
	int z = node_min.Z;
	// Need to adjust for the original implementation's +.5 offset...
	if (!(flags & MG_FLAT)) {
		noiseindev_terrain_base->perlinMap2D(
			x + 0.5 * noiseindev_terrain_base->npindev->spread.X * farscale(noiseindev_terrain_base->npindev->farspread, x, z),
			z + 0.5 * noiseindev_terrain_base->npindev->spread.Z * farscale(noiseindev_terrain_base->npindev->farspread, x, z));
		noiseindev_terrain_base->transformNoiseMapFarScale(x, y, z);

		noiseindev_terrain_higher->perlinMap2D(
			x + 0.5 * noiseindev_terrain_higher->npindev->spread.X * farscale(noiseindev_terrain_higher->npindev->farspread, x, z),
			z + 0.5 * noiseindev_terrain_higher->npindev->spread.Z * farscale(noiseindev_terrain_higher->npindev->farspread, x, z));
		noiseindev_terrain_higher->transformNoiseMapFarScale(x, y, z);

		noiseindev_steepness->perlinMap2D(
			x + 0.5 * noiseindev_steepness->npindev->spread.X * farscale(noiseindev_steepness->npindev->farspread, x, z),
			z + 0.5 * noiseindev_steepness->npindev->spread.Z * farscale(noiseindev_steepness->npindev->farspread, x, z));
		noiseindev_steepness->transformNoiseMapFarScale(x, y, z);

		noise_height_select->perlinMap2D(
			x + 0.5 * noise_height_select->np->spread.X,
			z + 0.5 * noise_height_select->np->spread.Z);

		noiseindev_float_islands1->perlinMap3D(
			x + 0.33 * noiseindev_float_islands1->npindev->spread.X * farscale(noiseindev_float_islands1->npindev->farspread, x, y, z),
			y + 0.33 * noiseindev_float_islands1->npindev->spread.Y * farscale(noiseindev_float_islands1->npindev->farspread, x, y, z),
			z + 0.33 * noiseindev_float_islands1->npindev->spread.Z * farscale(noiseindev_float_islands1->npindev->farspread, x, y, z)
		);
		noiseindev_float_islands1->transformNoiseMapFarScale(x, y, z);

		noiseindev_float_islands2->perlinMap3D(
			x + 0.33 * noiseindev_float_islands2->npindev->spread.X * farscale(noiseindev_float_islands2->npindev->farspread, x, y, z),
			y + 0.33 * noiseindev_float_islands2->npindev->spread.Y * farscale(noiseindev_float_islands2->npindev->farspread, x, y, z),
			z + 0.33 * noiseindev_float_islands2->npindev->spread.Z * farscale(noiseindev_float_islands2->npindev->farspread, x, y, z)
		);
		noiseindev_float_islands2->transformNoiseMapFarScale(x, y, z);

		noiseindev_float_islands3->perlinMap2D(
			x + 0.5 * noiseindev_float_islands3->npindev->spread.X * farscale(noiseindev_float_islands3->npindev->farspread, x, z),
			z + 0.5 * noiseindev_float_islands3->npindev->spread.Z * farscale(noiseindev_float_islands3->npindev->farspread, x, z));
		noiseindev_float_islands3->transformNoiseMapFarScale(x, y, z);

		noiseindev_mud->perlinMap2D(
			x + 0.5 * noiseindev_mud->npindev->spread.X * farscale(noiseindev_mud->npindev->farspread, x, y, z),
			z + 0.5 * noiseindev_mud->npindev->spread.Z * farscale(noiseindev_mud->npindev->farspread, x, y, z));
		noiseindev_mud->transformNoiseMapFarScale(x, y, z);
	}
	noise_beach->perlinMap2D(
		x + 0.2 * noise_beach->np->spread.X,
		z + 0.7 * noise_beach->np->spread.Z);

	noise_biome->perlinMap2D(
		x + 0.6 * noiseindev_biome->npindev->spread.X * farscale(noiseindev_biome->npindev->farspread, x, z),
		z + 0.2 * noiseindev_biome->npindev->spread.Z * farscale(noiseindev_biome->npindev->farspread, x, z));
}

MapgenIndevParams::MapgenIndevParams() {
	float_islands = 500;
	npindev_terrain_base    = NoiseIndevParams(-4,   20,  v3f(250, 250, 250), 82341, 5, 0.6,  10,  10);
	npindev_terrain_higher  = NoiseIndevParams(20,   16,  v3f(500, 500, 500), 85039, 5, 0.6,  10,  10);
	npindev_steepness       = NoiseIndevParams(0.85, 0.5, v3f(125, 125, 125), -932,  5, 0.7,  2,   10);
	npindev_mud             = NoiseIndevParams(4,    2,   v3f(200, 200, 200), 91013, 3, 0.55, 1,   1);
	npindev_biome           = NoiseIndevParams(0,    1,   v3f(250, 250, 250), 9130,  3, 0.50, 1,   10);
	npindev_float_islands1  = NoiseIndevParams(0,    1,   v3f(256, 256, 256), 3683,  6, 0.6,  1,   1.5);
	npindev_float_islands2  = NoiseIndevParams(0,    1,   v3f(8,   8,   8  ), 9292,  2, 0.5,  1,   1.5);
	npindev_float_islands3  = NoiseIndevParams(0,    1,   v3f(256, 256, 256), 6412,  2, 0.5,  1,   0.5);
}

void MapgenIndevParams::readParams(Settings *settings) {
	MapgenV6Params::readParams(settings);

	settings->getS16NoEx("mgindev_float_islands", float_islands);

	settings->getNoiseIndevParams("mgindev_np_terrain_base",   npindev_terrain_base);
	settings->getNoiseIndevParams("mgindev_np_terrain_higher", npindev_terrain_higher);
	settings->getNoiseIndevParams("mgindev_np_steepness",      npindev_steepness);
	settings->getNoiseIndevParams("mgindev_np_mud",            npindev_mud);
	settings->getNoiseIndevParams("mgindev_np_biome",          npindev_biome);
	settings->getNoiseIndevParams("mgindev_np_float_islands1", npindev_float_islands1);
	settings->getNoiseIndevParams("mgindev_np_float_islands2", npindev_float_islands2);
	settings->getNoiseIndevParams("mgindev_np_float_islands3", npindev_float_islands3);
}

void MapgenIndevParams::writeParams(Settings *settings) {
	MapgenV6Params::writeParams(settings);

	settings->setS16("mgindev_float_islands", float_islands);

	settings->setNoiseIndevParams("mgindev_np_terrain_base",   npindev_terrain_base);
	settings->setNoiseIndevParams("mgindev_np_terrain_higher", npindev_terrain_higher);
	settings->setNoiseIndevParams("mgindev_np_steepness",      npindev_steepness);
	settings->setNoiseIndevParams("mgindev_np_mud",            npindev_mud);
	settings->setNoiseIndevParams("mgindev_np_biome",          npindev_biome);
	settings->setNoiseIndevParams("mgindev_np_float_islands1", npindev_float_islands1);
	settings->setNoiseIndevParams("mgindev_np_float_islands2", npindev_float_islands2);
	settings->setNoiseIndevParams("mgindev_np_float_islands3", npindev_float_islands3);
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

float MapgenIndev::getMudAmount(int index) {
	if (flags & MG_FLAT)
		return AVERAGE_MUD_AMOUNT;
		
	/*return ((float)AVERAGE_MUD_AMOUNT + 2.0 * noise2d_perlin(
			0.5+(float)p.X/200, 0.5+(float)p.Y/200,
			seed+91013, 3, 0.55));*/
	
	return noiseindev_mud->result[index];
}

void MapgenIndev::generateCaves(int max_stone_y) {
	float cave_amount = NoisePerlin2D(np_cave, node_min.X, node_min.Y, seed);
	int volume_nodes = (node_max.X - node_min.X + 1) *
					   (node_max.Y - node_min.Y + 1) * MAP_BLOCKSIZE;
	cave_amount = MYMAX(0.0, cave_amount);
	u32 caves_count = cave_amount * volume_nodes / 50000;
	u32 bruises_count = 1;
	PseudoRandom ps(blockseed + 21343);
	PseudoRandom ps2(blockseed + 1032);
	
	if (ps.range(1, 6) == 1)
		bruises_count = ps.range(0, ps.range(0, 2));
	
	if (getBiome(v2s16(node_min.X, node_min.Z)) == BT_DESERT) {
		caves_count   /= 3;
		bruises_count /= 3;
	}
	
	for (u32 i = 0; i < caves_count + bruises_count; i++) {
		bool large_cave = (i >= caves_count);
		CaveIndev cave(this, &ps, &ps2, node_min, large_cave);

		cave.makeCave(node_min, node_max, max_stone_y);
	}
}

CaveIndev::CaveIndev(MapgenIndev *mg, PseudoRandom *ps, PseudoRandom *ps2,
				v3s16 node_min, bool is_large_cave) {
	this->mg = mg;
	this->vm = mg->vm;
	this->ndef = mg->ndef;
	this->water_level = mg->water_level;
	this->large_cave = is_large_cave;
	this->ps  = ps;
	this->ps2 = ps2;
	this->c_water_source = mg->c_water_source;
	this->c_lava_source  = mg->c_lava_source;

	min_tunnel_diameter = 2;
	max_tunnel_diameter = ps->range(2,6);
	dswitchint = ps->range(1,14);
	flooded = large_cave && ps->range(0,4);
	if (large_cave) {
		part_max_length_rs = ps->range(2,4);
		float scale = farscale(0.2, node_min.X, node_min.Y, node_min.Z);
		if (node_min.Y < -100 && !ps->range(0, scale * 30)) { //huge
			flooded = !ps->range(0, 3);
			tunnel_routepoints = ps->range(5, 30);
			min_tunnel_diameter = 30;
			max_tunnel_diameter = ps->range(40, ps->range(80, 150));
		} else {
			tunnel_routepoints = ps->range(5, ps->range(15,30));
			min_tunnel_diameter = 5;
			max_tunnel_diameter = ps->range(7, ps->range(8,24));
		}
	} else {
		part_max_length_rs = ps->range(2,9);
		tunnel_routepoints = ps->range(10, ps->range(15,30));
	}
	large_cave_is_flat = (ps->range(0,1) == 0);
}

/*
// version with one perlin3d. use with good params like
settings->setDefault("mgindev_np_float_islands1",  "-9.5, 10,  (20,  50,  50 ), 45123, 5, 0.6,  1.5, 5");
void MapgenIndev::generateFloatIslands(int min_y) {
	if (node_min.Y < min_y) return;
	v3s16 p0(node_min.X, node_min.Y, node_min.Z);
	MapNode n1(c_stone), n2(c_desert_stone);
	int xl = node_max.X - node_min.X;
	int yl = node_max.Y - node_min.Y;
	int zl = node_max.Z - node_min.Z;
	u32 index = 0;
	for (int x1 = 0; x1 <= xl; x1++)
	{
		//int x = x1 + node_min.Y;
		for (int z1 = 0; z1 <= zl; z1++)
		{
			//int z = z1 + node_min.Z;
			for (int y1 = 0; y1 <= yl; y1++, index++)
			{
				//int y = y1 + node_min.Y;
				float noise = noiseindev_float_islands1->result[index];
				//dstream << " y1="<<y1<< " x1="<<x1<<" z1="<<z1<< " noise="<<noise << std::endl;
				if (noise > 0 ) {
					v3s16 p = p0 + v3s16(x1, y1, z1);
					u32 i = vm->m_area.index(p);
					if (!vm->m_area.contains(i))
						continue;
					// Cancel if not  air
					if (vm->m_data[i].getContent() != CONTENT_AIR)
						continue;
					vm->m_data[i] = noise > 1 ? n1 : n2;
				}
			}
		}
	}
}
*/

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
	u32 zstride = xl + 1;
	float midy = node_min.Y + yl * 0.5;
	u32 index = 0;
	for (int z1 = 0; z1 <= zl; ++z1)
	for (int y1 = 0; y1 <= yl; ++y1)
	for (int x1 = 0; x1 <= xl; ++x1, ++index) {
		int y = y1 + node_min.Y;
		u32 index2d = z1 * zstride + x1;
		float noise3 = noiseindev_float_islands3->result[index2d];
		float pmidy = midy + noise3 / 1.5 * AMPY;
		float noise1 = noiseindev_float_islands1->result[index];
		float offset = y > pmidy ? (y - pmidy) / TGRAD : (pmidy - y) / BGRAD;
		float noise1off = noise1 - offset - RAR;
		if (noise1off > 0 && noise1off < 0.7) {
			float noise2 = noiseindev_float_islands2->result[index];
			if (noise2 - noise1off > -0.7) {
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

void MapgenIndev::generateExperimental() {
	if (float_islands)
		generateFloatIslands(float_islands);
}
