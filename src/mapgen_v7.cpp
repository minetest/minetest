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


#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
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
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "treegen.h"
#include "biome.h"
#include "mapgen_v7.h"


/////////////////// Mapgen V7 perlin noise default values
NoiseParams nparams_v7_def_terrain_base =
	{0, 80.0, v3f(250.0, 250.0, 250.0), 82341, 5, 0.6};
NoiseParams nparams_v7_def_terrain_alt =
	{0, 20.0, v3f(250.0, 250.0, 250.0), 5934, 5, 0.6};
NoiseParams nparams_v7_def_terrain_mod =
	{0, 1.0, v3f(350.0, 350.0, 350.0), 85039, 5, 0.6};
NoiseParams nparams_v7_def_terrain_persist =
	{0, 1.0, v3f(500.0, 500.0, 500.0), 539, 3, 0.6};
NoiseParams nparams_v7_def_height_select =
	{0.5, 0.5, v3f(250.0, 250.0, 250.0), 4213, 5, 0.69};
NoiseParams nparams_v7_def_ridge =
	{0.5, 1.0, v3f(100.0, 100.0, 100.0), 6467, 4, 0.75};
/*
NoiseParams nparams_v6_def_beach =
	{0.0, 1.0, v3f(250.0, 250.0, 250.0), 59420, 3, 0.50};
NoiseParams nparams_v6_def_cave =
	{6.0, 6.0, v3f(250.0, 250.0, 250.0), 34329, 3, 0.50};
NoiseParams nparams_v6_def_humidity =
	{0.5, 0.5, v3f(500.0, 500.0, 500.0), 72384, 4, 0.66};
NoiseParams nparams_v6_def_trees =
	{0.0, 1.0, v3f(125.0, 125.0, 125.0), 2, 4, 0.66};
NoiseParams nparams_v6_def_apple_trees =
	{0.0, 1.0, v3f(100.0, 100.0, 100.0), 342902, 3, 0.45};
*/
///////////////////////////////////////////////////////////////////////////////


MapgenV7::MapgenV7(int mapgenid, MapgenV7Params *params, EmergeManager *emerge) {
	this->generating  = false;
	this->id     = mapgenid;
	this->emerge = emerge;
	this->bmgr   = emerge->biomedef;

	this->seed     = (int)params->seed;
	this->water_level = params->water_level;
	this->flags   = params->flags;
	this->csize   = v3s16(1, 1, 1) * params->chunksize * MAP_BLOCKSIZE;
//	this->ystride = csize.X; //////fix this

	this->biomemap  = new u8[csize.X * csize.Z];
	this->heightmap = new s16[csize.X * csize.Z];
	this->ridge_heightmap = new s16[csize.X * csize.Z];

	// Terrain noise
	noise_terrain_base    = new Noise(params->np_terrain_base,    seed, csize.X, csize.Z);
	noise_terrain_alt     = new Noise(params->np_terrain_alt,     seed, csize.X, csize.Z);
	noise_terrain_mod     = new Noise(params->np_terrain_mod,     seed, csize.X, csize.Z);
	noise_terrain_persist = new Noise(params->np_terrain_persist, seed, csize.X, csize.Z);
	noise_height_select   = new Noise(params->np_height_select,   seed, csize.X, csize.Z);
	noise_ridge           = new Noise(params->np_ridge, seed, csize.X, csize.Y, csize.Z);
	
	// Biome noise
	noise_heat     = new Noise(bmgr->np_heat,     seed, csize.X, csize.Z);
	noise_humidity = new Noise(bmgr->np_humidity, seed, csize.X, csize.Z);	
}


MapgenV7::~MapgenV7() {
	delete noise_terrain_base;
	delete noise_terrain_mod;
	delete noise_terrain_persist;
	delete noise_height_select;
	delete noise_terrain_alt;
	delete noise_ridge;
	delete noise_heat;
	delete noise_humidity;
	
	delete[] ridge_heightmap;
	delete[] heightmap;
	delete[] biomemap;
}


int MapgenV7::getGroundLevelAtPoint(v2s16 p) {
	s16 groundlevel = baseTerrainLevelAtPoint(p.X, p.Y);
	float heat      = NoisePerlin2D(bmgr->np_heat, p.X, p.Y, seed);
	float humidity  = NoisePerlin2D(bmgr->np_humidity, p.X, p.Y, seed);
	Biome *b = bmgr->getBiome(heat, humidity, groundlevel);
	
	s16 y = groundlevel;
	if (y > water_level) {
		int iters = 1024; // don't even bother iterating more than 1024 times..
		while (iters--) {
			float ridgenoise = NoisePerlin3D(noise_ridge->np, p.X, y, p.Y, seed);
			if (ridgenoise * (float)(y * y) < 15.0)
				break;
			y--;
		}
	}

	return y + b->top_depth;
}


void MapgenV7::makeChunk(BlockMakeData *data) {
	assert(data->vmanip);
	assert(data->nodedef);
	assert(data->blockpos_requested.X >= data->blockpos_min.X &&
		   data->blockpos_requested.Y >= data->blockpos_min.Y &&
		   data->blockpos_requested.Z >= data->blockpos_min.Z);
	assert(data->blockpos_requested.X <= data->blockpos_max.X &&
		   data->blockpos_requested.Y <= data->blockpos_max.Y &&
		   data->blockpos_requested.Z <= data->blockpos_max.Z);
			
	this->generating = true;
	this->vm   = data->vmanip;	
	this->ndef = data->nodedef;
	//TimeTaker t("makeChunk");
	
	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	v3s16 blockpos_full_min = blockpos_min - v3s16(1, 1, 1);
	v3s16 blockpos_full_max = blockpos_max + v3s16(1, 1, 1);
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = emerge->getBlockSeed(full_node_min);  //////use getBlockSeed2()!

	// Make some noise
	calculateNoise();

	// Calculate height map
	s16 stone_surface_max_y = calcHeightMap();
	
	// Calculate biomes
	BiomeNoiseInput binput;
	binput.mapsize       = v2s16(csize.X, csize.Z);
	binput.heat_map      = noise_heat->result;
	binput.humidity_map  = noise_humidity->result;
	binput.height_map    = heightmap;
	bmgr->calcBiomes(&binput, biomemap);
	
	c_stone           = ndef->getId("mapgen_stone");
	c_dirt            = ndef->getId("mapgen_dirt");
	c_dirt_with_grass = ndef->getId("mapgen_dirt_with_grass");
	c_sand            = ndef->getId("mapgen_sand");
	c_water_source    = ndef->getId("mapgen_water_source");
	c_lava_source     = ndef->getId("mapgen_lava_source");
	
	generateTerrain();
	carveRidges();
	
	generateCaves(stone_surface_max_y);
	addTopNodes();
	//v3s16 central_area_size = node_max - node_min + v3s16(1,1,1);

	if (flags & MG_DUNGEONS) {
		DungeonGen dgen(ndef, data->seed, water_level);
		dgen.generate(vm, blockseed, full_node_min, full_node_max);
	}

	for (size_t i = 0; i != emerge->ores.size(); i++) {
		Ore *ore = emerge->ores[i];
		ore->placeOre(this, blockseed + i, node_min, node_max);
	}
	
	//printf("makeChunk: %dms\n", t.stop());
	
	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);
	
	calcLighting(node_min - v3s16(1, 0, 1) * MAP_BLOCKSIZE,
				 node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE);
	//setLighting(node_min - v3s16(1, 0, 1) * MAP_BLOCKSIZE,
	//			node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE, 0xFF);

	this->generating = false;
}


void MapgenV7::calculateNoise() {
	//TimeTaker t("calculateNoise", NULL, PRECISION_MICRO);
	int x = node_min.X;
	int y = node_min.Y;
	int z = node_min.Z;
	
	noise_terrain_mod->perlinMap2D(x, z);
	
	noise_height_select->perlinMap2D(x, z);
	noise_height_select->transformNoiseMap();
	
	noise_terrain_persist->perlinMap2D(x, z);
	float *persistmap = noise_terrain_persist->result;
	for (int i = 0; i != csize.X * csize.Z; i++)
		persistmap[i] = abs(persistmap[i]);
	
	noise_terrain_base->perlinMap2DModulated(x, z, persistmap);
	noise_terrain_base->transformNoiseMap();
	
	noise_terrain_alt->perlinMap2DModulated(x, z, persistmap);
	noise_terrain_alt->transformNoiseMap();
	
	noise_ridge->perlinMap3D(x, y, z);
	
	noise_heat->perlinMap2D(x, z);
	
	noise_humidity->perlinMap2D(x, z);
	
	//printf("calculateNoise: %dus\n", t.stop());
}


Biome *MapgenV7::getBiomeAtPoint(v3s16 p) {
	float heat      = NoisePerlin2D(bmgr->np_heat, p.X, p.Z, seed);
	float humidity  = NoisePerlin2D(bmgr->np_humidity, p.X, p.Z, seed);
	s16 groundlevel = baseTerrainLevelAtPoint(p.X, p.Z);
	
	return bmgr->getBiome(heat, humidity, groundlevel);
}


float MapgenV7::baseTerrainLevelAtPoint(int x, int z) {
	float terrain_mod = NoisePerlin2DNoTxfm(noise_terrain_mod->np, x, z, seed);
	float hselect     = NoisePerlin2D(noise_height_select->np, x, z, seed);
	float persist     = abs(NoisePerlin2DNoTxfm(noise_terrain_persist->np, x, z, seed));

	noise_terrain_base->np->persist = persist;
	float terrain_base = NoisePerlin2D(noise_terrain_base->np, x, z, seed);
	float height_base  = terrain_base * terrain_mod;

	noise_terrain_alt->np->persist = persist;
	float height_alt = NoisePerlin2D(noise_terrain_alt->np, x, z, seed);

	return (height_base * hselect) + (height_alt * (1.0 - hselect));
}


float MapgenV7::baseTerrainLevelFromMap(int index) {
	float terrain_mod  = noise_terrain_mod->result[index];
	float hselect      = noise_height_select->result[index];
	float terrain_base = noise_terrain_base->result[index];
	float height_base  = terrain_base * terrain_mod;
	float height_alt   = noise_terrain_alt->result[index];

	return (height_base * hselect) + (height_alt * (1.0 - hselect));
}


#if 0
// Crap code to test log rivers as a proof-of-concept.  Didn't work out too well.
void MapgenV7::carveRivers() {
	MapNode n_air(CONTENT_AIR), n_water_source(c_water_source);
	MapNode n_stone(c_stone);
	u32 index = 0;
	
	int river_depth = 4;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		float terrain_mod  = noise_terrain_mod->result[index];
		NoiseParams *np = noise_terrain_river->np;
		np->persist = noise_terrain_persist->result[index];
		float terrain_river = NoisePerlin2DNoTxfm(np, x, z, seed);
		float height = terrain_river * (1 - abs(terrain_mod)) *
						noise_terrain_river->np->scale;
		height = log(height * height); //log(h^3) is pretty interesting for terrain
		
		s16 y = heightmap[index];
		if (height < 1.0 && y > river_depth &&
			y - river_depth >= node_min.Y && y <= node_max.Y) {
			
			for (s16 ry = y; ry != y - river_depth; ry--) {
				u32 vi = vm->m_area.index(x, ry, z);
				vm->m_data[vi] = n_air;
			}
			
			u32 vi = vm->m_area.index(x, y - river_depth, z);
			vm->m_data[vi] = n_water_source;
		}
	}
}
#endif


int MapgenV7::calcHeightMap() {
	int stone_surface_max_y = -MAP_GENERATION_LIMIT;
	u32 index = 0;
	
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		float surface_height = baseTerrainLevelFromMap(index);
		s16 surface_y = (s16)surface_height;
		
		heightmap[index]       = surface_y; 
		ridge_heightmap[index] = surface_y;
		
		if (surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;
	}
		
	return stone_surface_max_y;
}


void MapgenV7::generateTerrain() {
	MapNode n_air(CONTENT_AIR), n_water_source(c_water_source);
	MapNode n_stone(c_stone);

	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;
	
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		s16 surface_y = heightmap[index];
		Biome *biome = bmgr->biomes[biomemap[index]];
		
		u32 i = vm->m_area.index(x, node_min.Y, z);
		for (s16 y = node_min.Y; y <= node_max.Y; y++) {
			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (y <= surface_y) {
					vm->m_data[i] = (y > water_level + biome->filler_height) ?
						MapNode(biome->c_filler) : n_stone;
				} else if (y <= water_level) {
					vm->m_data[i] = n_water_source;
				} else {
					vm->m_data[i] = n_air;
				}
			}
			vm->m_area.add_y(em, i, 1);
		}
	}
}


void MapgenV7::carveRidges() {
	if (node_max.Y <= water_level)
		return;
		
	MapNode n_air(CONTENT_AIR);
	u32 index = 0;
	
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y; y <= node_max.Y; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++, index++, vi++) {
			// Removing this check will create huge underwater caverns,
			// which are interesting but not desirable for gameplay
			if (y <= water_level)
				continue;
				
			if (noise_ridge->result[index] * (float)(y * y) < 15.0)
				continue;

			int j = (z - node_min.Z) * csize.Z + (x - node_min.X); //////obviously just temporary
			if (y < ridge_heightmap[j])
				ridge_heightmap[j] = y - 1; 

			vm->m_data[vi] = n_air;
		}
	}
}

/*
void MapgenV7::testBiomes() {
	u32 index = 0;
	
	for (s16 z = node_min.Z; z <= node_min.Z; z++)
	for (s16 x = node_min.X; x <= node_min.X; x++) {;
		Biome *b = bmgr->getBiome(heat, humidity, 0);
	}
	// make an 80x80 grid with axes heat/humidity as a voroni diagram for biomes
	// clear out y space for it first with air
	// use absolute positioning, each chunk will be a +1 height
}*/


void MapgenV7::addTopNodes() {
	v3s16 em = vm->m_area.getExtent();
	s16 ntopnodes;
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome = bmgr->biomes[biomemap[index]];
		
		//////////////////// First, add top nodes below the ridge
		s16 y = ridge_heightmap[index];
		
		// This cutoff is good enough, but not perfect.
		// It will cut off potentially placed top nodes at chunk boundaries
		if (y < node_min.Y)
			continue;
		if (y > node_max.Y) {
			y = node_max.Y; // Let's see if we can still go downward anyway
			u32 vi = vm->m_area.index(x, y, z);
			content_t c = vm->m_data[vi].getContent();
			if (ndef->get(c).walkable)
				continue;
		}
		
		// N.B.  It is necessary to search downward since range_heightmap[i]
		// might not be the actual height, just the lowest part in the chunk
		// where a ridge had been carved
		u32 i = vm->m_area.index(x, y, z);
		for (; y >= node_min.Y; y--) {
			content_t c = vm->m_data[i].getContent();
			if (ndef->get(c).walkable)
				break;
			vm->m_area.add_y(em, i, -1);
		}

		if (y != node_min.Y - 1 && y >= water_level) {
			ridge_heightmap[index] = y; //update ridgeheight
			ntopnodes = biome->top_depth;
			for (; y <= node_max.Y && ntopnodes; y++) {
				ntopnodes--;
				vm->m_data[i] = MapNode(biome->c_top);
				vm->m_area.add_y(em, i, 1);
			}
			// If dirt, grow grass on it.
			if (vm->m_data[i].getContent() == CONTENT_AIR) {
				vm->m_area.add_y(em, i, -1);
				if (vm->m_data[i].getContent() == c_dirt)
					vm->m_data[i] = MapNode(c_dirt_with_grass);
			}
		}
		
		//////////////////// Now, add top nodes on top of the ridge
		y = heightmap[index];
		if (y > node_max.Y) {
			y = node_max.Y; // Let's see if we can still go downward anyway
			u32 vi = vm->m_area.index(x, y, z);
			content_t c = vm->m_data[vi].getContent();
			if (ndef->get(c).walkable)
				continue;
		}

		i = vm->m_area.index(x, y, z);
		for (; y >= node_min.Y; y--) {
			content_t c = vm->m_data[i].getContent();
			if (ndef->get(c).walkable)
				break;
			vm->m_area.add_y(em, i, -1);
		}

		if (y != node_min.Y - 1) {
			ntopnodes = biome->top_depth;
			// Let's see if we've already added it...
			if (y == ridge_heightmap[index] + ntopnodes - 1)
				continue;

			for (; y <= node_max.Y && ntopnodes; y++) {
				ntopnodes--;
				vm->m_data[i] = MapNode(biome->c_top);
				vm->m_area.add_y(em, i, 1);
			}
			// If dirt, grow grass on it.
			if (vm->m_data[i].getContent() == CONTENT_AIR) {
				vm->m_area.add_y(em, i, -1);
				if (vm->m_data[i].getContent() == c_dirt)
					vm->m_data[i] = MapNode(c_dirt_with_grass);
			}
		}
	}
}


#include "mapgen_v6.h"
void MapgenV7::generateCaves(int max_stone_y) {
	PseudoRandom ps(blockseed + 21343);
	PseudoRandom ps2(blockseed + 1032);

	int volume_nodes = (node_max.X - node_min.X + 1) *
					   (node_max.Y - node_min.Y + 1) * MAP_BLOCKSIZE;
	float cave_amount = NoisePerlin2D(&nparams_v6_def_cave,
								node_min.X, node_min.Y, seed);
	
	u32 caves_count = MYMAX(0.0, cave_amount) * volume_nodes / 50000;
	for (u32 i = 0; i < caves_count; i++) {
		CaveV6 cave(this, &ps, &ps2, false, c_water_source, c_lava_source);
		cave.makeCave(node_min, node_max, max_stone_y);
	}
	
	u32 bruises_count = (ps.range(1, 6) == 1) ? ps.range(0, ps.range(0, 2)) : 1;
	for (u32 i = 0; i < bruises_count; i++) {
		CaveV6 cave(this, &ps, &ps2, true, c_water_source, c_lava_source);
		cave.makeCave(node_min, node_max, max_stone_y);
	}	
}
