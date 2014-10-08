/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include "biome.h"
#include "nodedef.h"
#include "map.h" //for ManualMapVoxelManipulator
#include "log.h"
#include "util/numeric.h"
#include "main.h"
#include "util/mathconstants.h"
#include "porting.h"

NoiseParams nparams_biome_def_heat(50, 50, v3f(500.0, 500.0, 500.0), 5349, 3, 0.70);
NoiseParams nparams_biome_def_humidity(50, 50, v3f(500.0, 500.0, 500.0), 842, 3, 0.55);


BiomeDefManager::BiomeDefManager(NodeResolver *resolver) {
	biome_registration_finished = false;
	np_heat     = &nparams_biome_def_heat;
	np_humidity = &nparams_biome_def_humidity;

	// Create default biome to be used in case none exist
	Biome *b = new Biome;
	
	b->id             = 0;
	b->name           = "Default";
	b->flags          = 0;
	b->depth_top      = 0;
	b->depth_filler   = 0;
	b->height_min     = -MAP_GENERATION_LIMIT;
	b->height_max     = MAP_GENERATION_LIMIT;
	b->heat_point     = 0.0;
	b->humidity_point = 0.0;

	resolver->addNode("air",                 "", CONTENT_AIR, &b->c_top);
	resolver->addNode("air",                 "", CONTENT_AIR, &b->c_filler);
	resolver->addNode("mapgen_water_source", "", CONTENT_AIR, &b->c_water);
	resolver->addNode("air",                 "", CONTENT_AIR, &b->c_dust);
	resolver->addNode("mapgen_water_source", "", CONTENT_AIR, &b->c_dust_water);

	biomes.push_back(b);
}


BiomeDefManager::~BiomeDefManager() {
	//if (biomecache)
	//	delete[] biomecache;
	
	for (size_t i = 0; i != biomes.size(); i++)
		delete biomes[i];
}


Biome *BiomeDefManager::createBiome(BiomeTerrainType btt) {
	/*switch (btt) {
		case BIOME_TERRAIN_NORMAL:
			return new Biome;
		case BIOME_TERRAIN_LIQUID:
			return new BiomeLiquid;
		case BIOME_TERRAIN_NETHER:
			return new BiomeHell;
		case BIOME_TERRAIN_AETHER:
			return new BiomeSky;
		case BIOME_TERRAIN_FLAT:
			return new BiomeSuperflat;
	}
	return NULL;*/
	return new Biome;
}


// just a PoC, obviously needs optimization later on (precalculate this)
void BiomeDefManager::calcBiomes(BiomeNoiseInput *input, u8 *biomeid_map) {
	int i = 0;
	for (int y = 0; y != input->mapsize.Y; y++) {
		for (int x = 0; x != input->mapsize.X; x++, i++) {
			float heat     = (input->heat_map[i] + 1) * 50;
			float humidity = (input->humidity_map[i] + 1) * 50;
			biomeid_map[i] = getBiome(heat, humidity, input->height_map[i])->id;
		}
	}
}


void BiomeDefManager::addBiome(Biome *b) {
	if (biome_registration_finished) {
		errorstream << "BIomeDefManager: biome registration already "
			"finished, dropping " << b->name << std::endl;
		delete b;
		return;
	}
	
	size_t nbiomes = biomes.size();
	if (nbiomes >= 0xFF) {
		errorstream << "BiomeDefManager: too many biomes, dropping "
			<< b->name << std::endl;
		delete b;
		return;
	}

	b->id = (u8)nbiomes;
	biomes.push_back(b);
	verbosestream << "BiomeDefManager: added biome " << b->name << std::endl;
}


Biome *BiomeDefManager::getBiome(float heat, float humidity, s16 y) {
	Biome *b, *biome_closest = NULL;
	float dist_min = FLT_MAX;

	for (size_t i = 1; i < biomes.size(); i++) {
		b = biomes[i];
		if (y > b->height_max || y < b->height_min)
			continue;

		float d_heat     = heat     - b->heat_point;
		float d_humidity = humidity - b->humidity_point;
		float dist = (d_heat * d_heat) +
					 (d_humidity * d_humidity);
		if (dist < dist_min) {
			dist_min = dist;
			biome_closest = b;
		}
	}
	
	return biome_closest ? biome_closest : biomes[0];
}


u8 BiomeDefManager::getBiomeIdByName(const char *name) {
	for (size_t i = 0; i != biomes.size(); i++) {
		if (!strcasecmp(name, biomes[i]->name.c_str()))
			return i;
	}
	
	return 0;
}
