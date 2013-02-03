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

#include "mapgen_flat.h"
#include "mapgen.h"
#include "voxel.h"
#include "map.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "main.h" // For g_profiler
#include "profiler.h"
#include "treegen.h"

MapgenFlat::MapgenFlat(MapgenFlatParams *params)
{
	this->generate_trees = params->generate_trees;
}

void MapgenFlat::makeChunk(BlockMakeData *data)
{
	if(data->no_op)
	{
		//dstream<<"makeBlock: no-op"<<std::endl;
		return;
	}

	INodeDefManager *ndef = data->nodedef;
	ManualMapVoxelManipulator &vmanip = *(data->vmanip);

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;


	// Area of central chunk
	v3s16 node_min = blockpos_min*MAP_BLOCKSIZE;
	v3s16 node_max = (blockpos_max+v3s16(1,1,1))*MAP_BLOCKSIZE-v3s16(1,1,1);
	v3s16 central_area_size = node_max - node_min + v3s16(1,1,1);

	content_t c_air = ndef->getId("mapgen_air");
	content_t c_dirt_with_grass = ndef->getId("mapgen_dirt_with_grass");
	content_t c_dirt = ndef->getId("mapgen_dirt");
	content_t c_stone= ndef->getId("mapgen_stone");
	
	for(s16 x=node_min.X; x<=node_max.X; x++)
	for(s16 z=node_min.Z; z<=node_max.Z; z++)
	{
		// Node position
		v2s16 p2d = v2s16(x,z);
		// Use fast index incrementing
		v3s16 em = vmanip.m_area.getExtent();
		u32 i = vmanip.m_area.index(v3s16(p2d.X, node_min.Y, p2d.Y));
		for(s16 y=node_min.Y; y<=node_max.Y; y++)
		{
			if(vmanip.m_data[i].getContent() == CONTENT_IGNORE){
				if(y <= -3){
						vmanip.m_data[i] = MapNode(c_stone);
				} else if(y <= -1){
					vmanip.m_data[i] = MapNode(c_dirt);
				} else if(y == 0){
					vmanip.m_data[i] = MapNode(c_dirt_with_grass);
				} else {
					vmanip.m_data[i] = MapNode(c_air);
				}
			}
			vmanip.m_area.add_y(em, i, 1);
		}
	}

	/*
		Generate trees
	*/
	if(generate_trees)
	{
		// Divide area into parts
		s16 div = 8;
		s16 sidelen = central_area_size.X / div;
		double area = sidelen * sidelen;
		for(s16 x0=0; x0<div; x0++)
		for(s16 z0=0; z0<div; z0++)
		{
			// Center position of part of division
			v2s16 p2d_center(
				node_min.X + sidelen/2 + sidelen*x0,
				node_min.Z + sidelen/2 + sidelen*z0
			);
			// Minimum edge of part of division
			v2s16 p2d_min(
				node_min.X + sidelen*x0,
				node_min.Z + sidelen*z0
			);
			// Maximum edge of part of division
			v2s16 p2d_max(
				node_min.X + sidelen + sidelen*x0 - 1,
				node_min.Z + sidelen + sidelen*z0 - 1
			);
			// Amount of trees
			u32 tree_count = area * tree_amount_2d(data->seed, p2d_center);
			// Put trees in random places on part of division
			for(u32 i=0; i<tree_count; i++)
			{
				s16 x = myrand_range(p2d_min.X, p2d_max.X);
				s16 z = myrand_range(p2d_min.Y, p2d_max.Y);
				s16 y = 0;
				v3s16 p(x,y,z);
				/*
					Trees grow only on mud and grass
				*/
				{
					u32 i = vmanip.m_area.index(v3s16(p));
					MapNode *n = &vmanip.m_data[i];
					if(n->getContent() != c_dirt
							&& n->getContent() != c_dirt_with_grass)
						continue;
				}
				p.Y++;
				// Make a tree
				treegen::make_tree(vmanip, p, false, ndef);
			}
		}
	}

	/*
		Calculate lighting
	*/
	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update",
			SPT_AVG);
	VoxelArea a(node_min-v3s16(1,0,1)*MAP_BLOCKSIZE,
			node_max+v3s16(1,0,1)*MAP_BLOCKSIZE);
	enum LightBank banks[2] = {LIGHTBANK_DAY, LIGHTBANK_NIGHT};
	for(int i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];

		core::map<v3s16, bool> light_sources;
		core::map<v3s16, u8> unlight_from;

		voxalgo::clearLightAndCollectSources(vmanip, a, bank, ndef,
				light_sources, unlight_from);
		
		bool inexistent_top_provides_sunlight = node_max.Y >= 0;
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				vmanip, a, inexistent_top_provides_sunlight,
				light_sources, ndef);
		// TODO: Do stuff according to bottom_sunlight_valid

		vmanip.unspreadLight(bank, unlight_from, light_sources, ndef);

		vmanip.spreadLight(bank, light_sources, ndef);
	}
}
