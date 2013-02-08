/*
Minetest-c55
Copyright (C) 2010-2011 kwolekr, Ryan Kwolek <kwolekr2@cs.scranton.edu>

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
#include "main.h"

#define BT_NONE			0
#define BT_OCEAN		1
#define BT_LAKE			2
#define BT_SBEACH		3
#define BT_GBEACH		4
#define BT_PLAINS		5
#define BT_HILLS		6
#define BT_EXTREMEHILLS 7
#define BT_MOUNTAINS	8
#define BT_DESERT		9
#define BT_DESERTHILLS	10
#define BT_HELL			11
#define BT_AETHER		12

#define BT_BTMASK		0x3F

#define BTF_SNOW		0x40
#define BTF_FOREST		0x80

#define BGFREQ_1 (           0.40)
#define BGFREQ_2 (BGFREQ_1 + 0.05)
#define BGFREQ_3 (BGFREQ_2 + 0.08)
#define BGFREQ_4 (BGFREQ_3 + 0.35)
#define BGFREQ_5 (BGFREQ_4 + 0.18)
//BGFREQ_5 is not checked as an upper bound; it ought to sum up to 1.00, but it's okay if it doesn't.


/*float bg1_temps[] = {0.0};
int bg1_biomes[]  = {BT_OCEAN};

float bg2_temps[] = {10.0};
int bg2_biomes[]  = {BT_GBEACH, BT_SBEACH};

float bg3_temps[] = {30.0, 40.0};
int bg3_biomes[]  = {BT_HILLS, BT_EXTREMEHILLS, BT_MOUNTAINS};

float bg4_temps[] = {25.0, 30.0, 35.0, 40.0};
int bg4_biomes[]  = {BT_HILLS, BT_EXTREMEHILLS, BT_MOUNTAINS, BT_DESERT, BT_DESERTHILLS};

float bg5_temps[] = {5.0, 40.0};
int bg5_biomes[]  = {BT_LAKE, BT_PLAINS, BT_DESERT};*/

NoiseParams np_default = {20.0, 15.0, v3f(250., 250., 250.), 82341, 5, 0.6};


BiomeDefManager::BiomeDefManager(IGameDef *gamedef) {
	this->m_gamedef = gamedef;
	this->ndef      = gamedef->ndef();

	//the initial biome group
	bgroups.push_back(new std::vector<Biome *>);
}


BiomeDefManager::~BiomeDefManager() {
	for (unsigned int i = 0; i != bgroups.size(); i++)
		delete bgroups[i];
}


Biome *BiomeDefManager::createBiome(BiomeTerrainType btt) {
	switch (btt) {
		case BIOME_TERRAIN_NORMAL:
			return new Biome;
		case BIOME_TERRAIN_LIQUID:
			return new BiomeLiquid;
		case BIOME_TERRAIN_NETHER:
			return new BiomeHell;
		case BIOME_TERRAIN_AETHER:
			return new BiomeAether;
		case BIOME_TERRAIN_FLAT:
			return new BiomeSuperflat;
	}
	return NULL;
}


void BiomeDefManager::addBiomeGroup(float freq) {
	int size = bgroup_freqs.size();
	float newfreq = freq;

	if (size)
		newfreq += bgroup_freqs[size - 1];
	bgroup_freqs.push_back(newfreq);
	bgroups.push_back(new std::vector<Biome *>);

	verbosestream << "BiomeDefManager: added biome group with frequency " <<
		newfreq << std::endl;
}


void BiomeDefManager::addBiome(Biome *b) {
	std::vector<Biome *> *bgroup;

	if ((unsigned int)b->groupid >= bgroups.size()) {
		errorstream << "BiomeDefManager: attempted to add biome '" << b->name
		 << "' to nonexistent biome group " << b->groupid << std::endl;
		return;
	}

	bgroup = bgroups[b->groupid];
	bgroup->push_back(b);

	verbosestream << "BiomeDefManager: added biome '" << b->name <<
		"' to biome group " << (int)b->groupid << std::endl;
}


void BiomeDefManager::addDefaultBiomes() {
	Biome *b;

	b = new Biome;
	b->name         = "Default";
	b->n_top        = MapNode(ndef->getId("mapgen_stone"));
	b->n_filler     = b->n_top;
	b->ntopnodes    = 0;
	b->height_min   = -MAP_GENERATION_LIMIT;
	b->height_max   = MAP_GENERATION_LIMIT;
	b->heat_min     = FLT_MIN;
	b->heat_max     = FLT_MAX;
	b->humidity_min = FLT_MIN;
	b->humidity_max = FLT_MAX;
	b->np = &np_default;
	biome_default = b;
}


Biome *BiomeDefManager::getBiome(float bgfreq, float heat, float humidity) {
	std::vector<Biome *> *bgroup;
	Biome *b;
	int i;

	int ngroups = bgroup_freqs.size();
	if (!ngroups)
		return biome_default;
	for (i = 0; (i != ngroups) && (bgfreq > bgroup_freqs[i]); i++);
	bgroup = bgroups[i];

	int nbiomes = bgroup->size();
	for (i = 0; i != nbiomes; i++) {
		b = bgroup->operator[](i);
		if (heat >= b->heat_min && heat <= b->heat_max &&
			humidity >= b->humidity_min && humidity <= b->humidity_max)
			return b;
	}

	return biome_default;
}


//////////////////////////// [ Generic biome ] ////////////////////////////////


int Biome::getSurfaceHeight(float noise_terrain) {
	return np->offset + np->scale * noise_terrain;
}


void Biome::genColumn(Mapgen *mapgen, int x, int z, int y1, int y2) {

}


///////////////////////////// [ Ocean biome ] /////////////////////////////////


void BiomeLiquid::genColumn(Mapgen *mapgen, int x, int z, int y1, int y2) {

}


///////////////////////////// [ Nether biome ] /////////////////////////////////


int BiomeHell::getSurfaceHeight(float noise_terrain) {
	return np->offset + np->scale * noise_terrain;
}


void BiomeHell::genColumn(Mapgen *mapgen, int x, int z, int y1, int y2) {

}


///////////////////////////// [ Aether biome ] ////////////////////////////////


int BiomeAether::getSurfaceHeight(float noise_terrain) {
	return np->offset + np->scale * noise_terrain;
}


void BiomeAether::genColumn(Mapgen *mapgen, int x, int z, int y1, int y2) {

}


/////////////////////////// [ Superflat biome ] ///////////////////////////////


int BiomeSuperflat::getSurfaceHeight(float noise_terrain) {
	return ntopnodes;
}


void BiomeSuperflat::genColumn(Mapgen *mapgen, int x, int z, int y1, int y2) {

}
