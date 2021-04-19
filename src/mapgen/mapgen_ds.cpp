/*
Minetest
Copyright (C) 2013-2018 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2018 paramat

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

#include "mapgen_ds.h"
#include "voxel.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "emerge.h"
#include <cmath>
#include <cstdlib>
#include "mg_biome.h"

MapgenDSParams::MapgenDSParams():np_filler_depth (0,   1.2, v3f(150, 150, 150), 261,   3, 0.7,  2.0)
{}

MapgenDSParams::~MapgenDSParams()
{}

//////////////////////// Map generator

double range_random(double a, double b)
{
	return a + (((double)rand())/RAND_MAX)*(b-a);
}

DiamondSquareMountain::DiamondSquareMountain(int n, double left_top, double right_top, double left_bottom, double right_bottom, double center)
{
	this->n = n;
	int half = pow(2, n-1);
	size = pow(2, n);
	height_map = new double*[size+1];
	for (int i = 0; i < size+1; i++)
	{
		height_map[i] = new double[size+1];
		for (int j = 0; j < size+1; j++)
			height_map[i][j] = -1;
	}

	// Init diamond square
	height_map[0][0] = left_top;
	height_map[0][size] = right_top;
	height_map[size][0] = left_bottom;
	height_map[size][size] = right_bottom;
	height_map[half][half] = center;

	minimal = std::min(std::min(left_top, right_top), std::min(left_bottom, right_bottom));

	big_rnd = 2;
	small_rnd = 4;
	small_rnd_size = 8;
	process();
}

double DiamondSquareMountain::get_value(int z, int x)
{
	if (x < 0 || z < 0 || x > size || z > size)
	{
                return 0;
        }

        return height_map[z][x];
}

void DiamondSquareMountain::set_value(int z, int x, double h)
{
	if (x < 0 || z < 0 || x > size || z > size)
	{
                return;
        }
        height_map[z][x] = std::max(h, minimal);
}

void DiamondSquareMountain::process()
{
	for (int i = n; i >= 1; i--)
	{
		double random_div;
		int step = pow(2, i);
		int step2 = step/2;
		
		int num = pow(2, n-i);

		if (step >= small_rnd_size)
			random_div = big_rnd;
		else
			random_div = small_rnd;

		// Diamond step
		if (i < n)
		{
			int z = 0;
                        for (int p = 1; p <= num; p++)
			{
                                int x = 0;
                                for (int q = 1; q <= num; q++)
				{
                                        double val1 = get_value(z, x);
                                        double val2 = get_value(z+step, x);
                                        double val3 = get_value(z, x+step);
                                        double val4 = get_value(z+step, x+step);
                                        double rv = range_random(-step2/random_div, step2/random_div);

                                        double val = (val1 + val2 + val3 + val4)/4 + rv;
                                        set_value(z+step2, x+step2, val);
                                        x += step;
				}
                                z += step;
                        }
		}

		// Square step
		{
			int z = 0;
        	        for (int p = 1; p <= num+1; p++)
			{
	                        int x = 0;
	                        for (int q = 1; q <= num+1; q++)
				{
					double val1, val2, val3, val4;
					double val, rv;

					val1 = get_value(z-step2, x+step2);
					val2 = get_value(z+step2, x+step2);
					val3 = get_value(z, x);
					val4 = get_value(z, x+step);
					rv = range_random(-step2/random_div, step2/random_div);

					val = (val1 + val2 + val3 + val4)/4 + rv;
					set_value(z, x+step2, val);

					val1 = get_value(z+step2, x-step2);
					val2 = get_value(z+step2, x+step2);
					val3 = get_value(z, x);
					val4 = get_value(z+step, x);
					rv = range_random(-step2/random_div, step2/random_div);

					val = (val1 + val2 + val3 + val4)/4 + rv;
					set_value(z+step2, x, val);

					x += step;
				}
                        	z += step;
                        }
                }
	}

	int half = size/2;
	double val1 = get_value(half-1, half);
	double val2 = get_value(half+1, half);
        double val3 = get_value(half, half-1);
        double val4 = get_value(half, half+1);

	double val = (val1 + val2 + val3 + val4)/4;
	set_value(half, half, val); 
/*
	for (int i = 0; i <= size; i++)
	{
		for (int j = 0; j <= size; j++)
			printf("%lf ", height_map[i][j]);
		printf("\n");
	}
*/
}

DiamondSquareMountain::~DiamondSquareMountain()
{
	for (int i = 0; i < size; i++)
		delete[] height_map[i];
	delete height_map;
}

////////////////////////////////////////////////////

MapgenDS::MapgenDS(MapgenDSParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_DIAMOND_SQUARE, params, emerge)
{
	const NodeDefManager *ndef = emerge->ndef;

	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	c_node = CONTENT_AIR;

	MapNode n_node(c_node);
	set_light = (ndef->get(n_node).sunlight_propagates) ? LIGHT_SUN : 0x00;

	m = new DiamondSquareMountain(9, 0, 0, 0, 0, 100);

	offset_x = 0;
	offset_z = 0;
}

void MapgenDS::makeChunk(BlockMakeData *data)
{
	// Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;

	// Area of central chunk
	v3s16 node_min = blockpos_min * MAP_BLOCKSIZE;
	v3s16 node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(node_min, data->seed);

	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y; y <= node_max.Y; y++)
	{
		u32 i = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++)
		{
			int mx = x - offset_x + m->size/2;
			int mz = z - offset_z + m->size/2;
			double height = m->get_value(mz, mx);
			if (vm->m_data[i].getContent() == CONTENT_IGNORE)
			{
				if (y > height)
					vm->m_data[i] = n_air;
				else
					vm->m_data[i] = n_stone;
			}
			i++;
		}
	}

	// update height map
	updateHeightmap(node_min, node_max);

	// create biomes
	//biomegen->calcBiomeNoise(node_min);
	//generateBiomes();

	// Add top and bottom side of water to transforming_liquid queue
	updateLiquid(&data->transforming_liquid, node_min, node_max);

	// Set lighting
	if ((flags & MG_LIGHT) && set_light == LIGHT_SUN)
		setLighting(LIGHT_SUN, node_min, node_max);

	this->generating = false;
}


int MapgenDS::getSpawnLevelAtPoint(v2s16 p)
{
	return 0;
}

MapgenDS::~MapgenDS()
{
	delete m;
	delete noise_filler_depth;
}

