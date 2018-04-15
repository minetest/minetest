/*
Minetest
Copyright (C) 2018 paramat

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


#include <cmath>
#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "content_sao.h"
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
#include "mapgen_planet.h"


////////////////////////////////////////////////////////////////////////////////


MapgenPlanet::MapgenPlanet(
		int mapgenid, MapgenPlanetParams *params, EmergeManager *emerge)
	: MapgenBasic(mapgenid, params, emerge)
{
	spflags          = params->spflags;
	cave_width       = params->cave_width;
	large_cave_depth = params->large_cave_depth;
	lava_depth       = params->lava_depth;
	dungeon_ymin     = params->dungeon_ymin;
	dungeon_ymax     = params->dungeon_ymax;

	//// 2D Terrain noise
	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	//// 3D terrain noise
	// 1 up 1 down overgeneration
	noise_terrain = new Noise(&params->np_terrain, seed, csize.X, csize.Y + 2, csize.Z);

	//// Cave noise
	MapgenBasic::np_cave1 = params->np_cave1;
	MapgenBasic::np_cave2 = params->np_cave2;
}


MapgenPlanet::~MapgenPlanet()
{
	delete noise_filler_depth;
	delete noise_terrain;
}


MapgenPlanetParams::MapgenPlanetParams():
	np_filler_depth (0,  1.2, v3f(128, 128, 128), 261,   3, 0.7,  2.0),
	np_terrain      (0,  1,   v3f(384, 192, 384), 1692,  5, 0.63, 2.0),
	np_cave1        (0,  12,  v3f(61,  61,  61),  52534, 3, 0.5,  2.0),
	np_cave2        (0,  12,  v3f(67,  67,  67),  10325, 3, 0.5,  2.0)
{
}


void MapgenPlanetParams::readParams(const Settings *settings)
{
	settings->getFloatNoEx("mgplanet_cave_width",       cave_width);
	settings->getS16NoEx("mgplanet_large_cave_depth",   large_cave_depth);
	settings->getS16NoEx("mgplanet_lava_depth",         lava_depth);
	settings->getS16NoEx("mgplanet_dungeon_ymin",       dungeon_ymin);
	settings->getS16NoEx("mgplanet_dungeon_ymax",       dungeon_ymax);

	settings->getNoiseParams("mgplanet_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgplanet_np_terrain",      np_terrain);
	settings->getNoiseParams("mgplanet_np_cave1",        np_cave1);
	settings->getNoiseParams("mgplanet_np_cave2",        np_cave2);
}


void MapgenPlanetParams::writeParams(Settings *settings) const
{
	settings->setFloat("mgplanet_cave_width",       cave_width);
	settings->setS16("mgplanet_large_cave_depth",   large_cave_depth);
	settings->setS16("mgplanet_lava_depth",         lava_depth);
	settings->setS16("mgplanet_dungeon_ymin",       dungeon_ymin);
	settings->setS16("mgplanet_dungeon_ymax",       dungeon_ymax);

	settings->setNoiseParams("mgplanet_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgplanet_np_terrain",      np_terrain);
	settings->setNoiseParams("mgplanetn_np_cave1",       np_cave1);
	settings->setNoiseParams("mgplanet_np_cave2",        np_cave2);
}


////////////////////////////////////////////////////////////////////////////////


void MapgenPlanet::makeChunk(BlockMakeData *data)
{
	// Pre-conditions
	assert(data->vmanip);
	assert(data->nodedef);
	assert(data->blockpos_requested.X >= data->blockpos_min.X &&
			data->blockpos_requested.Y >= data->blockpos_min.Y &&
			data->blockpos_requested.Z >= data->blockpos_min.Z);
	assert(data->blockpos_requested.X <= data->blockpos_max.X &&
			data->blockpos_requested.Y <= data->blockpos_max.Y &&
			data->blockpos_requested.Z <= data->blockpos_max.Z);

	this->generating = true;
	this->vm = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	// Create a block-specific seed
	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate terrain or air mapchunks
	bool planet = true;
	s16 stone_surface_max_y;

	if (node_min.X < -192 ||
			node_min.X > 128 ||
			node_min.Y < -192 ||
			node_min.Y > 128 ||
			node_min.Z < -192 ||
			node_min.Z > 128) {
		stone_surface_max_y = generateAir();
		planet = false;
	} else {
		stone_surface_max_y = generateTerrain();
	}

	// Create heightmap
	if (planet)
		updateHeightmap(node_min, node_max);

	// Init biome generator, place biome-specific nodes, and build biomemap
	biomegen->calcBiomeNoise(node_min);
	generateBiomes();

	if (planet) {
		// Generate caverns, tunnels and classic caves
		if (flags & MG_CAVES)
			generateCaves(stone_surface_max_y, large_cave_depth);

		// Generate dungeons
		if ((flags & MG_DUNGEONS) &&
				full_node_min.Y >= dungeon_ymin &&
				full_node_max.Y <= dungeon_ymax)
			generateDungeons(stone_surface_max_y);

		// Generate the registered decorations
		if (flags & MG_DECORATIONS)
			m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max,
				ndef);

		// Generate the registered ores
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

		// Sprinkle some dust on top after everything else was generated
		dustTopNodes();

		// Update liquids
		updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);
	}

	// Calculate lighting
	if (flags & MG_LIGHT) {
		// Avoid darkness on planet side
		if (!planet)
			setLighting(LIGHT_SUN,
				node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0));

		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
				full_node_min, full_node_max, planet);
	}

	this->generating = false;
}


////////////////////////////////////////////////////////////////////////////////


int MapgenPlanet::getSpawnLevelAtPoint(v2s16 p)
{
	if (p.X < -192 ||
			p.X > 207 ||
			p.Y < -192 ||
			p.Y > 207 )
		return MAX_MAP_GENERATION_LIMIT; // Unsuitable spawn point

	return 128;
}


////////////////////////////////////////////////////////////////////////////////


int MapgenPlanet::generateAir()
{
	MapNode mn_air(c_air);

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y; y <= node_max.Y; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++, vi++) {
			vm->m_data[vi] = mn_air;
		}
	}

	return -MAX_MAP_GENERATION_LIMIT;
}


int MapgenPlanet::generateTerrain()
{
	MapNode mn_air(c_air);
	MapNode mn_stone(c_stone);
	MapNode mn_water(c_water_source);

	// Calculate noise for terrain generation
	noise_terrain->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	//// Place nodes
	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 index3d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
		u32 vi = vm->m_area.index(node_min.X, y, z);
		for (s16 x = node_min.X; x <= node_max.X; x++, index3d++, vi++) {
			if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
				continue;

			float n_terrain = noise_terrain->result[index3d];
			float den_grad = (y >= 5) ?
				(float)(5 - y) / 96.0f :
				(float)(5 - y) / 32.0f;
			float density = n_terrain + den_grad;

			if (density > 0.0f) {
				vm->m_data[vi] = mn_stone; // Stone
				if (y > stone_surface_max_y)
					stone_surface_max_y = y;
			} else if (y <= water_level) {
				vm->m_data[vi] = mn_water; // Sea water
			} else {
				vm->m_data[vi] = mn_air; // Air
			}
		}
	}

	return stone_surface_max_y;
}
