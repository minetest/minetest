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


BiomeDefManager::BiomeDefManager(IGameDef *gamedef) {
	this->m_gamedef = gamedef;
	this->ndef      = gamedef->ndef();

	//addDefaultBiomes(); //can't do this in the ctor, too early
}


BiomeDefManager::~BiomeDefManager() {
	for (int i = 0; i != bgroups.size(); i++)
		delete bgroups[i];
}


void BiomeDefManager::addBiome() {

}


NoiseParams npmtdef = {0.0, 20.0, v3f(250., 250., 250.), 82341, 5, 0.6};

void BiomeDefManager::addDefaultBiomes() {
	std::vector<Biome *> *bgroup;
	Biome *b;

	//bgroup = new std::vector<Biome *>;

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
	b->np = &npmtdef;
	biome_default = b;

	//bgroup->push_back(b);
	//bgroups.push_back(bgroup);
}


Biome *BiomeDefManager::getBiome(float bgfreq, float heat, float humidity) {
	std::vector<Biome *> bgroup;
	Biome *b;
	int i;

	int ngroups = bgroup_freqs.size();
	if (!ngroups)
		return biome_default;
	for (i = 0; (i != ngroups - 1) && (bgfreq > bgroup_freqs[i]); i++);
	bgroup = *(bgroups[i]);

	int nbiomes = bgroup.size();
	if (!nbiomes)
		return biome_default;
	for (i = 0; i != nbiomes - 1; i++) {
		b = bgroup[i];
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


void Biome::genColumn(Mapgen *mg, int x, int z, int y1, int y2) {
	//printf("(%d, %d): %f\n", x, z, mg->map_terrain[z * mg->ystride + x]);

	//int surfaceh = 4;
	int surfaceh = np->offset + np->scale * mg->map_terrain[(z - mg->node_min.Z) * 80 /*THIS IS TEMPORARY mg->ystride*/ + (x - mg->node_min.X)];
	//printf("gen column %f\n", );
	int y = y1;
	int i = mg->vmanip->m_area.index(x, y, z);  //z * mg->zstride + x + (y - mg->vmanip->m_area.MinEdge.Y) * mg->ystride;
	for (; y <= surfaceh - ntopnodes && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_filler;
	for (; y <= surfaceh && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_top;
	for (; y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = mg->n_air;
}



///////////////////////////// [ Ocean biome ] /////////////////////////////////



void BiomeOcean::genColumn(Mapgen *mg, int x, int z, int y1, int y2) {
	int y, i = 0;

	int surfaceh = np->offset + np->scale * mg->map_terrain[z * mg->ystride + x];

	i = z * mg->zstride + x;
	for (y = y1; y <= surfaceh - ntopnodes && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_filler;
	for (; y <= surfaceh && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_top;
	for (; y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = mg->n_air;
}


///////////////////////////// [ Nether biome ] /////////////////////////////////


int BiomeHell::getSurfaceHeight(float noise_terrain) {
	return np->offset + np->scale * noise_terrain;
}


void BiomeHell::genColumn(Mapgen *mg, int x, int z, int y1, int y2) {
	int y, i = 0;

	int surfaceh = np->offset + np->scale * mg->map_terrain[z * mg->ystride + x];

	i = z * mg->zstride + x;
	for (y = y1; y <= surfaceh - ntopnodes && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_filler;
	for (; y <= surfaceh && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_top;
	for (; y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = mg->n_air;
}


///////////////////////////// [ Aether biome ] ////////////////////////////////

/////////////////////////// [ Superflat biome ] ///////////////////////////////


int BiomeSuperflat::getSurfaceHeight(float noise_terrain) {
	return ntopnodes;
}


void BiomeSuperflat::genColumn(Mapgen *mg, int x, int z, int y1, int y2) {
	int y, i = 0;

	int surfaceh = ntopnodes;

	i = z * mg->zstride + x;
	for (y = y1; y <= surfaceh - ntopnodes && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_filler;
	for (; y <= surfaceh && y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = n_top;
	for (; y <= y2; y++, i += mg->ystride)
		mg->vmanip->m_data[i] = mg->n_air;
}
