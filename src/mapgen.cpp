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

#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "biome.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
//#include "serverobject.h"
#include "content_sao.h"
#include "nodedef.h"
#include "content_mapnode.h" // For content_mapnode_get_new_name
#include "voxelalgorithms.h"
#include "profiler.h"
#include "settings.h" // For g_settings
#include "main.h" // For g_profiler
#include "treegen.h"
#include "mapgen_v6.h"

FlagDesc flagdesc_mapgen[] = {
	{"trees",          MG_TREES},
	{"caves",          MG_CAVES},
	{"dungeons",       MG_DUNGEONS},
	{"v6_forests",     MGV6_FORESTS},
	{"v6_biome_blend", MGV6_BIOME_BLEND},
	{"flat",           MG_FLAT},
	{NULL,			   0}
};


///////////////////////////////////////////////////////////////////////////////


void Mapgen::updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax) {
	bool isliquid, wasliquid;
	v3s16 em  = vm->m_area.getExtent();

	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 x = nmin.X; x <= nmax.X; x++) {
			wasliquid = true;
			
			u32 i = vm->m_area.index(x, nmax.Y, z);
			for (s16 y = nmax.Y; y >= nmin.Y; y--) {
				isliquid = ndef->get(vm->m_data[i]).isLiquid();
				
				// there was a change between liquid and nonliquid, add to queue
				if (isliquid != wasliquid)
					trans_liquid->push_back(v3s16(x, y, z));

				wasliquid = isliquid;
				vm->m_area.add_y(em, i, -1);
			}
		}
	}
}


void Mapgen::setLighting(v3s16 nmin, v3s16 nmax, u8 light) {
	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	VoxelArea a(nmin - v3s16(1,0,1) * MAP_BLOCKSIZE,
				nmax + v3s16(1,0,1) * MAP_BLOCKSIZE);

	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int y = a.MinEdge.Y; y <= a.MaxEdge.Y; y++) {
			u32 i = vm->m_area.index(a.MinEdge.X, y, z);
			for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++, i++)
				vm->m_data[i].param1 = light;
		}
	}
}


void Mapgen::lightSpread(VoxelArea &a, v3s16 p, u8 light) {
	if (light <= 1 || !a.contains(p))
		return;
		
	u32 vi = vm->m_area.index(p);
	MapNode &nn = vm->m_data[vi];

	light--;
	// should probably compare masked, but doesn't seem to make a difference
	if (light <= nn.param1 || !ndef->get(nn).light_propagates)
		return;
	
	nn.param1 = light;
	
	lightSpread(a, p + v3s16(0, 0, 1), light);
	lightSpread(a, p + v3s16(0, 1, 0), light);
	lightSpread(a, p + v3s16(1, 0, 0), light);
	lightSpread(a, p - v3s16(0, 0, 1), light);
	lightSpread(a, p - v3s16(0, 1, 0), light);
	lightSpread(a, p - v3s16(1, 0, 0), light);
}


void Mapgen::updateLighting(v3s16 nmin, v3s16 nmax) {
	VoxelArea a(nmin - v3s16(1,0,1) * MAP_BLOCKSIZE,
				nmax + v3s16(1,0,1) * MAP_BLOCKSIZE);
	bool block_is_underground = (water_level >= nmax.Y);

	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	//TimeTaker t("updateLighting");

	// first, send vertical rays of sunshine downward
	v3s16 em = vm->m_area.getExtent();
	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++) {
			// see if we can get a light value from the overtop
			u32 i = vm->m_area.index(x, a.MaxEdge.Y + 1, z);
			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (block_is_underground)
					continue;
			} else if ((vm->m_data[i].param1 & 0x0F) != LIGHT_SUN) {
				continue;
			}
			vm->m_area.add_y(em, i, -1);

			for (int y = a.MaxEdge.Y; y >= a.MinEdge.Y; y--) {
				MapNode &n = vm->m_data[i];
				if (!ndef->get(n).sunlight_propagates)
					break;
				n.param1 = LIGHT_SUN;
				vm->m_area.add_y(em, i, -1);
			}
		}
	}
	
	// now spread the sunlight and light up any sources
	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int y = a.MinEdge.Y; y <= a.MaxEdge.Y; y++) {
			u32 i = vm->m_area.index(a.MinEdge.X, y, z);
			for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++, i++) {
				MapNode &n = vm->m_data[i];
				if (n.getContent() == CONTENT_IGNORE ||
					!ndef->get(n).light_propagates)
					continue;
				
				u8 light_produced = ndef->get(n).light_source & 0x0F;
				if (light_produced)
					n.param1 = light_produced;
				
				u8 light = n.param1 & 0x0F;
				if (light) {
					lightSpread(a, v3s16(x,     y,     z + 1), light);
					lightSpread(a, v3s16(x,     y + 1, z    ), light);
					lightSpread(a, v3s16(x + 1, y,     z    ), light);
					lightSpread(a, v3s16(x,     y,     z - 1), light);
					lightSpread(a, v3s16(x,     y - 1, z    ), light);
					lightSpread(a, v3s16(x - 1, y,     z    ), light);
				}
			}
		}
	}
	
	//printf("updateLighting: %dms\n", t.stop());
}


void Mapgen::updateLightingOld(v3s16 nmin, v3s16 nmax) {
	enum LightBank banks[2] = {LIGHTBANK_DAY, LIGHTBANK_NIGHT};

	VoxelArea a(nmin - v3s16(1,0,1) * MAP_BLOCKSIZE,
				nmax + v3s16(1,0,1) * MAP_BLOCKSIZE);
	bool block_is_underground = (water_level > nmax.Y);
	bool sunlight = !block_is_underground;

	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	
	for (int i = 0; i < 2; i++) {
		enum LightBank bank = banks[i];
		std::set<v3s16> light_sources;
		std::map<v3s16, u8> unlight_from;

		voxalgo::clearLightAndCollectSources(*vm, a, bank, ndef,
                                        light_sources, unlight_from);
		voxalgo::propagateSunlight(*vm, a, sunlight, light_sources, ndef);

		vm->unspreadLight(bank, unlight_from, light_sources, ndef);
		vm->spreadLight(bank, light_sources, ndef);
	}
}


//////////////////////// Mapgen V6 parameter read/write

bool MapgenV6Params::readParams(Settings *settings) {
	freq_desert = settings->getFloat("mgv6_freq_desert");
	freq_beach  = settings->getFloat("mgv6_freq_beach");

	np_terrain_base   = settings->getNoiseParams("mgv6_np_terrain_base");
	np_terrain_higher = settings->getNoiseParams("mgv6_np_terrain_higher");
	np_steepness      = settings->getNoiseParams("mgv6_np_steepness");
	np_height_select  = settings->getNoiseParams("mgv6_np_height_select");
	np_trees          = settings->getNoiseParams("mgv6_np_trees");
	np_mud            = settings->getNoiseParams("mgv6_np_mud");
	np_beach          = settings->getNoiseParams("mgv6_np_beach");
	np_biome          = settings->getNoiseParams("mgv6_np_biome");
	np_cave           = settings->getNoiseParams("mgv6_np_cave");

	bool success =
		np_terrain_base  && np_terrain_higher && np_steepness &&
		np_height_select && np_trees          && np_mud       &&
		np_beach         && np_biome          && np_cave;
	return success;
}


void MapgenV6Params::writeParams(Settings *settings) {
	settings->setFloat("mgv6_freq_desert", freq_desert);
	settings->setFloat("mgv6_freq_beach",  freq_beach);
	
	settings->setNoiseParams("mgv6_np_terrain_base",   np_terrain_base);
	settings->setNoiseParams("mgv6_np_terrain_higher", np_terrain_higher);
	settings->setNoiseParams("mgv6_np_steepness",      np_steepness);
	settings->setNoiseParams("mgv6_np_height_select",  np_height_select);
	settings->setNoiseParams("mgv6_np_trees",          np_trees);
	settings->setNoiseParams("mgv6_np_mud",            np_mud);
	settings->setNoiseParams("mgv6_np_beach",          np_beach);
	settings->setNoiseParams("mgv6_np_biome",          np_biome);
	settings->setNoiseParams("mgv6_np_cave",           np_cave);
}


/////////////////////////////////// legacy static functions for farmesh


s16 Mapgen::find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision) {
	//just need to return something
	s16 level = 5;
	return level;
}


bool Mapgen::get_have_beach(u64 seed, v2s16 p2d) {
	double sandnoise = noise2d_perlin(
			0.2+(float)p2d.X/250, 0.7+(float)p2d.Y/250,
			seed+59420, 3, 0.50);

	return (sandnoise > 0.15);
}


double Mapgen::tree_amount_2d(u64 seed, v2s16 p) {
	double noise = noise2d_perlin(
			0.5+(float)p.X/125, 0.5+(float)p.Y/125,
			seed+2, 4, 0.66);
	double zeroval = -0.39;
	if(noise < zeroval)
		return 0;
	else
		return 0.04 * (noise-zeroval) / (1.0-zeroval);
}
