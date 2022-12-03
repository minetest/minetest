/*
Minetest
Copyright (C) 2015-2020 paramat
Copyright (C) 2015-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2021-2022 cartmic, Michael Carter

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
#include "nodedef.h"
#include "voxelalgorithms.h"
//#include "profiler.h" // For TimeTaker
#include "settings.h" // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_trailgen.h"


FlagDesc flagdesc_mapgen_trailgen[] = {
	{"caverns", MGTRAILGEN_CAVERNS},
	{NULL,    0}
};

///////////////////////////////////////////////////////////////////////////////////////


MapgenTrailgen::MapgenTrailgen(MapgenTrailgenParams *params, EmergeParams *emerge)
	: MapgenBasic(MAPGEN_TRAILGEN, params, emerge)
{
	ystride = csize.X;
	spflags				= params->spflags;
	cave_width			= params->cave_width;
	small_cave_num_min	= params->small_cave_num_min;
	small_cave_num_max	= params->small_cave_num_max;
	large_cave_num_min	= params->large_cave_num_min;
	large_cave_num_max	= params->large_cave_num_max;
	large_cave_depth		= params->large_cave_depth;
	large_cave_flooded		= params->large_cave_flooded;
	cavern_limit			= params->cavern_limit;
	cavern_taper 			= params->cavern_taper;
	cavern_threshold		= params->cavern_threshold;
	map_height_mod		= params->map_height_mod;
	dungeon_ymin		= params->dungeon_ymin;
	dungeon_ymax		= params->dungeon_ymax;

	// 2D noise
	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	noise_terrain = new Noise(&params->np_terrain, seed, csize.X, csize.Z);
	noise_terrain_higher = new Noise(&params->np_terrain_higher, seed, csize.X, csize.Z);
	noise_steepness = new Noise(&params->np_steepness, seed, csize.X, csize.Z);
	noise_height_select = new Noise(&params->np_height_select, seed, csize.X, csize.Z);

	// 3D noise
	MapgenBasic::np_cave1    = params->np_cave1;
	MapgenBasic::np_cave2    = params->np_cave2;
	MapgenBasic::np_cavern   = params->np_cavern;
	MapgenBasic::np_dungeons = params->np_dungeons;
}


MapgenTrailgen::~MapgenTrailgen()
{
	delete noise_filler_depth;

	delete noise_terrain;
	delete noise_terrain_higher;
	delete noise_steepness;
	delete noise_height_select;

}


MapgenTrailgenParams::MapgenTrailgenParams():
	np_terrain      (-4,   20.0, v3f(250.0, 250.0, 250.0), 82341,  5, 0.6,  2.0),
	np_terrain_higher (20,   16.0, v3f(500.0, 500.0, 500.0), 85039,  5, 0.6,  2.0),
	np_steepness      (0.85, 0.5,  v3f(125.0, 125.0, 125.0), -932,   5, 0.7,  2.0),
	np_height_select  (0,    1.0,  v3f(250.0, 250.0, 250.0), 4213,   5, 0.69, 2.0),
	np_filler_depth (1,   2,  v3f(200.0, 200.0, 200.0), 91013,  3, 0.7, 2.0),
	np_cavern       (0.0, 1.0, v3f(384, 128, 384), 723,   5, 0.63, 2.0),
	np_cave1        (0,   12,  v3f(61,  61,  61),  52534, 3, 0.5,  2.0),
	np_cave2        (0,   12,  v3f(67,  67,  67),  10325, 3, 0.5,  2.0),
	np_dungeons     (0.9, 0.5, v3f(500, 500, 500), 0,     2, 0.8,  2.0)
{
}


void MapgenTrailgenParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgtrailgen_spflags", spflags, flagdesc_mapgen_trailgen);
	settings->getS16NoEx("mgtrailgen_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgtrailgen_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgtrailgen_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgtrailgen_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgtrailgen_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgtrailgen_large_cave_flooded", large_cave_flooded);
	settings->getFloatNoEx("mgtrailgen_cave_width",         cave_width);
	settings->getS16NoEx("mgtrailgen_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgtrailgen_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgtrailgen_cavern_threshold",   cavern_threshold);
	settings->getU16NoEx("mgtrailgen_map_height_mod",   map_height_mod);
	settings->getS16NoEx("mgtrailgen_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgtrailgen_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgtrailgen_np_terrain", np_terrain);
	settings->getNoiseParams("mgtrailgen_np_terrain_higher", np_terrain_higher);
	settings->getNoiseParams("mgtrailgen_np_steepness", np_steepness);
	settings->getNoiseParams("mgtrailgen_np_height_select", np_height_select);
	settings->getNoiseParams("mgtrailgen_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgtrailgen_np_cavern",       np_cavern);
	settings->getNoiseParams("mgtrailgen_np_cave1",        np_cave1);
	settings->getNoiseParams("mgtrailgen_np_cave2",        np_cave2);
	settings->getNoiseParams("mgtrailgen_np_dungeons",     np_dungeons);
}


void MapgenTrailgenParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgtrailgen_spflags", spflags, flagdesc_mapgen_trailgen);
	settings->setS16("mgtrailgen_large_cave_depth",     large_cave_depth);
	settings->setU16("mgtrailgen_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgtrailgen_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgtrailgen_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgtrailgen_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgtrailgen_large_cave_flooded", large_cave_flooded);
	settings->setFloat("mgtrailgen_cave_width",         cave_width);
	settings->setS16("mgtrailgen_cavern_limit",         cavern_limit);
	settings->setS16("mgtrailgen_cavern_taper",         cavern_taper);
	settings->setFloat("mgtrailgen_cavern_threshold",   cavern_threshold);
	settings->setU16("mgtrailgen_map_height_mod",   map_height_mod);
	settings->setS16("mgtrailgen_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgtrailgen_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgtrailgen_np_terrain", np_terrain);
	settings->setNoiseParams("mgtrailgen_np_terrain_higher",      np_terrain_higher);
	settings->setNoiseParams("mgtrailgen_np_steepness", np_steepness);
	settings->setNoiseParams("mgtrailgen_np_height_select", np_height_select);
	settings->setNoiseParams("mgtrailgen_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgtrailgen_np_cavern",       np_cavern);
	settings->setNoiseParams("mgtrailgen_np_cave1",        np_cave1);
	settings->setNoiseParams("mgtrailgen_np_cave2",        np_cave2);
	settings->setNoiseParams("mgtrailgen_np_dungeons",     np_dungeons);
}


void MapgenTrailgenParams::setDefaultSettings(Settings *settings)
{
	settings->setDefault("mgtrailgen_spflags", flagdesc_mapgen_trailgen,
		MGTRAILGEN_CAVERNS);
}


/////////////////////////////////////////////////////////////////

void MapgenTrailgen::makeChunk(BlockMakeData *data)
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

	blockseed = getBlockSeed2(full_node_min, seed);

	// Make some noise
	calculateNoise();

	// Generate base terrain
	s16 stone_surface_max_y = generateTerrain();

	// Create heightmap
	updateHeightmap(node_min, node_max);

	// Init biome generator, place biome-specific nodes, and build biomemap
	if (flags & MG_BIOMES) {
		biomegen->calcBiomeNoise(node_min);
		generateBiomes();
	}

	// Generate tunnels, caverns and large randomwalk caves
	if (flags & MG_CAVES) {
		// Generate tunnels first as caverns confuse them
		generateCavesNoiseIntersection(stone_surface_max_y);

		// Generate caverns
		bool near_cavern = false;
		if (spflags & MGTRAILGEN_CAVERNS)
			near_cavern = generateCavernsNoise(stone_surface_max_y);

		// Generate large randomwalk caves
		if (near_cavern)
			// Disable large randomwalk caves in this mapchunk by setting
			// 'large cave depth' to world base. Avoids excessive liquid in
			// large caverns and floating blobs of overgenerated liquid.
			generateCavesRandomWalk(stone_surface_max_y,
				-MAX_MAP_GENERATION_LIMIT);
		else
			generateCavesRandomWalk(stone_surface_max_y, large_cave_depth);
	}

	// Generate the registered ores
	if (flags & MG_ORES)
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	if (flags & MG_DUNGEONS)
		generateDungeons(stone_surface_max_y);

	// Generate the registered decorations
	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	if (flags & MG_BIOMES)
		dustTopNodes();

	//printf("makeChunk: %dms\n", t.stop());

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);

	//setLighting(node_min - v3s16(1, 0, 1) * MAP_BLOCKSIZE,
	//			node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE, 0xFF);

	this->generating = false;
}

float MapgenTrailgen::baseTerrainLevel(float terrain, float terrain_higher, float steepness, float height_select)
{
	float base   = 1 + terrain;
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

void MapgenTrailgen::calculateNoise()
{
	int x = node_min.X;
	int z = node_min.Z;

	noise_terrain->perlinMap2D_PO(x, 0.5, z, 0.5);
	noise_terrain_higher->perlinMap2D_PO(x, 0.5, z, 0.5);
	noise_steepness->perlinMap2D_PO(x, 0.5, z, 0.5);
	noise_height_select->perlinMap2D_PO(x, 0.5, z, 0.5);

}

////

float MapgenTrailgen::baseTerrainLevelFromNoise(v2s16 p)
{

	float terrain   = NoisePerlin2D_PO(&noise_terrain->np,
							p.X, 0.5, p.Y, 0.5, seed);
	float terrain_higher = NoisePerlin2D_PO(&noise_terrain_higher->np,
							p.X, 0.5, p.Y, 0.5, seed);
	float steepness      = NoisePerlin2D_PO(&noise_steepness->np,
							p.X, 0.5, p.Y, 0.5, seed);
	float height_select  = NoisePerlin2D_PO(&noise_height_select->np,
							p.X, 0.5, p.Y, 0.5, seed);

	return baseTerrainLevel(terrain, terrain_higher,
							steepness, height_select);
}


float MapgenTrailgen::baseTerrainLevelFromMap(v2s16 p)
{
	int index = (p.Y - node_min.Z) * ystride + (p.X - node_min.X);
	return baseTerrainLevelFromMap(index);
}

float MapgenTrailgen::baseTerrainLevelFromMap(int index)
{
	float terrain  = noise_terrain->result[index];
	float terrain_higher = noise_terrain_higher->result[index];
	float steepness      = noise_steepness->result[index];
	float height_select  = noise_height_select->result[index];

	return baseTerrainLevel(terrain, terrain_higher,
							steepness, height_select);
}

int MapgenTrailgen::getGroundLevelAtPoint(v2s16 p)
{
	return baseTerrainLevelFromNoise(p) + 4;
}

int MapgenTrailgen::getSpawnLevelAtPoint(v2s16 p)
{

	s16 level_at_point = baseTerrainLevelFromNoise(p) + 4;
	if (level_at_point <= water_level ||
			level_at_point > water_level + 16)
		return MAX_MAP_GENERATION_LIMIT;  // Unsuitable spawn point

	return level_at_point;

}

////

s16 MapgenTrailgen::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);


	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;

	u32 index = 0;
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		s16 surface_y = (s16)baseTerrainLevelFromMap(index) + map_height_mod;

		if (surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;

		const v3s16 &em = vm->m_area.getExtent();
		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[vi].getContent() == CONTENT_IGNORE) {
				if (y <= surface_y) {
					vm->m_data[vi] = n_stone;
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				} else if (y <= water_level) {
					vm->m_data[vi] = n_water;
				} else {
					vm->m_data[vi] = n_air;
				}
			}
			VoxelArea::add_y(em, vi, 1);
		}
	}

	return stone_surface_max_y;
}


