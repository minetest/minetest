/*
Minetest
Copyright (C) 2010-2018 paramat
Copyright (C) 2010-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "cavegen.h"
#include "mapgen_watershed.h"


FlagDesc flagdesc_mapgen_watershed[] = {
	{"volcanos",   MGWATERSHED_VOLCANOS},
	{NULL      , 0}
};


///////////////////////////////////////////////////////////////////////////////


MapgenWatershed::MapgenWatershed(int mapgenid, MapgenWatershedParams *params,
	EmergeManager *emerge)
	: MapgenBasic(mapgenid, params, emerge)
{
	spflags        = params->spflags;
	div            = params->map_scale;
	vertical_scale = params->vertical_scale / div;
	coast_level    = params->coast_level;
	river_width    = params->river_width;
	river_depth    = params->river_depth;
	river_bank     = params->river_bank;

	cave_width       = params->cave_width;
	large_cave_depth = params->large_cave_depth;
	lava_depth       = params->lava_depth;
	cavern_limit     = params->cavern_limit;
	cavern_taper     = params->cavern_taper;
	cavern_threshold = params->cavern_threshold;

	np_vent_div        = params->np_vent;
	np_base_div        = params->np_base;
	np_valley_div      = params->np_valley;
	np_amplitude_div   = params->np_amplitude;
	np_blend_div       = params->np_blend;
	np_rough_2d_div    = params->np_rough_2d;
	np_rough_3d_div    = params->np_rough_3d;
	np_rough_blend_div = params->np_rough_blend;
	np_highland_div    = params->np_highland;
	np_select_div      = params->np_select;

	if (div != 1.0f) {
		np_vent_div.spread        /= v3f(div, div, div);
		np_base_div.spread        /= v3f(div, div, div);
		np_valley_div.spread      /= v3f(div, div, div);
		np_amplitude_div.spread   /= v3f(div, div, div);
		np_blend_div.spread       /= v3f(div, div, div);
		np_rough_2d_div.spread    /= v3f(div, div, div);
		np_rough_3d_div.spread    /= v3f(div, div, div);
		np_rough_blend_div.spread /= v3f(div, div, div);
		np_highland_div.spread    /= v3f(div, div, div);
		np_select_div.spread      /= v3f(div, div, div);
	}

	//// 2D Terrain noise
	noise_vent        = new Noise(&np_vent_div,        seed, csize.X, csize.Z);
	noise_base        = new Noise(&np_base_div,        seed, csize.X, csize.Z);
	noise_valley      = new Noise(&np_valley_div,      seed, csize.X, csize.Z);
	noise_amplitude   = new Noise(&np_amplitude_div,   seed, csize.X, csize.Z);
	noise_blend       = new Noise(&np_blend_div,       seed, csize.X, csize.Z);
	noise_rough_2d    = new Noise(&np_rough_2d_div,    seed, csize.X, csize.Z);
	noise_rough_blend = new Noise(&np_rough_blend_div, seed, csize.X, csize.Z);
	noise_highland    = new Noise(&np_highland_div,    seed, csize.X, csize.Z);
	noise_select      = new Noise(&np_select_div,      seed, csize.X, csize.Z);

	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	//// 3D terrain noise
	// 1 up 1 down overgeneration
	noise_rough_3d =
		new Noise(&np_rough_3d_div, seed, csize.X, csize.Y + 2, csize.Z);
	// 1 down overgeneration
	MapgenBasic::np_cave1 = params->np_cave1;
	MapgenBasic::np_cave2 = params->np_cave2;
	MapgenBasic::np_cavern = params->np_cavern;

	//// Resolve nodes to be used
	c_volcanic_rock        = ndef->getId("mapgen_volcanic_rock");
	c_obsidianbrick        = ndef->getId("mapgen_obsidianbrick");
	c_stair_obsidian_block = ndef->getId("mapgen_stair_obsidian_block");

	if (c_volcanic_rock == CONTENT_IGNORE)
		c_volcanic_rock = c_stone;
	if (c_obsidianbrick == CONTENT_IGNORE)
		c_obsidianbrick = c_cobble;
	if (c_stair_obsidian_block == CONTENT_IGNORE)
		c_stair_obsidian_block = c_obsidianbrick;
}


MapgenWatershed::~MapgenWatershed()
{
	delete noise_vent;
	delete noise_base;
	delete noise_valley;
	delete noise_amplitude;
	delete noise_blend;
	delete noise_rough_2d;
	delete noise_rough_3d;
	delete noise_rough_blend;
	delete noise_highland;
	delete noise_select;

	delete noise_filler_depth;
}


MapgenWatershedParams::MapgenWatershedParams()
{
	spflags        = MGWATERSHED_VOLCANOS;
	map_scale      = 1.0;
	vertical_scale = 128.0;
	coast_level    = 6;
	river_width    = 0.07;
	river_depth    = 0.25;
	river_bank     = 0.009;

	cave_width       = 0.09;
	large_cave_depth = -33;
	lava_depth       = -256;
	cavern_limit     = -256;
	cavern_taper     = 256;
	cavern_threshold = 0.7;

	np_vent        = NoiseParams(-1.0, 1.08, v3f(48,   48,   48),   692,    1, 0.5,  2.0);
	np_base        = NoiseParams(0.0,  1.0,  v3f(2048, 2048, 2048), 106,    3, 0.4,  2.0);
	np_valley      = NoiseParams(0.0,  1.0,  v3f(1024, 1024, 1024), 2177,   5, 0.5,  2.0);
	np_amplitude   = NoiseParams(1.0,  2.0,  v3f(1024, 1024, 1024), 91206,  3, 0.4,  2.0);
	np_blend       = NoiseParams(0.5,  2.0,  v3f(1024, 1024, 1024), 3120,   3, 0.4,  2.0);
	np_rough_2d    = NoiseParams(0.0,  1.0,  v3f(384,  384,  384),  500012, 5, 0.63, 2.0);
	np_rough_3d    = NoiseParams(0.0,  1.0,  v3f(384,  384,  384),  70033,  5, 0.63, 2.0);
	np_rough_blend = NoiseParams(0.5,  0.5,  v3f(1024, 1024, 1024), 899007, 1, 0.5,  2.0);
	np_highland    = NoiseParams(0.6,  0.2,  v3f(1024, 1024, 1024), 8111,   3, 0.4,  2.0);
	np_select      = NoiseParams(-0.4, 1.0,  v3f(2048, 2048, 2048), 300899, 8, 0.7,  2.0);

	np_filler_depth = NoiseParams(0.0,  1.0,  v3f(128,  128,  128),  261,   3, 0.7,  2.0);
	np_cave1        = NoiseParams(0.0,  12.0, v3f(61,   61,   61),   52534, 3, 0.5,  2.0);
	np_cave2        = NoiseParams(0.0,  12.0, v3f(67,   67,   67),   10325, 3, 0.5,  2.0);
	np_cavern       = NoiseParams(0.0,  1.0,  v3f(384,  128,  384),  723,   5, 0.63, 2.0);
}


void MapgenWatershedParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed);
	settings->getFloatNoEx("mgwatershed_map_scale",      map_scale);
	settings->getFloatNoEx("mgwatershed_vertical_scale", vertical_scale);
	settings->getS16NoEx("mgwatershed_coast_level",      coast_level);
	settings->getFloatNoEx("mgwatershed_river_width",    river_width);
	settings->getFloatNoEx("mgwatershed_river_depth",    river_depth);
	settings->getFloatNoEx("mgwatershed_river_bank",     river_bank);

	settings->getFloatNoEx("mgwatershed_cave_width",       cave_width);
	settings->getS16NoEx("mgwatershed_large_cave_depth",   large_cave_depth);
	settings->getS16NoEx("mgwatershed_lava_depth",         lava_depth);
	settings->getS16NoEx("mgwatershed_cavern_limit",       cavern_limit);
	settings->getS16NoEx("mgwatershed_cavern_taper",       cavern_taper);
	settings->getFloatNoEx("mgwatershed_cavern_threshold", cavern_threshold);

	settings->getNoiseParams("mgwatershed_np_vent",        np_vent);
	settings->getNoiseParams("mgwatershed_np_base",        np_base);
	settings->getNoiseParams("mgwatershed_np_valley",      np_valley);
	settings->getNoiseParams("mgwatershed_np_amplitude",   np_amplitude);
	settings->getNoiseParams("mgwatershed_np_blend",       np_blend);
	settings->getNoiseParams("mgwatershed_np_rough_2d",    np_rough_2d);
	settings->getNoiseParams("mgwatershed_np_rough_3d",    np_rough_3d);
	settings->getNoiseParams("mgwatershed_np_rough_blend", np_rough_blend);
	settings->getNoiseParams("mgwatershed_np_highland",    np_highland);
	settings->getNoiseParams("mgwatershed_np_select",      np_select);

	settings->getNoiseParams("mgwatershed_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->getNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->getNoiseParams("mgwatershed_np_cavern",       np_cavern);
}


void MapgenWatershedParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed, U32_MAX);
	settings->setFloat("mgwatershed_map_scale",      map_scale);
	settings->setFloat("mgwatershed_vertical_scale", vertical_scale);
	settings->setS16("mgwatershed_coast_level",      coast_level);
	settings->setFloat("mgwatershed_river_width",    river_width);
	settings->setFloat("mgwatershed_river_depth",    river_depth);
	settings->setFloat("mgwatershed_river_bank",     river_bank);

	settings->setFloat("mgwatershed_cave_width",       cave_width);
	settings->setS16("mgwatershed_large_cave_depth",   large_cave_depth);
	settings->setS16("mgwatershed_lava_depth",         lava_depth);
	settings->setS16("mgwatershed_cavern_limit",       cavern_limit);
	settings->setS16("mgwatershed_cavern_taper",       cavern_taper);
	settings->setFloat("mgwatershed_cavern_threshold", cavern_threshold);

	settings->setNoiseParams("mgwatershed_np_vent",        np_vent);
	settings->setNoiseParams("mgwatershed_np_base",        np_base);
	settings->setNoiseParams("mgwatershed_np_valley",      np_valley);
	settings->setNoiseParams("mgwatershed_np_amplitude",   np_amplitude);
	settings->setNoiseParams("mgwatershed_np_blend",       np_blend);
	settings->setNoiseParams("mgwatershed_np_rough_2d",    np_rough_2d);
	settings->setNoiseParams("mgwatershed_np_rough_3d",    np_rough_3d);
	settings->setNoiseParams("mgwatershed_np_rough_blend", np_rough_blend);
	settings->setNoiseParams("mgwatershed_np_highland",    np_highland);
	settings->setNoiseParams("mgwatershed_np_select",      np_select);

	settings->setNoiseParams("mgwatershed_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->setNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->setNoiseParams("mgwatershed_np_cavern",       np_cavern);
}


///////////////////////////////////////////////////////////////////////////////


int MapgenWatershed::getSpawnLevelAtPoint(v2s16 p)
{
	float den_grad_coast = (1.0f - coast_level) / vertical_scale;

	float n_vbase = NoisePerlin2D(&noise_base->np, p.X, p.Y, seed);
	float n_vbase_shaped;
	if (n_vbase < den_grad_coast) {
		n_vbase_shaped = den_grad_coast - (den_grad_coast - n_vbase) * 0.5f;
	} else {
		n_vbase_shaped = den_grad_coast +
			std::pow((n_vbase - den_grad_coast), 1.5f) * 0.5f;
	}

	// Check if in a river
	float n_valley = NoisePerlin2D(&noise_valley->np, p.X, p.Y, seed);
	float n_valley_abs = std::fabs(n_valley);
	float n_valley_sunk = n_valley_abs - (0.8f - n_vbase_shaped) * river_width;
	if (n_valley_sunk < 0.0f)
		return MAX_MAP_GENERATION_LIMIT; // Unsuitable spawn point

	float n_amplitude = NoisePerlin2D(&noise_amplitude->np, p.X, p.Y, seed);
	float n_amplitude_lim = std::fmax(n_amplitude, 0.0f) * 0.3f;
	float n_valley_shaped = std::pow(n_valley_sunk, 1.5f) * n_amplitude_lim;
	float n_blend = NoisePerlin2D(&noise_blend->np, p.X, p.Y, seed);
	float n_blend_lim = rangelim(n_blend, 0.0f, 1.0f);
	float n_highland_shaped = NoisePerlin2D(&noise_highland->np, p.X, p.Y, seed);
	float n_select = std::fmin(NoisePerlin2D(&noise_select->np, p.X, p.Y, seed),
		n_valley_sunk * 5.0f) + std::fmin(n_vbase, 0.0f);
	float n_select_blend = (n_select > 0) ?
		std::fmin(n_select * n_select * n_select * n_select * 50.0f, 1.0f) :
		0.0f;

	float n_vent = (n_valley > 0.0f) ?
		NoisePerlin2D(&noise_vent->np, p.X, p.Y, seed) : -1.0f;
	float vbase_mod = std::fmax(-n_vbase, 0.0f);
	float blend_mod = 1.0f - n_blend_lim;
	float valley_shaped_mod = std::fmax(0.5f - n_valley_shaped, 0.0f);
	float mod = vbase_mod + blend_mod + valley_shaped_mod;
	float n_vent_shaped = n_vent - mod * mod;

	// Check if in a volcano
	if (n_vent_shaped > 0.0f)
		return MAX_MAP_GENERATION_LIMIT; // Unsuitable spawn point

	float n_rough_2d = NoisePerlin2D(&noise_rough_2d->np, p.X, p.Y, seed);
	float n_rough_blend =
		rangelim(NoisePerlin2D(&noise_rough_blend->np, p.X, p.Y, seed), 0.0f, 1.0f);

	s16 search_min = water_level;
	s16 search_max = water_level + 16;
	bool solid_below = false; // Solid node is present below to spawn on
	u8 air_count = 0; // Consecutive air nodes above the solid node

	for (s16 y = search_min; y <= search_max; y++) {
		float n_rough_3d = NoisePerlin3D(&noise_rough_3d->np, p.X, y, p.Y, seed);
		float n_rough_blended = (1.0f - n_rough_blend) * n_rough_2d +
			n_rough_blend * n_rough_3d;
		float n_rough_shaped = (n_rough_blended + 1.0f) * n_valley_shaped * 4.0f;
		float n_lowland_blend = n_vbase_shaped + 
			(1.0f - n_blend_lim) * n_valley_shaped +
			n_blend_lim * n_rough_shaped;
		float n_highland_shaped_lim =
			std::fmax(n_highland_shaped, n_lowland_blend);
		float n_terrain = (1.0f - n_select_blend) * n_lowland_blend +
			n_select_blend * n_highland_shaped_lim;
		float density_grad = (1.0f - y) / vertical_scale;
		float density = n_terrain + density_grad;

		if (density >= 0.0f) { // Solid node
			solid_below = true;
			air_count = 0;
		} else if (solid_below) { // Air above solid node
			air_count++;
			// 3 to account for snowblock dust
			if (air_count == 3)
				return y - 2;
		}
	}

	return MAX_MAP_GENERATION_LIMIT; // No suitable spawn point found
}


void MapgenWatershed::makeChunk(BlockMakeData *data)
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

	//TimeTaker t("makeChunk");

	this->generating = true;
	this->vm         = data->vmanip;
	this->ndef       = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate terrain
	s16 stone_surface_max_y;
	bool volcanic_placed;
	generateTerrain(&stone_surface_max_y, &volcanic_placed);

	if (div == 1.0f) {
		// Create heightmap
		updateHeightmap(node_min, node_max);

		// Init biome generator, place biome-specific nodes, and build biomemap
		biomegen->calcBiomeNoise(node_min);

		MgStoneType mgstone_type;
		content_t biome_stone;
		generateBiomes(&mgstone_type, &biome_stone);

		// Generate caverns, tunnels and classic caves
		if (flags & MG_CAVES) {
			// Generate caverns
			bool has_cavern = generateCaverns(stone_surface_max_y);
			// Generate tunnels and classic caves
			if (has_cavern)
				// Disable randomwalk large caves in this mapchunk by setting
				// 'large cave depth' to world base. Avoids excessive liquid in
				// large caverns and floating blobs of overgenerated liquid.
				generateCaves(stone_surface_max_y, -MAX_MAP_GENERATION_LIMIT);
			else
				generateCaves(stone_surface_max_y, large_cave_depth);
		}

		// Generate dungeons
		if (flags & MG_DUNGEONS) {
			if (volcanic_placed) {
				// Obsidian dungeons
				DungeonParams dp;

				dp.seed             = seed;
				dp.c_water          = c_water_source;
				dp.c_river_water    = c_river_water_source;
				dp.only_in_ground   = true;
				dp.corridor_len_min = 1;
				dp.corridor_len_max = 13;
				dp.rooms_min        = 16;
				dp.rooms_max        = 32;
				dp.y_min            = -MAX_MAP_GENERATION_LIMIT;
				dp.y_max            = MAX_MAP_GENERATION_LIMIT;
				dp.np_density       = nparams_dungeon_density;
				dp.np_alt_wall      = nparams_dungeon_alt_wall;

				dp.c_wall     = c_obsidianbrick;
				dp.c_alt_wall = CONTENT_IGNORE;
				dp.c_stair    = c_stair_obsidian_block;

				dp.diagonal_dirs       = false;
				dp.holesize            = v3s16(3,  2, 3);
				dp.room_size_min       = v3s16(8,  4, 8);
				dp.room_size_max       = v3s16(12, 6, 12);
				dp.room_size_large_min = v3s16(12, 6, 12);
				dp.room_size_large_max = v3s16(18, 9, 18);
				dp.notifytype          = GENNOTIFY_DUNGEON;
			
				DungeonGen dgen(ndef, &gennotify, &dp);
				dgen.generate(vm, blockseed, full_node_min, full_node_max);
			} else {
				// Standard dungeons
				generateDungeons(stone_surface_max_y, mgstone_type, biome_stone);
			}
		}

		// Generate the registered decorations
		if (flags & MG_DECORATIONS)
			m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

		// Generate the registered ores
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

		// Sprinkle some dust on top after everything else was generated
		dustTopNodes();
	}

	// Update liquids
	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	// Calculate lighting
	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max, true);

	this->generating = false;

	//printf("makeChunk: %dms\n", t.stop());
}


void MapgenWatershed::generateTerrain(s16 *stone_surface_max_y, bool *volcanic_placed)
{
	MapNode mn_air(CONTENT_AIR);
	MapNode mn_stone(c_stone);
	MapNode mn_water(c_water_source);
	MapNode mn_river_water(c_river_water_source);
	MapNode mn_magma(c_lava_source);
	MapNode mn_volcanic(c_volcanic_rock);

	// Calculate noise
	noise_vent       ->perlinMap2D(node_min.X, node_min.Z);
	noise_base       ->perlinMap2D(node_min.X, node_min.Z);
	noise_valley     ->perlinMap2D(node_min.X, node_min.Z);
	noise_amplitude  ->perlinMap2D(node_min.X, node_min.Z);
	noise_blend      ->perlinMap2D(node_min.X, node_min.Z);
	noise_rough_2d   ->perlinMap2D(node_min.X, node_min.Z);
	noise_rough_blend->perlinMap2D(node_min.X, node_min.Z);
	noise_highland   ->perlinMap2D(node_min.X, node_min.Z);
	noise_select     ->perlinMap2D(node_min.X, node_min.Z);

	noise_rough_3d ->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	// Cache density_gradient values
	float density_grad_cache[csize.Y + 2];
	u8 density_grad_index = 0; // Index zero at column base
	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++, density_grad_index++) {
		density_grad_cache[density_grad_index] = (1.0f - y) / vertical_scale;
	}

	// Calculate density gradient value of coast level
	float den_grad_coast = (1.0f - coast_level) / vertical_scale;

	//// Place nodes
	int stone_max_y = -MAX_MAP_GENERATION_LIMIT;
	bool volcanic = false;
	const v3s16 &em = vm->m_area.getExtent();
	u32 index2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index2d++) {
		// Valley base / riverbank level
		float n_vbase = noise_base->result[index2d];
		float n_vbase_shaped;
		if (n_vbase < den_grad_coast) {
			// Set ocean depth
			n_vbase_shaped = den_grad_coast - (den_grad_coast - n_vbase) * 0.5f;
		} else {
			// Create flatter coastal areas and steeper mountain areas
			// Set valley base height
			n_vbase_shaped = den_grad_coast +
				std::pow((n_vbase - den_grad_coast), 1.5f) * 0.5f;
		}

		// River valley
		float n_valley = noise_valley->result[index2d];
		float n_valley_abs = std::fabs(n_valley);
		// Valley cross section sinks a little below 0 to define river area and width
		// Set altitude of river source
		float n_valley_sunk = n_valley_abs - (0.8f - n_vbase_shaped) * river_width;

		// Shape river channel and terrain between rivers
		float n_valley_shaped;
		float n_blend_lim = 0.0f;  // Initialise these for river channel
		float n_highland_shaped = 0.0f;
		float n_select_blend = 0.0f;
		bool between_rivers = true;

		if (n_valley_sunk >= 0.0f) { // Between rivers
			// Smooth lowland amplitude, lower limit 0
			float n_amplitude = noise_amplitude->result[index2d];
			// Set smooth lowland amplitude here
			float n_amplitude_lim = std::fmax(n_amplitude, 0.0f) * 0.3f;
			n_valley_shaped = std::pow(n_valley_sunk, 1.5f) * n_amplitude_lim;
			// Noise for blending smooth and rough lowland, limited to 0 - 1
			float n_blend = noise_blend->result[index2d];
			n_blend_lim = rangelim(n_blend, 0.0f, 1.0f);
			// Highland
			n_highland_shaped = noise_highland->result[index2d];
			// Noise for selecting lowland and highland
			// Reduced either side of rivers and reduced in oceans
			float n_select = std::fmin(noise_select->result[index2d],
				n_valley_sunk * 5.0f) + std::fmin(n_vbase, 0.0f);
			n_select_blend = (n_select > 0) ?
				std::fmin(n_select * n_select * n_select * n_select * 50.0f, 1.0f) :
				0.0f;
		} else { // River channel
			between_rivers = false;
			float river_depth_shaped = (n_vbase_shaped < -0.0f) ?
				std::fmax(river_depth + n_vbase_shaped * 4.0f, 0.0f) :
				river_depth;
			n_valley_shaped = -sqrt(-n_valley_sunk) * river_depth_shaped;
		}

		// Magma vents
		// Vent areas decided by sign of n_valley
		float n_vent = (n_valley > 0.0f) ? noise_vent->result[index2d] : -1.0f;
		float vbase_mod = std::fmax(-n_vbase, 0.0f);
		float blend_mod = 1.0f - n_blend_lim;
		float valley_shaped_mod = std::fmax(0.5f - n_valley_shaped, 0.0f);
		float mod = vbase_mod + blend_mod + valley_shaped_mod;
		// Restrict to higher, rougher land away from rivers
		float n_vent_shaped = n_vent - mod * mod;
		
		// rough_2d and rough_blend noises
		float n_rough_2d = noise_rough_2d->result[index2d];
		float n_rough_blend =
			rangelim(noise_rough_blend->result[index2d], 0.0f, 1.0f);

		// Y loop
		// Indexes at column base
		density_grad_index = 0;
		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);
		u32 index3d = (z - node_min.Z) * zstride_1u1d + (x - node_min.X);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1;
				y++,
				vm->m_area.add_y(em, vi, 1),
				index3d += ystride,
				density_grad_index++) {
			if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
				continue;

			float n_rough_3d = 0.0f; // Initialise these for river channel
			float n_rough_shaped = 0.0f;
			float n_terrain = n_vbase_shaped + n_valley_shaped;

			if (between_rivers) {
				// 3D noise
				n_rough_3d = noise_rough_3d->result[index3d];
				// Blended rough terrain
				float n_rough_blended = (1.0f - n_rough_blend) * n_rough_2d +
					n_rough_blend * n_rough_3d;
				// Set blended rough terrain amplitude here
				n_rough_shaped = (n_rough_blended + 1.0f) * n_valley_shaped * 4.0f;
				// Blended lowland terrain
				float n_lowland_blend = n_vbase_shaped +
					(1.0f - n_blend_lim) * n_valley_shaped +
					n_blend_lim * n_rough_shaped;
				// Lowland and highland terrain selected
				float n_highland_shaped_lim =
					std::fmax(n_highland_shaped, n_lowland_blend);
				n_terrain = (1.0f - n_select_blend) * n_lowland_blend +
					n_select_blend * n_highland_shaped_lim;
			}

			// Density gradient
			float density_grad = density_grad_cache[density_grad_index];
			// Valley base / riverbank level
			float density_base = n_vbase_shaped + density_grad;
			// Terrain level
			float density = n_terrain + density_grad;

			if (density >= 0.0f) {
				// Maximum vent wall thickness
				float vent_wall = 0.05f + std::fabs(n_rough_3d) * 0.05f;

				if ((spflags & MGWATERSHED_VOLCANOS) &&
						n_vent_shaped >= -vent_wall) { // Vent
					if (n_vent_shaped > 0.0f) { // Magma channel
						if (density_base >= 0.0f)
							vm->m_data[vi] = mn_magma; // Magma
						else
							vm->m_data[vi] = mn_air; // Magma channel air
					} else { // Vent wall and taper
						float cone = (n_vent_shaped + vent_wall) / vent_wall * 0.2f;
						if (density >= cone) {
							vm->m_data[vi] = mn_volcanic; // Vent wall
							volcanic = true;
						} else {
							vm->m_data[vi] = mn_air; // Taper air
						}
					}
				} else {
					vm->m_data[vi] = mn_stone; // Stone
					if (y > stone_max_y)
						stone_max_y = y;
				}
			} else if (y <= water_level) {
				vm->m_data[vi] = mn_water; // Sea water
			} else if (density_base >= river_bank) {
				vm->m_data[vi] = mn_river_water; // River water
			} else {
				vm->m_data[vi] = mn_air; // Air
			}
		}
	}

	*stone_surface_max_y = stone_max_y;
	*volcanic_placed = volcanic;
}
