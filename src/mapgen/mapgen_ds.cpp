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
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mg_biome.h"

#include <cmath>
#include <cstdlib>


MapgenDSParams::MapgenDSParams():
    np_filler_depth (0,   1.2, v3f(150, 150, 150), 261,   3, 0.7,  2.0)
{}

MapgenDSParams::~MapgenDSParams()
{}

//////////////////////// Map generator


////////////////////////////////////////////////////


void MapgenDS::AddMountainRange(double height, int z1, int x1, int z2, int x2)
{
    double len = sqrt((x2-x1)*(x2-x1)+(z2-z1)*(z2-z1));
    int n = 2 * len / height + 1;

    std::vector<std::pair<int, int>> poss;
    for (int i = 0; i < n; i++)
    {
        int x = x1 + (x2-x1)*i/(n-1) + prandom.range(-len/n/4, len/n/4);
        int z = z1 + (z2-z1)*i/(n-1) + prandom.range(-len/n/4, len/n/4);
        int angle = prandom.range(45, 65);
        auto h = height*prandom.range(50,100)/100.0;
        if (angle > 55)
            landscape->AddMountain(angle, z, x, h, 3.5, -20);
        else
            landscape->AddCuttedMountain(angle, 65, z, x, h, 0.85, 3.5, -25);
    }
}


void MapgenDS::AddVolcanicIsland(double height, int z, int x)
{
    // central mountain
    landscape->AddCuttedMountain(40, 70, z, x, height, 0.85, 3.5, -25);
    landscape->AddMountain(75, z, x, height/5, 3.5, -20);

    // several less mountains
    int n = prandom.range(0, 3);
    for (int i = 0; i < n; i++)
    {
        // mountain
        double h = prandom.range(33, 50) * height / 100.0;
        double r = prandom.range(50, 150) * height / 100.0;
        double angle = prandom.range(0, 360)*3.14159/180;
        double sx = r*cos(angle);
        double sz = r*sin(angle);
        landscape->AddCuttedMountain(60, 70, z+sz, x+sx, h, 0.95, 3, -20);
        landscape->AddMountain(75, z+sz, x+sx, prandom.range(20, 40), 3.5, -15);
    }

    // second peak
    n = prandom.range(0, 1);
    for (int i = 0; i < n; i++)
    {
        // mountain
        double h = prandom.range(20, 66)*height/100.0;
        double r = prandom.range(100, 250)*height/100.0;
        double angle = prandom.range(0, 360)*3.14159/180;
        double sx = r*cos(angle);
        double sz = r*sin(angle);
        landscape->AddCuttedMountain(60, 70, z+sz, x+sx, h, 0.95, 3, -20);
        landscape->AddMountain(75, z+sz, x+sx, prandom.range(20, 40), 3.5, -15);
    }
}

MapgenDS::MapgenDS(MapgenDSParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_DIAMOND_SQUARE, params, emerge)
{
	const NodeDefManager *ndef = emerge->ndef;
    prandom.seed(seed);

	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);
    landscape = new MountainLandscape(&prandom);    

	c_node = CONTENT_AIR;

	MapNode n_node(c_node);
	set_light = (ndef->get(n_node).sunlight_propagates) ? LIGHT_SUN : 0x00;

    //AddVolcanicIsland(150, 0, 0);
    AddMountainRange(150, -300, -300, 300, 300);
    AddMountainRange(250, -400+300, -400, 400+300, 400);

	offset_x = 0;
	offset_z = 0;
}

s16 MapgenDS::generateBaseTerrain()
{
    u32 index = 0;
	u32 index2d = 0;
	int stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;

	for (s16 z=node_min.Z; z<=node_max.Z; z++) {
		for (s16 y=node_min.Y - 1; y<=node_max.Y + 1; y++) {
			u32 vi = vm->m_area.index(node_min.X, y, z);
			for (s16 x=node_min.X; x<=node_max.X; x++, vi++, index++, index2d++)
            {
				if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
					continue;

                double mx = x - offset_x;
			    double mz = z - offset_z;
			    double height = landscape->Height(mz, mx);

                if (y > height)
                {
                    if (y > 5)
                    {
					    vm->m_data[vi] = MapNode(CONTENT_AIR);
                    }
                    else
                    {
                        vm->m_data[vi] = MapNode(c_water_source);
                    }
                    
                }
				else
                {
					vm->m_data[vi] = MapNode(c_stone);
                    stone_surface_max_y = std::max((int)stone_surface_max_y, (int)y);
                }
			}
			index2d -= ystride;
		}
		index2d += ystride;
	}
    return stone_surface_max_y;
}

void MapgenDS::makeChunk(BlockMakeData *data)
{
    // Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);

	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;
	//TimeTaker t("makeChunk");

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	// Create a block-specific seed
	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate base terrain
	generateBaseTerrain();

	// Create heightmap
	updateHeightmap(node_min, node_max);

	// Init biome generator, place biome-specific nodes, and build biomemap
	if (flags & MG_BIOMES) {
		biomegen->calcBiomeNoise(node_min);
		generateBiomes();
	}

	// Generate the registered ores
	if (flags & MG_ORES)
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	// Generate the registered decorations
	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	if (flags & MG_BIOMES)
		dustTopNodes();

	// Calculate lighting
	if (flags & MG_LIGHT) {
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);
	}

	this->generating = false;
}


int MapgenDS::getSpawnLevelAtPoint(v2s16 p)
{
	return 0;
}

MapgenDS::~MapgenDS()
{
    delete landscape;
	delete noise_filler_depth;
}
