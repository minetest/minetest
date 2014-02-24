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
#include "mapgen_v6.h"

FlagDesc flagdesc_mapgen_v6[] = {
	{"jungles",    MGV6_JUNGLES},
	{"biomeblend", MGV6_BIOMEBLEND},
	{"mudflow",    MGV6_MUDFLOW},
	{NULL,         0}
};

///////////////////////////////////////////////////////////////////////////////


MapgenV6::MapgenV6(int mapgenid, MapgenParams *params, EmergeManager *emerge) {
	this->generating  = false;
	this->id       = mapgenid;
	this->emerge   = emerge;

	this->seed        = (int)params->seed;
	this->water_level = params->water_level;
	this->flags       = params->flags;
	this->csize       = v3s16(1, 1, 1) * params->chunksize * MAP_BLOCKSIZE;
	this->gennotify   = emerge->gennotify;

	this->ystride = csize.X; //////fix this
	
	MapgenV6Params *sp = (MapgenV6Params *)params->sparams;

	this->spflags     = sp->spflags;
	this->freq_desert = sp->freq_desert;
	this->freq_beach  = sp->freq_beach;

	np_cave        = &sp->np_cave;
	np_humidity    = &sp->np_humidity;
	np_trees       = &sp->np_trees;
	np_apple_trees = &sp->np_apple_trees;

	noise_terrain_base   = new Noise(&sp->np_terrain_base,   seed, csize.X, csize.Y);
	noise_terrain_higher = new Noise(&sp->np_terrain_higher, seed, csize.X, csize.Y);
	noise_steepness      = new Noise(&sp->np_steepness,      seed, csize.X, csize.Y);
	noise_height_select  = new Noise(&sp->np_height_select,  seed, csize.X, csize.Y);
	noise_mud            = new Noise(&sp->np_mud,            seed, csize.X, csize.Y);
	noise_beach          = new Noise(&sp->np_beach,          seed, csize.X, csize.Y);
	noise_biome          = new Noise(&sp->np_biome,          seed, csize.X, csize.Y);
}


MapgenV6::~MapgenV6() {
	delete noise_terrain_base;
	delete noise_terrain_higher;
	delete noise_steepness;
	delete noise_height_select;
	delete noise_mud;
	delete noise_beach;
	delete noise_biome;
}


MapgenV6Params::MapgenV6Params() {
	spflags     = MGV6_BIOMEBLEND | MGV6_MUDFLOW;
	freq_desert = 0.45;
	freq_beach  = 0.15;

	np_terrain_base   = NoiseParams(-4,  20.0, v3f(250.0, 250.0, 250.0), 82341,  5, 0.6);
	np_terrain_higher = NoiseParams(20,  16.0, v3f(500.0, 500.0, 500.0), 85039,  5, 0.6);
	np_steepness      = NoiseParams(0.85,0.5,  v3f(125.0, 125.0, 125.0), -932,   5, 0.7);
	np_height_select  = NoiseParams(0.5, 1.0,  v3f(250.0, 250.0, 250.0), 4213,   5, 0.69);
	np_mud            = NoiseParams(4,   2.0,  v3f(200.0, 200.0, 200.0), 91013,  3, 0.55);
	np_beach          = NoiseParams(0,   1.0,  v3f(250.0, 250.0, 250.0), 59420,  3, 0.50);
	np_biome          = NoiseParams(0,   1.0,  v3f(250.0, 250.0, 250.0), 9130,   3, 0.50);
	np_cave           = NoiseParams(6,   6.0,  v3f(250.0, 250.0, 250.0), 34329,  3, 0.50);
	np_humidity       = NoiseParams(0.5, 0.5,  v3f(500.0, 500.0, 500.0), 72384,  4, 0.66);
	np_trees          = NoiseParams(0,   1.0,  v3f(125.0, 125.0, 125.0), 2,      4, 0.66);
	np_apple_trees    = NoiseParams(0,   1.0,  v3f(100.0, 100.0, 100.0), 342902, 3, 0.45);
}


void MapgenV6Params::readParams(Settings *settings) {
	settings->getFlagStrNoEx("mgv6_spflags", spflags, flagdesc_mapgen_v6);
	settings->getFloatNoEx("mgv6_freq_desert", freq_desert);
	settings->getFloatNoEx("mgv6_freq_beach",  freq_beach);

	settings->getNoiseParams("mgv6_np_terrain_base",   np_terrain_base);
	settings->getNoiseParams("mgv6_np_terrain_higher", np_terrain_higher);
	settings->getNoiseParams("mgv6_np_steepness",      np_steepness);
	settings->getNoiseParams("mgv6_np_height_select",  np_height_select);
	settings->getNoiseParams("mgv6_np_mud",            np_mud);
	settings->getNoiseParams("mgv6_np_beach",          np_beach);
	settings->getNoiseParams("mgv6_np_biome",          np_biome);
	settings->getNoiseParams("mgv6_np_cave",           np_cave);
	settings->getNoiseParams("mgv6_np_humidity",       np_humidity);
	settings->getNoiseParams("mgv6_np_trees",          np_trees);
	settings->getNoiseParams("mgv6_np_apple_trees",    np_apple_trees);
}


void MapgenV6Params::writeParams(Settings *settings) {
	settings->setFlagStr("mgv6_spflags", spflags, flagdesc_mapgen_v6, (u32)-1);
	settings->setFloat("mgv6_freq_desert", freq_desert);
	settings->setFloat("mgv6_freq_beach",  freq_beach);

	settings->setNoiseParams("mgv6_np_terrain_base",   np_terrain_base);
	settings->setNoiseParams("mgv6_np_terrain_higher", np_terrain_higher);
	settings->setNoiseParams("mgv6_np_steepness",      np_steepness);
	settings->setNoiseParams("mgv6_np_height_select",  np_height_select);
	settings->setNoiseParams("mgv6_np_mud",            np_mud);
	settings->setNoiseParams("mgv6_np_beach",          np_beach);
	settings->setNoiseParams("mgv6_np_biome",          np_biome);
	settings->setNoiseParams("mgv6_np_cave",           np_cave);
	settings->setNoiseParams("mgv6_np_humidity",       np_humidity);
	settings->setNoiseParams("mgv6_np_trees",          np_trees);
	settings->setNoiseParams("mgv6_np_apple_trees",    np_apple_trees);
}


//////////////////////// Some helper functions for the map generator


// Returns Y one under area minimum if not found
s16 MapgenV6::find_stone_level(v2s16 p2d) {
	v3s16 em = vm->m_area.getExtent();
	s16 y_nodes_max = vm->m_area.MaxEdge.Y;
	s16 y_nodes_min = vm->m_area.MinEdge.Y;
	u32 i = vm->m_area.index(p2d.X, y_nodes_max, p2d.Y);
	s16 y;

	for (y = y_nodes_max; y >= y_nodes_min; y--) {
		MapNode &n = vm->m_data[i];
		content_t c = n.getContent();
		if (c != CONTENT_IGNORE && (
			c == c_stone || c == c_desert_stone))
			break;

		vm->m_area.add_y(em, i, -1);
	}
	return (y >= y_nodes_min) ? y : y_nodes_min - 1;
}


// Required by mapgen.h
bool MapgenV6::block_is_underground(u64 seed, v3s16 blockpos)
{
	/*s16 minimum_groundlevel = (s16)get_sector_minimum_ground_level(
			seed, v2s16(blockpos.X, blockpos.Z));*/
	// Nah, this is just a heuristic, just return something
	s16 minimum_groundlevel = water_level;

	if(blockpos.Y*MAP_BLOCKSIZE + MAP_BLOCKSIZE <= minimum_groundlevel)
		return true;
	else
		return false;
}


//////////////////////// Base terrain height functions

float MapgenV6::baseTerrainLevel(float terrain_base, float terrain_higher,
									float steepness, float height_select) {	
	float base   = 1 + terrain_base;
	float higher = 1 + terrain_higher;

	// Limit higher ground level to at least base
	if(higher < base)
		higher = base;

	// Steepness factor of cliffs
	float b = steepness;
	b = rangelim(b, 0.0, 1000.0);
	b = 5 * b * b * b * b * b * b * b;
	b = rangelim(b, 0.5, 1000.0);

	// Values 1.5...100 give quite horrible looking slopes
	if (b > 1.5 && b < 100.0)
		b = (b < 10.0) ? 1.5 : 100.0;

	float a_off = -0.20; // Offset to more low
	float a = 0.5 + b * (a_off + height_select);
	a = rangelim(a, 0.0, 1.0); // Limit
	
	return base * (1.0 - a) + higher * a;
}


float MapgenV6::baseTerrainLevelFromNoise(v2s16 p) {
	if (flags & MG_FLAT)
		return water_level;
		
	float terrain_base   = NoisePerlin2DPosOffset(noise_terrain_base->np,
							p.X, 0.5, p.Y, 0.5, seed);
	float terrain_higher = NoisePerlin2DPosOffset(noise_terrain_higher->np,
							p.X, 0.5, p.Y, 0.5, seed);
	float steepness      = NoisePerlin2DPosOffset(noise_steepness->np,
							p.X, 0.5, p.Y, 0.5, seed);
	float height_select  = NoisePerlin2DNoTxfmPosOffset(noise_height_select->np,
							p.X, 0.5, p.Y, 0.5, seed);

	return baseTerrainLevel(terrain_base, terrain_higher,
							steepness,    height_select);
}


float MapgenV6::baseTerrainLevelFromMap(v2s16 p) {
	int index = (p.Y - node_min.Z) * ystride + (p.X - node_min.X);
	return baseTerrainLevelFromMap(index);
}


float MapgenV6::baseTerrainLevelFromMap(int index) {
	if (flags & MG_FLAT)
		return water_level;
	
	float terrain_base   = noise_terrain_base->result[index];
	float terrain_higher = noise_terrain_higher->result[index];
	float steepness      = noise_steepness->result[index];
	float height_select  = noise_height_select->result[index];
	
	return baseTerrainLevel(terrain_base, terrain_higher,
							steepness,    height_select);
}


s16 MapgenV6::find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision) {
	return baseTerrainLevelFromNoise(p2d) + AVERAGE_MUD_AMOUNT;
}


int MapgenV6::getGroundLevelAtPoint(v2s16 p) {
	return baseTerrainLevelFromNoise(p) + AVERAGE_MUD_AMOUNT;
}


//////////////////////// Noise functions

float MapgenV6::getMudAmount(v2s16 p) {
	int index = (p.Y - node_min.Z) * ystride + (p.X - node_min.X);
	return getMudAmount(index);
}


bool MapgenV6::getHaveBeach(v2s16 p) {
	int index = (p.Y - node_min.Z) * ystride + (p.X - node_min.X);
	return getHaveBeach(index);
}


BiomeType MapgenV6::getBiome(v2s16 p) {
	int index = (p.Y - node_min.Z) * ystride + (p.X - node_min.X);
	return getBiome(index, p);
}


float MapgenV6::getHumidity(v2s16 p)
{
	/*double noise = noise2d_perlin(
		0.5+(float)p.X/500, 0.5+(float)p.Y/500,
		seed+72384, 4, 0.66);
	noise = (noise + 1.0)/2.0;*/

	float noise = NoisePerlin2D(np_humidity, p.X, p.Y, seed);

	if (noise < 0.0)
		noise = 0.0;
	if (noise > 1.0)
		noise = 1.0;
	return noise;
}


float MapgenV6::getTreeAmount(v2s16 p)
{
	/*double noise = noise2d_perlin(
			0.5+(float)p.X/125, 0.5+(float)p.Y/125,
			seed+2, 4, 0.66);*/
	
	float noise = NoisePerlin2D(np_trees, p.X, p.Y, seed);
	float zeroval = -0.39;
	if (noise < zeroval)
		return 0;
	else
		return 0.04 * (noise-zeroval) / (1.0-zeroval);
}


bool MapgenV6::getHaveAppleTree(v2s16 p)
{
	/*is_apple_tree = noise2d_perlin(
		0.5+(float)p.X/100, 0.5+(float)p.Z/100,
		data->seed+342902, 3, 0.45) > 0.2;*/
	
	float noise = NoisePerlin2D(np_apple_trees, p.X, p.Y, seed);
	
	return noise > 0.2;
}


float MapgenV6::getMudAmount(int index)
{
	if (flags & MG_FLAT)
		return AVERAGE_MUD_AMOUNT;
		
	/*return ((float)AVERAGE_MUD_AMOUNT + 2.0 * noise2d_perlin(
			0.5+(float)p.X/200, 0.5+(float)p.Y/200,
			seed+91013, 3, 0.55));*/
	
	return noise_mud->result[index];
}


bool MapgenV6::getHaveBeach(int index)
{
	// Determine whether to have sand here
	/*double sandnoise = noise2d_perlin(
			0.2+(float)p2d.X/250, 0.7+(float)p2d.Y/250,
			seed+59420, 3, 0.50);*/
	
	float sandnoise = noise_beach->result[index];
	return (sandnoise > freq_beach);
}


BiomeType MapgenV6::getBiome(int index, v2s16 p)
{
	// Just do something very simple as for now
	/*double d = noise2d_perlin(
			0.6+(float)p2d.X/250, 0.2+(float)p2d.Y/250,
			seed+9130, 3, 0.50);*/
	
	float d = noise_biome->result[index];
	if (d > freq_desert)
		return BT_DESERT;
		
	if ((spflags & MGV6_BIOMEBLEND) &&
		(d > freq_desert - 0.10) &&
		((noise2d(p.X, p.Y, seed) + 1.0) > (freq_desert - d) * 20.0))
		return BT_DESERT;
	
	return BT_NORMAL;
}


u32 MapgenV6::get_blockseed(u64 seed, v3s16 p)
{
	s32 x=p.X, y=p.Y, z=p.Z;
	return (u32)(seed%0x100000000ULL) + z*38134234 + y*42123 + x*23;
}


//////////////////////// Map generator

void MapgenV6::makeChunk(BlockMakeData *data) {
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
	
	// Hack: use minimum block coords for old code that assumes a single block
	v3s16 blockpos = data->blockpos_requested;
	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;

	// Area of central chunk
	node_min = blockpos_min*MAP_BLOCKSIZE;
	node_max = (blockpos_max+v3s16(1,1,1))*MAP_BLOCKSIZE-v3s16(1,1,1);

	// Full allocated area
	full_node_min = (blockpos_min-1)*MAP_BLOCKSIZE;
	full_node_max = (blockpos_max+2)*MAP_BLOCKSIZE-v3s16(1,1,1);

	central_area_size = node_max - node_min + v3s16(1,1,1);
	assert(central_area_size.X == central_area_size.Z);

	int volume_blocks = (blockpos_max.X - blockpos_min.X + 1)
					  * (blockpos_max.Y - blockpos_min.Y + 1)
					  * (blockpos_max.Z - blockpos_max.Z + 1);

	volume_nodes = volume_blocks *
		MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;

	// Create a block-specific seed
	blockseed = get_blockseed(data->seed, full_node_min);

	// Make some noise
	calculateNoise();

	c_stone           = ndef->getId("mapgen_stone");
	c_dirt            = ndef->getId("mapgen_dirt");
	c_dirt_with_grass = ndef->getId("mapgen_dirt_with_grass");
	c_sand            = ndef->getId("mapgen_sand");
	c_water_source    = ndef->getId("mapgen_water_source");
	c_lava_source     = ndef->getId("mapgen_lava_source");
	c_gravel          = ndef->getId("mapgen_gravel");
	c_cobble          = ndef->getId("mapgen_cobble");
	c_desert_sand     = ndef->getId("mapgen_desert_sand");
	c_desert_stone    = ndef->getId("mapgen_desert_stone");
	c_mossycobble     = ndef->getId("mapgen_mossycobble");
	c_sandbrick       = ndef->getId("mapgen_sandstonebrick");
	c_stair_cobble    = ndef->getId("mapgen_stair_cobble");
	c_stair_sandstone = ndef->getId("mapgen_stair_sandstone");
	if (c_desert_sand == CONTENT_IGNORE)
		c_desert_sand = c_sand;
	if (c_desert_stone == CONTENT_IGNORE)
		c_desert_stone = c_stone;
	if (c_mossycobble == CONTENT_IGNORE)
		c_mossycobble = c_cobble;
	if (c_sandbrick == CONTENT_IGNORE)
		c_sandbrick = c_desert_stone;
	if (c_stair_cobble == CONTENT_IGNORE)
		c_stair_cobble = c_cobble;
	if (c_stair_sandstone == CONTENT_IGNORE)
		c_stair_sandstone = c_sandbrick;

	// Maximum height of the stone surface and obstacles.
	// This is used to guide the cave generation
	s16 stone_surface_max_y;

	// Generate general ground level to full area
	stone_surface_max_y = generateGround();

	generateExperimental();

	const s16 max_spread_amount = MAP_BLOCKSIZE;
	// Limit dirt flow area by 1 because mud is flown into neighbors.
	s16 mudflow_minpos = -max_spread_amount + 1;
	s16 mudflow_maxpos = central_area_size.X + max_spread_amount - 2;

	// Loop this part, it will make stuff look older and newer nicely
	const u32 age_loops = 2;
	for (u32 i_age = 0; i_age < age_loops; i_age++) { // Aging loop
		// Make caves (this code is relatively horrible)
		if (flags & MG_CAVES)
			generateCaves(stone_surface_max_y);

		// Add mud to the central chunk
		addMud();

		// Add blobs of dirt and gravel underground
		addDirtGravelBlobs();

		// Flow mud away from steep edges
		if (spflags & MGV6_MUDFLOW)
			flowMud(mudflow_minpos, mudflow_maxpos);

	}
	
	// Add dungeons
	if (flags & MG_DUNGEONS) {
		DungeonParams dp;

		dp.np_rarity  = nparams_dungeon_rarity;
		dp.np_density = nparams_dungeon_density;
		dp.np_wetness = nparams_dungeon_wetness;
		dp.c_water = c_water_source;
		if (getBiome(0, v2s16(node_min.X, node_min.Z)) == BT_NORMAL) {
			dp.c_cobble  = c_cobble;
			dp.c_moss    = c_mossycobble;
			dp.c_stair   = c_stair_cobble;

			dp.diagonal_dirs = false;
			dp.mossratio  = 3.0;
			dp.holesize   = v3s16(1, 2, 1);
			dp.roomsize   = v3s16(0, 0, 0);
			dp.notifytype = GENNOTIFY_DUNGEON;
		} else {
			dp.c_cobble  = c_sandbrick;
			dp.c_moss    = c_sandbrick; // should make this 'cracked sandstone' later
			dp.c_stair   = c_stair_sandstone;

			dp.diagonal_dirs = true;
			dp.mossratio  = 0.0;
			dp.holesize   = v3s16(2, 3, 2);
			dp.roomsize   = v3s16(2, 5, 2);
			dp.notifytype = GENNOTIFY_TEMPLE;
		}

		DungeonGen dgen(this, &dp);
		dgen.generate(blockseed, full_node_min, full_node_max);
	}
	
	// Add top and bottom side of water to transforming_liquid queue
	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	// Grow grass
	growGrass();

	// Generate some trees, and add grass, if a jungle
	if (flags & MG_TREES)
		placeTreesAndJungleGrass();
	
	// Generate the registered decorations
	for (unsigned int i = 0; i != emerge->decorations.size(); i++) {
		Decoration *deco = emerge->decorations[i];
		deco->placeDeco(this, blockseed + i, node_min, node_max);
	}

	// Generate the registered ores
	for (unsigned int i = 0; i != emerge->ores.size(); i++) {
		Ore *ore = emerge->ores[i];
		ore->placeOre(this, blockseed + i, node_min, node_max);
	}

	// Calculate lighting
	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(1, 1, 1) * MAP_BLOCKSIZE,
					 node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE);
	
	this->generating = false;
}


void MapgenV6::calculateNoise() {
	int x = node_min.X;
	int z = node_min.Z;

	// Need to adjust for the original implementation's +.5 offset...
	if (!(flags & MG_FLAT)) {
		noise_terrain_base->perlinMap2D(
			x + 0.5 * noise_terrain_base->np->spread.X,
			z + 0.5 * noise_terrain_base->np->spread.Z);
		noise_terrain_base->transformNoiseMap();

		noise_terrain_higher->perlinMap2D(
			x + 0.5 * noise_terrain_higher->np->spread.X,
			z + 0.5 * noise_terrain_higher->np->spread.Z);
		noise_terrain_higher->transformNoiseMap();

		noise_steepness->perlinMap2D(
			x + 0.5 * noise_steepness->np->spread.X,
			z + 0.5 * noise_steepness->np->spread.Z);
		noise_steepness->transformNoiseMap();

		noise_height_select->perlinMap2D(
			x + 0.5 * noise_height_select->np->spread.X,
			z + 0.5 * noise_height_select->np->spread.Z);

		noise_mud->perlinMap2D(
			x + 0.5 * noise_mud->np->spread.X,
			z + 0.5 * noise_mud->np->spread.Z);
		noise_mud->transformNoiseMap();
	}

	noise_beach->perlinMap2D(
		x + 0.2 * noise_beach->np->spread.X,
		z + 0.7 * noise_beach->np->spread.Z);

	noise_biome->perlinMap2D(
		x + 0.6 * noise_biome->np->spread.X,
		z + 0.2 * noise_biome->np->spread.Z);
}


int MapgenV6::generateGround() {
	//TimeTaker timer1("Generating ground level");
	MapNode n_air(CONTENT_AIR), n_water_source(c_water_source);
	MapNode n_stone(c_stone), n_desert_stone(c_desert_stone);
	int stone_surface_max_y = -MAP_GENERATION_LIMIT;
	u32 index = 0;
	
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		// Surface height
		s16 surface_y = (s16)baseTerrainLevelFromMap(index);
		
		// Log it
		if (surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;

		BiomeType bt = getBiome(index, v2s16(x, z));
		
		// Fill ground with stone
		v3s16 em = vm->m_area.getExtent();
		u32 i = vm->m_area.index(x, node_min.Y, z);
		for (s16 y = node_min.Y; y <= node_max.Y; y++) {
			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (y <= surface_y) {
					vm->m_data[i] = (y > water_level && bt == BT_DESERT) ? 
						n_desert_stone : n_stone;
				} else if (y <= water_level) {
					vm->m_data[i] = n_water_source;
				} else {
					vm->m_data[i] = n_air;
				}
			}
			vm->m_area.add_y(em, i, 1);
		}
	}
	
	return stone_surface_max_y;
}


void MapgenV6::addMud() {
	// 15ms @cs=8
	//TimeTaker timer1("add mud");
	MapNode n_dirt(c_dirt), n_gravel(c_gravel);
	MapNode n_sand(c_sand), n_desert_sand(c_desert_sand);
	MapNode addnode;

	u32 index = 0;
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		// Randomize mud amount
		s16 mud_add_amount = getMudAmount(index) / 2.0 + 0.5;

		// Find ground level
		s16 surface_y = find_stone_level(v2s16(x, z)); /////////////////optimize this!
		
		// Handle area not found
		if (surface_y == vm->m_area.MinEdge.Y - 1)
			continue;
		
		BiomeType bt = getBiome(index, v2s16(x, z));
		addnode = (bt == BT_DESERT) ? n_desert_sand : n_dirt;

		if (bt == BT_DESERT && surface_y + mud_add_amount <= water_level + 1) {
			addnode = n_sand;
		} else if (mud_add_amount <= 0) {
			mud_add_amount = 1 - mud_add_amount;
			addnode = n_gravel;
		} else if (bt == BT_NORMAL && getHaveBeach(index) &&
				surface_y + mud_add_amount <= water_level + 2) {
			addnode = n_sand;
		}

		if (bt == BT_DESERT && surface_y > 20)
			mud_add_amount = MYMAX(0, mud_add_amount - (surface_y - 20) / 5);

		// If topmost node is grass, change it to mud.  It might be if it was
		// flown to there from a neighboring chunk and then converted.
		u32 i = vm->m_area.index(x, surface_y, z);
		if (vm->m_data[i].getContent() == c_dirt_with_grass)
			vm->m_data[i] = n_dirt;

		// Add mud on ground
		s16 mudcount = 0;
		v3s16 em = vm->m_area.getExtent();
		s16 y_start = surface_y + 1;
		i = vm->m_area.index(x, y_start, z);
		for (s16 y = y_start; y <= node_max.Y; y++) {
			if (mudcount >= mud_add_amount)
				break;

			vm->m_data[i] = addnode;
			mudcount++;

			vm->m_area.add_y(em, i, 1);
		}
	}
}


void MapgenV6::flowMud(s16 &mudflow_minpos, s16 &mudflow_maxpos) {
	// 340ms @cs=8
	TimeTaker timer1("flow mud");

	// Iterate a few times
	for(s16 k = 0; k < 3; k++) {
		for (s16 z = mudflow_minpos; z <= mudflow_maxpos; z++)
		for (s16 x = mudflow_minpos; x <= mudflow_maxpos; x++) {
			// Invert coordinates every 2nd iteration
			if (k % 2 == 0) {
				x = mudflow_maxpos - (x - mudflow_minpos);
				z = mudflow_maxpos - (z - mudflow_minpos);
			}

			// Node position in 2d
			v2s16 p2d = v2s16(node_min.X, node_min.Z) + v2s16(x, z);

			v3s16 em = vm->m_area.getExtent();
			u32 i = vm->m_area.index(p2d.X, node_max.Y, p2d.Y);
			s16 y = node_max.Y;

			while(y >= node_min.Y)
			{

			for(;; y--)
			{
				MapNode *n = NULL;
				// Find mud
				for(; y >= node_min.Y; y--) {
					n = &vm->m_data[i];
					if (n->getContent() == c_dirt ||
						n->getContent() == c_dirt_with_grass ||
						n->getContent() == c_gravel)
						break;

					vm->m_area.add_y(em, i, -1);
				}

				// Stop if out of area
				//if(vmanip.m_area.contains(i) == false)
				if (y < node_min.Y)
					break;

				if (n->getContent() == c_dirt ||
					n->getContent() == c_dirt_with_grass)
				{
					// Make it exactly mud
					n->setContent(c_dirt);

					// Don't flow it if the stuff under it is not mud
					{
						u32 i2 = i;
						vm->m_area.add_y(em, i2, -1);
						// Cancel if out of area
						if(vm->m_area.contains(i2) == false)
							continue;
						MapNode *n2 = &vm->m_data[i2];
						if (n2->getContent() != c_dirt &&
							n2->getContent() != c_dirt_with_grass)
							continue;
					}
				}

				v3s16 dirs4[4] = {
					v3s16(0,0,1), // back
					v3s16(1,0,0), // right
					v3s16(0,0,-1), // front
					v3s16(-1,0,0), // left
				};

				// Check that upper is air or doesn't exist.
				// Cancel dropping if upper keeps it in place
				u32 i3 = i;
				vm->m_area.add_y(em, i3, 1);
				if (vm->m_area.contains(i3) == true &&
					ndef->get(vm->m_data[i3]).walkable)
					continue;

				// Drop mud on side
				for(u32 di=0; di<4; di++) {
					v3s16 dirp = dirs4[di];
					u32 i2 = i;
					// Move to side
					vm->m_area.add_p(em, i2, dirp);
					// Fail if out of area
					if (vm->m_area.contains(i2) == false)
						continue;
					// Check that side is air
					MapNode *n2 = &vm->m_data[i2];
					if (ndef->get(*n2).walkable)
						continue;
					// Check that under side is air
					vm->m_area.add_y(em, i2, -1);
					if (vm->m_area.contains(i2) == false)
						continue;
					n2 = &vm->m_data[i2];
					if (ndef->get(*n2).walkable)
						continue;
					// Loop further down until not air
					bool dropped_to_unknown = false;
					do {
						vm->m_area.add_y(em, i2, -1);
						n2 = &vm->m_data[i2];
						// if out of known area
						if(vm->m_area.contains(i2) == false ||
							n2->getContent() == CONTENT_IGNORE) {
							dropped_to_unknown = true;
							break;
						}
					} while (ndef->get(*n2).walkable == false);
					// Loop one up so that we're in air
					vm->m_area.add_y(em, i2, 1);
					n2 = &vm->m_data[i2];

					bool old_is_water = (n->getContent() == c_water_source);
					// Move mud to new place
					if (!dropped_to_unknown) {
						*n2 = *n;
						// Set old place to be air (or water)
						if(old_is_water)
							*n = MapNode(c_water_source);
						else
							*n = MapNode(CONTENT_AIR);
					}

					// Done
					break;
				}
			}
			}
		}
	}
}


void MapgenV6::addDirtGravelBlobs() {
	if (getBiome(v2s16(node_min.X, node_min.Z)) != BT_NORMAL)
		return;
	
	PseudoRandom pr(blockseed + 983);
	for (int i = 0; i < volume_nodes/10/10/10; i++) {
		bool only_fill_cave = (myrand_range(0,1) != 0);
		v3s16 size(
			pr.range(1, 8),
			pr.range(1, 8),
			pr.range(1, 8)
		);
		v3s16 p0(
			pr.range(node_min.X, node_max.X) - size.X / 2,
			pr.range(node_min.Y, node_max.Y) - size.Y / 2,
			pr.range(node_min.Z, node_max.Z) - size.Z / 2
		);
		
		MapNode n1((p0.Y > -32 && !pr.range(0, 1)) ? c_dirt : c_gravel);
		for (int z1 = 0; z1 < size.Z; z1++)
		for (int y1 = 0; y1 < size.Y; y1++)
		for (int x1 = 0; x1 < size.X; x1++) {
			v3s16 p = p0 + v3s16(x1, y1, z1);
			u32 i = vm->m_area.index(p);
			if (!vm->m_area.contains(i))
				continue;
			// Cancel if not stone and not cave air
			if (vm->m_data[i].getContent() != c_stone &&
				!(vm->m_flags[i] & VMANIP_FLAG_CAVE))
				continue;
			if (only_fill_cave && !(vm->m_flags[i] & VMANIP_FLAG_CAVE))
				continue;
			vm->m_data[i] = n1;
		}
	}
}


void MapgenV6::placeTreesAndJungleGrass() {
	//TimeTaker t("placeTrees");
	if (node_max.Y < water_level)
		return;
	
	PseudoRandom grassrandom(blockseed + 53);
	content_t c_junglegrass = ndef->getId("mapgen_junglegrass");
	// if we don't have junglegrass, don't place cignore... that's bad
	if (c_junglegrass == CONTENT_IGNORE)
		c_junglegrass = CONTENT_AIR;
	MapNode n_junglegrass(c_junglegrass);
	v3s16 em = vm->m_area.getExtent();
	
	// Divide area into parts
	s16 div = 8;
	s16 sidelen = central_area_size.X / div;
	double area = sidelen * sidelen;
	
	// N.B.  We must add jungle grass first, since tree leaves will
	// obstruct the ground, giving us a false ground level
	for (s16 z0 = 0; z0 < div; z0++)
	for (s16 x0 = 0; x0 < div; x0++) {
		// Center position of part of division
		v2s16 p2d_center(
			node_min.X + sidelen / 2 + sidelen * x0,
			node_min.Z + sidelen / 2 + sidelen * z0
		);
		// Minimum edge of part of division
		v2s16 p2d_min(
			node_min.X + sidelen * x0,
			node_min.Z + sidelen * z0
		);
		// Maximum edge of part of division
		v2s16 p2d_max(
			node_min.X + sidelen + sidelen * x0 - 1,
			node_min.Z + sidelen + sidelen * z0 - 1
		);
		
		// Amount of trees, jungle area
		u32 tree_count = area * getTreeAmount(p2d_center);
		
		float humidity;
		bool is_jungle = false;
		if (spflags & MGV6_JUNGLES) {
			humidity = getHumidity(p2d_center);
			if (humidity > 0.75) {
				is_jungle = true;
				tree_count *= 4;
			}
		}

		// Add jungle grass
		if (is_jungle) {			
			u32 grass_count = 5 * humidity * tree_count;
			for (u32 i = 0; i < grass_count; i++) {
				s16 x = grassrandom.range(p2d_min.X, p2d_max.X);
				s16 z = grassrandom.range(p2d_min.Y, p2d_max.Y);
				
				s16 y = findGroundLevelFull(v2s16(x, z)); ////////////////optimize this!
				if (y < water_level || y < node_min.Y || y > node_max.Y)
					continue;
				
				u32 vi = vm->m_area.index(x, y, z);
				// place on dirt_with_grass, since we know it is exposed to sunlight
				if (vm->m_data[vi].getContent() == c_dirt_with_grass) {
					vm->m_area.add_y(em, vi, 1);
					vm->m_data[vi] = n_junglegrass;
				}
			}
		}
		
		// Put trees in random places on part of division
		for (u32 i = 0; i < tree_count; i++) {
			s16 x = myrand_range(p2d_min.X, p2d_max.X);
			s16 z = myrand_range(p2d_min.Y, p2d_max.Y);
			s16 y = findGroundLevelFull(v2s16(x, z)); ////////////////////optimize this!
			// Don't make a tree under water level
			// Don't make a tree so high that it doesn't fit
			if(y < water_level || y > node_max.Y - 6)
				continue;
			
			v3s16 p(x,y,z);
			// Trees grow only on mud and grass
			{
				u32 i = vm->m_area.index(p);
				MapNode *n = &vm->m_data[i];
				if (n->getContent() != c_dirt &&
					n->getContent() != c_dirt_with_grass)
					continue;
			}
			p.Y++;
			
			// Make a tree
			if (is_jungle) {
				treegen::make_jungletree(*vm, p, ndef, myrand());
			} else {
				bool is_apple_tree = (myrand_range(0, 3) == 0) &&
										getHaveAppleTree(v2s16(x, z));
				treegen::make_tree(*vm, p, is_apple_tree, ndef, myrand());
			}
		}
	}
	//printf("placeTreesAndJungleGrass: %dms\n", t.stop());
}


void MapgenV6::growGrass() {
	for (s16 z = full_node_min.Z; z <= full_node_max.Z; z++)
	for (s16 x = full_node_min.X; x <= full_node_max.X; x++) {
		// Find the lowest surface to which enough light ends up to make
		// grass grow.  Basically just wait until not air and not leaves.
		s16 surface_y = 0;
		{
			v3s16 em = vm->m_area.getExtent();
			u32 i = vm->m_area.index(x, node_max.Y, z);
			s16 y;
			// Go to ground level
			for (y = node_max.Y; y >= full_node_min.Y; y--) {
				MapNode &n = vm->m_data[i];
				if (ndef->get(n).param_type != CPT_LIGHT ||
					ndef->get(n).liquid_type != LIQUID_NONE)
					break;
				vm->m_area.add_y(em, i, -1);
			}
			surface_y = (y >= full_node_min.Y) ? y : full_node_min.Y;
		}

		u32 i = vm->m_area.index(x, surface_y, z);
		MapNode *n = &vm->m_data[i];
		if (n->getContent() == c_dirt && surface_y >= water_level - 20)
			n->setContent(c_dirt_with_grass);
	}
}


void MapgenV6::generateCaves(int max_stone_y) {
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
		CaveV6 cave(this, &ps, &ps2, large_cave);

		cave.makeCave(node_min, node_max, max_stone_y);
	}
}
