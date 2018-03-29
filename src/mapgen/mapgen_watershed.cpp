/*
Minetest
Copyright (C) 2020 paramat

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
#include "nodedef.h"
#include "voxelalgorithms.h"
//#include "profiler.h" // For TimeTaker only
#include "settings.h" // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "cavegen.h"
#include "mapgen_watershed.h"


FlagDesc flagdesc_mapgen_watershed[] = {
	{NULL        , 0}
};


MapgenWatershed::MapgenWatershed(MapgenWatershedParams *params, EmergeManager *emerge)
	: MapgenBasic(MAPGEN_WATERSHED, params, emerge)
{
	spflags          = params->spflags;
	div              = std::fmax(params->map_scale, 1.0f);
	sea_y            = params->sea_y;
	flat_y           = params->flat_y;
	continent_area   = params->continent_area;
	river_width      = params->river_width;
	river_depth      = params->river_depth;
	river_bank       = params->river_bank;
	sea_depth        = params->sea_depth;
	land_height      = params->land_height;
	hill_height      = params->hill_height;
	mount_height     = params->mount_height;
	source_level     = params->source_level;

	cave_width         = params->cave_width;
	large_cave_depth   = params->large_cave_depth;
	small_cave_num_min = params->small_cave_num_min;
	small_cave_num_max = params->small_cave_num_max;
	large_cave_num_min = params->large_cave_num_min;
	large_cave_num_max = params->large_cave_num_max;
	large_cave_flooded = params->large_cave_flooded;
	cavern_limit       = params->cavern_limit;
	cavern_taper       = params->cavern_taper;
	cavern_threshold   = params->cavern_threshold;
	dungeon_ymin       = params->dungeon_ymin;
	dungeon_ymax       = params->dungeon_ymax;

	// Density gradient vertical scale
	vertical_scale = 128.0f;

	// Calculate density value of terrain base at flat area level, before division of
	// vertical_scale by div.
	base_flat = (flat_y - sea_y) / vertical_scale;

	// Create noise parameter copies to enable scaling without altering configured or
	// stored noise parameters.
	np_continent_div   = params->np_continent;
	np_base_div        = params->np_base;
	np_flat_div        = params->np_flat;
	np_river1_div      = params->np_river1;
	np_river2_div      = params->np_river2;
	np_mountain_div    = params->np_mountain;
	np_plateau_div     = params->np_plateau;
	np_plat_select_div = params->np_plat_select;
	np_3d_div          = params->np_3d;

	// If in scaled mode, divide vertical scale and noise spreads
	if (div != 1.0f) {
		vertical_scale            /= div;

		np_continent_div.spread   /= v3f(div, div, div);
		np_base_div.spread        /= v3f(div, div, div);
		np_flat_div.spread        /= v3f(div, div, div);
		np_river1_div.spread      /= v3f(div, div, div);
		np_river2_div.spread      /= v3f(div, div, div);
		np_mountain_div.spread    /= v3f(div, div, div);
		np_plateau_div.spread     /= v3f(div, div, div);
		np_plat_select_div.spread /= v3f(div, div, div);
		np_3d_div.spread          /= v3f(div, div, div);
	}

	//// 2D noise
	noise_continent   = new Noise(&np_continent_div,   seed, csize.X, csize.Z);
	noise_base        = new Noise(&np_base_div,        seed, csize.X, csize.Z);
	noise_flat        = new Noise(&np_flat_div,        seed, csize.X, csize.Z);
	noise_river1      = new Noise(&np_river1_div,      seed, csize.X, csize.Z);
	noise_river2      = new Noise(&np_river2_div,      seed, csize.X, csize.Z);
	noise_mountain    = new Noise(&np_mountain_div,    seed, csize.X, csize.Z);
	noise_plateau     = new Noise(&np_plateau_div,     seed, csize.X, csize.Z);
	noise_plat_select = new Noise(&np_plat_select_div, seed, csize.X, csize.Z);

	// Not scaled by map scaling
	noise_filler_depth = new Noise(&params->np_filler_depth, seed, csize.X, csize.Z);

	//// 3D noise
	// 1 up 1 down overgeneration
	noise_3d = new Noise(&np_3d_div, seed, csize.X, csize.Y + 2, csize.Z);
	// 1 down overgeneration
	// Not scaled by map scaling
	MapgenBasic::np_cave1 = params->np_cave1;
	MapgenBasic::np_cave2 = params->np_cave2;
	MapgenBasic::np_cavern = params->np_cavern;
}


MapgenWatershed::~MapgenWatershed()
{
	delete noise_continent;
	delete noise_base;
	delete noise_flat;
	delete noise_river1;
	delete noise_river2;
	delete noise_mountain;
	delete noise_plateau;
	delete noise_plat_select;
	delete noise_3d;

	delete noise_filler_depth;
}


MapgenWatershedParams::MapgenWatershedParams()
{
	// map_scale > 1.0 creates a 3D map world at scale 1:map_scale.
	// map_scale = 31 is a recommended maximum, a 2000x2000 node area will map
	// an entire world.
	// If set above approximately 16, some noise parameters will need to have
	// 'octaves' reduced by approximately 1 to avoid the any octave having a
	// spread < 1. The noise parameters most likely to need altering are
	// 'np_mountain', 'np_plat_select' and 'np_3d'.
	map_scale        = 1.0;
	// Y where density gradient is zero. Terrain is shaped assuming sea level
	// is at this y. This and 'water_level' are independent for flexibility.
	sea_y            = 1.0;
	// Y of coastal flat areas
	flat_y           = 8.0;
	// Adjusts proportion of world covered by continents
	continent_area   = -1.0;
	river_width      = 0.03;
	river_depth      = 0.25;
	// Height of river bank above river surface
	river_bank       = 0.01;
	sea_depth        = 0.2f;
	// Height of the terrain base: the surface of the river courses
	land_height      = 1.4f;
	// Height of the hills between the rivers
	hill_height      = 0.5f;
	mount_height     = 1.5f;
	// Height of river source level above sea level, expressed as a multiplier
	// of 128 (the vertical scale).
	source_level     = 0.9f;

	cave_width         = 0.1;
	large_cave_depth   = -33;
	small_cave_num_min = 0;
	small_cave_num_max = 0;
	large_cave_num_min = 0;
	large_cave_num_max = 2;
	large_cave_flooded = 0.5f;
	cavern_limit       = -256;
	cavern_taper       = 256;
	cavern_threshold   = 0.7;
	dungeon_ymin       = -31000;
	dungeon_ymax       = 31000;

	//*
	np_continent    = NoiseParams(0.0,  1.0,  v3f(12288, 12288, 12288), 4001,  3, 0.5,  2.0);
	np_base         = NoiseParams(0.0,  1.0,  v3f(3072,  3072,  3072),  106,   3, 0.5,  2.0);
	np_flat         = NoiseParams(0.0,  0.4,  v3f(2048,  2048,  2048),  909,   3, 0.5,  2.0);
	np_river1       = NoiseParams(0.0,  1.0,  v3f(1536,  1536,  1536),  2177,  5, 0.5,  2.0);
	np_river2       = NoiseParams(0.0,  1.0,  v3f(1536,  1536,  1536),  5003,  5, 0.5,  2.0);
	np_mountain     = NoiseParams(2.0,  -1.0, v3f(1536,  1536,  1536),  50001, 7, 0.6,  2.0,
		NOISE_FLAG_EASED | NOISE_FLAG_ABSVALUE);
	np_plateau      = NoiseParams(0.5,  0.2,  v3f(1024,  1024,  1024),  8111,  4, 0.4,  2.0);
	np_plat_select  = NoiseParams(-2.0, 6.0,  v3f(3072,  3072,  3072),  30089, 8, 0.7,  2.0);
	np_3d           = NoiseParams(0.0,  1.0,  v3f(128,   256,   128),   70033, 4, 0.6,  2.0);

	/* Map scale = 31 params, 2000x2000 maps an entire world
	np_continent    = NoiseParams(0.0,  1.0,  v3f(12288, 12288, 12288), 4001,  3, 0.5,  2.0);
	np_base         = NoiseParams(0.0,  1.0,  v3f(3072,  3072,  3072),  106,   3, 0.5,  2.0);
	np_flat         = NoiseParams(0.0,  0.4,  v3f(2048,  2048,  2048),  909,   3, 0.5,  2.0);
	np_river1       = NoiseParams(0.0,  1.0,  v3f(1024,  1024,  1024),  2177,  5, 0.5,  2.0);
	np_river2       = NoiseParams(0.0,  1.0,  v3f(1024,  1024,  1024),  5003,  5, 0.5,  2.0);
	np_mountain     = NoiseParams(2.0,  -1.0, v3f(1536,  1536,  1536),  50001, 6, 0.6,  2.0,
		NOISE_FLAG_EASED | NOISE_FLAG_ABSVALUE);
	np_plateau      = NoiseParams(0.5,  0.2,  v3f(1024,  1024,  1024),  8111,  4, 0.4,  2.0);
	np_plat_select  = NoiseParams(-2.0, 6.0,  v3f(3072,  3072,  3072),  30089, 7, 0.7,  2.0);
	np_3d           = NoiseParams(0.0,  1.0,  v3f(128,   256,   128),   70033, 3, 0.6,  2.0);*/

	np_filler_depth = NoiseParams(0.0,  1.0,  v3f(128,   128,   128),   261,   3, 0.7,  2.0);
	np_cave1        = NoiseParams(0.0,  12.0, v3f(61,    61,    61),    52534, 3, 0.5,  2.0);
	np_cave2        = NoiseParams(0.0,  12.0, v3f(67,    67,    67),    10325, 3, 0.5,  2.0);
	np_cavern       = NoiseParams(0.0,  1.0,  v3f(384,   128,   384),   723,   5, 0.6,  2.0);
}


void MapgenWatershedParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed);
	settings->getFloatNoEx("mgwatershed_map_scale",      map_scale);
	settings->getFloatNoEx("mgwatershed_sea_y",          sea_y);
	settings->getFloatNoEx("mgwatershed_flat_y",         flat_y);
	settings->getFloatNoEx("mgwatershed_continent_area", continent_area);
	settings->getFloatNoEx("mgwatershed_river_width",    river_width);
	settings->getFloatNoEx("mgwatershed_river_depth",    river_depth);
	settings->getFloatNoEx("mgwatershed_river_bank",     river_bank);
	settings->getFloatNoEx("mgwatershed_sea_depth",      sea_depth);
	settings->getFloatNoEx("mgwatershed_land_height",    land_height);
	settings->getFloatNoEx("mgwatershed_hill_height",    hill_height);
	settings->getFloatNoEx("mgwatershed_mount_height",   mount_height);
	settings->getFloatNoEx("mgwatershed_source_level",   source_level);

	settings->getFloatNoEx("mgwatershed_cave_width",         cave_width);
	settings->getS16NoEx("mgwatershed_large_cave_depth",     large_cave_depth);
	settings->getU16NoEx("mgwatershed_small_cave_num_min",   small_cave_num_min);
	settings->getU16NoEx("mgwatershed_small_cave_num_max",   small_cave_num_max);
	settings->getU16NoEx("mgwatershed_large_cave_num_min",   large_cave_num_min);
	settings->getU16NoEx("mgwatershed_large_cave_num_max",   large_cave_num_max);
	settings->getFloatNoEx("mgwatershed_large_cave_flooded", large_cave_flooded);
	settings->getS16NoEx("mgwatershed_cavern_limit",         cavern_limit);
	settings->getS16NoEx("mgwatershed_cavern_taper",         cavern_taper);
	settings->getFloatNoEx("mgwatershed_cavern_threshold",   cavern_threshold);
	settings->getS16NoEx("mgwatershed_dungeon_ymin",         dungeon_ymin);
	settings->getS16NoEx("mgwatershed_dungeon_ymax",         dungeon_ymax);

	settings->getNoiseParams("mgwatershed_np_continent",   np_continent);
	settings->getNoiseParams("mgwatershed_np_base",        np_base);
	settings->getNoiseParams("mgwatershed_np_flat",        np_flat);
	settings->getNoiseParams("mgwatershed_np_river1",      np_river1);
	settings->getNoiseParams("mgwatershed_np_river2",      np_river2);
	settings->getNoiseParams("mgwatershed_np_mountain",    np_mountain);
	settings->getNoiseParams("mgwatershed_np_plateau",     np_plateau);
	settings->getNoiseParams("mgwatershed_np_plat_select", np_plat_select);
	settings->getNoiseParams("mgwatershed_np_3d",          np_3d);

	settings->getNoiseParams("mgwatershed_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->getNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->getNoiseParams("mgwatershed_np_cavern",       np_cavern);
}


void MapgenWatershedParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed);
	settings->setFloat("mgwatershed_map_scale",      map_scale);
	settings->setFloat("mgwatershed_sea_y",          sea_y);
	settings->setFloat("mgwatershed_flat_y",         flat_y);
	settings->setFloat("mgwatershed_continent_area", continent_area);
	settings->setFloat("mgwatershed_river_width",    river_width);
	settings->setFloat("mgwatershed_river_depth",    river_depth);
	settings->setFloat("mgwatershed_river_bank",     river_bank);
	settings->setFloat("mgwatershed_sea_depth",      sea_depth);
	settings->setFloat("mgwatershed_land_height",    land_height);
	settings->setFloat("mgwatershed_hill_height",    hill_height);
	settings->setFloat("mgwatershed_mount_height",   mount_height);
	settings->setFloat("mgwatershed_source_level",   source_level);

	settings->setFloat("mgwatershed_cave_width",         cave_width);
	settings->setS16("mgwatershed_large_cave_depth",     large_cave_depth);
	settings->setU16("mgwatershed_small_cave_num_min",   small_cave_num_min);
	settings->setU16("mgwatershed_small_cave_num_max",   small_cave_num_max);
	settings->setU16("mgwatershed_large_cave_num_min",   large_cave_num_min);
	settings->setU16("mgwatershed_large_cave_num_max",   large_cave_num_max);
	settings->setFloat("mgwatershed_large_cave_flooded", large_cave_flooded);
	settings->setS16("mgwatershed_cavern_limit",         cavern_limit);
	settings->setS16("mgwatershed_cavern_taper",         cavern_taper);
	settings->setFloat("mgwatershed_cavern_threshold",   cavern_threshold);
	settings->setS16("mgwatershed_dungeon_ymin",         dungeon_ymin);
	settings->setS16("mgwatershed_dungeon_ymax",         dungeon_ymax);

	settings->setNoiseParams("mgwatershed_np_continent",   np_continent);
	settings->setNoiseParams("mgwatershed_np_base",        np_base);
	settings->setNoiseParams("mgwatershed_np_flat",        np_flat);
	settings->setNoiseParams("mgwatershed_np_river1",      np_river1);
	settings->setNoiseParams("mgwatershed_np_river2",      np_river2);
	settings->setNoiseParams("mgwatershed_np_mountain",    np_mountain);
	settings->setNoiseParams("mgwatershed_np_plateau",     np_plateau);
	settings->setNoiseParams("mgwatershed_np_plat_select", np_plat_select);
	settings->setNoiseParams("mgwatershed_np_3d",          np_3d);

	settings->setNoiseParams("mgwatershed_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->setNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->setNoiseParams("mgwatershed_np_cavern",       np_cavern);
}


void MapgenWatershedParams::setDefaultSettings(Settings *settings)
{
	settings->setDefault("mgwatershed_spflags", flagdesc_mapgen_watershed, 0);
}


int MapgenWatershed::getSpawnLevelAtPoint(v2s16 p)
{
	// If in scaled mode, immediately return y = 128
	if (div != 1.0f)
		return 128;

	float n_continent = continent_area +
		std::fabs(NoisePerlin2D(&noise_continent->np, p.X, p.Y, seed)) * 2.0f;
	float n_cont_tanh = std::tanh(n_continent * 4.0f);
	float n_base = NoisePerlin2D(&noise_base->np, p.X, p.Y, seed);
	float n_tbase = n_cont_tanh * 0.6f + n_base - 0.2f;

	float n_flat = std::fmax(NoisePerlin2D(&noise_flat->np, p.X, p.Y, seed), 0.0f);
	float n_base_shaped = base_flat;
	if (n_tbase < base_flat) {
		n_base_shaped = base_flat - (base_flat - n_tbase) * sea_depth;
	} else if (n_tbase > base_flat + n_flat) {
		n_base_shaped = base_flat +
			std::pow(n_tbase - (base_flat + n_flat), 1.5f) * land_height;
	}

	float n_river1 = NoisePerlin2D(&noise_river1->np, p.X, p.Y, seed);
	float n_river1_abs = std::fabs(n_river1);
	float n_river2 = NoisePerlin2D(&noise_river2->np, p.X, p.Y, seed);
	float n_river2_abs = std::fabs(n_river2);
	float sink = ((source_level - n_base_shaped) / source_level) * river_width;
	float n_valley1_sunk = n_river1_abs - sink;
	float n_valley2_sunk = n_river2_abs - sink;
	float verp = std::tanh((n_valley2_sunk - n_valley1_sunk) * 16.0f) * 0.5f + 0.5f;
	float n_valley_sunk = verp * n_valley1_sunk + (1.0f - verp) * n_valley2_sunk;

	float n_valley_shaped;
	if (n_valley_sunk > 0.0f) {
		float blend = (n_tbase - (base_flat + n_flat)) / 0.3f;
		float n_val_amp = 0.0f;
		if (blend >= 1.0f) {
			n_val_amp = 1.0f;
		} else if (blend > 0.0f && blend < 1.0f) {
			n_val_amp = blend * blend * (3.0f - 2.0f * blend);
		}
		n_valley_shaped = std::pow(n_valley_sunk, 1.5f) * n_val_amp * hill_height;
	} else {
		// Unsuitable spawn position, river channel
		return MAX_MAP_GENERATION_LIMIT;
	}

	float n_mount_amp = n_base_shaped - source_level;
	float n_mount = -1000.0f;
	if (n_mount_amp > 0.0f) {
		float n_mountain = NoisePerlin2D(&noise_mountain->np, p.X, p.Y, seed);
		n_mount = n_mountain * n_mount_amp * n_mount_amp * mount_height;
	}

	float n_lowland = n_base_shaped + std::fmax(n_valley_shaped, n_mount);
	float n_plateau = std::fmax(NoisePerlin2D(&noise_plateau->np, p.X, p.Y, seed),
		n_lowland);

	float n_plat_select = NoisePerlin2D(&noise_plat_select->np, p.X, p.Y, seed);
	float n_plat_sel_coast = (n_tbase + 0.1f) * 16.0f;

	//// Y loop.
	// Starting spawn search at max_spawn_y + 128 ensures many nodes of open
	// space above spawn position. Avoids spawning in sealed voids.

	for (s16 y = sea_y + 128; y >= sea_y; y--) {
		float n_3d = NoisePerlin3D(&noise_3d->np, p.X, y, p.Y, seed);
		float n_plat_sel_canyon = -1000.0f;
		if (n_valley_sunk > 0)
			n_plat_sel_canyon = n_base_shaped +
				std::pow(n_valley_sunk - std::fabs(n_3d) * 0.1f, 3.0f) * 512.0f;
		float n_select = std::fmin(
			std::fmin(n_plat_select, n_plat_sel_coast) + n_3d * 2.0f,
			n_plat_sel_canyon);
		float n_terrain = rangelim(n_select, n_lowland, n_plateau);
		float density_grad = (sea_y - y) / vertical_scale;
		float density = n_terrain + density_grad;

		if (density >= 0.0f) {
			// Solid node
			if (y < water_level || y > sea_y + 16)
				// Unsuitable spawn position, underwater or not low altitude
				return MAX_MAP_GENERATION_LIMIT;

			// y + 2 because y is surface and due to biome 'dust' nodes.
			return y + 2;
		}
	}

	// Unsuitable spawn position, no ground found
	return MAX_MAP_GENERATION_LIMIT;
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
	s16 stone_surface_max_y = generateTerrain();

	// If not scaled mode
	if (div == 1.0f) {
		// Create heightmap
		updateHeightmap(node_min, node_max);

		// Init biome generator, place biome-specific nodes, and build biomemap
		if (flags & MG_BIOMES) {
			biomegen->calcBiomeNoise(node_min);
			generateBiomes();
		}

		// Generate caverns, tunnels and classic caves
		if (flags & MG_CAVES) {
			// Generate tunnels first as caverns confuse them
			generateCavesNoiseIntersection(stone_surface_max_y);

			// Generate caverns
			bool near_cavern = generateCavernsNoise(stone_surface_max_y);

			// Generate large randomwalk caves
			if (near_cavern)
				// Disable large randomwalk caves in this mapchunk by setting
				// 'large cave depth' to world base. Avoids excessive liquid in large
				// caverns and floating blobs of overgenerated liquid.
				// Avoids liquids leaking at world edge.
				generateCavesRandomWalk(stone_surface_max_y, -MAX_MAP_GENERATION_LIMIT);
			else
				generateCavesRandomWalk(stone_surface_max_y, large_cave_depth);
		}

		// Generate the registered ores
		m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

		// Generate dungeons
		if (flags & MG_DUNGEONS)
			generateDungeons(stone_surface_max_y);

		// Generate the registered decorations
		if (flags & MG_DECORATIONS)
			m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

		// Sprinkle some dust on top after everything else was generated
		if (flags & MG_BIOMES)
			dustTopNodes();
	}

	// Update liquids
	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	// Calculate lighting
	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max, true);

	this->generating = false;

	//printf("makeChunk: %lums\n", t.stop());
}


int MapgenWatershed::generateTerrain()
{
	MapNode mn_air(CONTENT_AIR);
	MapNode mn_stone(c_stone);
	MapNode mn_water(c_water_source);
	MapNode mn_river_water(c_river_water_source);

	noise_continent  ->perlinMap2D(node_min.X, node_min.Z);
	noise_base       ->perlinMap2D(node_min.X, node_min.Z);
	noise_flat       ->perlinMap2D(node_min.X, node_min.Z);
	noise_river1     ->perlinMap2D(node_min.X, node_min.Z);
	noise_river2     ->perlinMap2D(node_min.X, node_min.Z);
	noise_mountain   ->perlinMap2D(node_min.X, node_min.Z);
	noise_plateau    ->perlinMap2D(node_min.X, node_min.Z);
	noise_plat_select->perlinMap2D(node_min.X, node_min.Z);

	noise_3d->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	//// Cache density gradient values
	float density_grad_cache[csize.Y + 2];
	u8 cache_index = 0;

	for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++, cache_index++) {
		density_grad_cache[cache_index] = (sea_y - y) / vertical_scale;
	}

	//// ZX loop
	int stone_max_y = -MAX_MAP_GENERATION_LIMIT;
	const v3s16 &em = vm->m_area.getExtent();
	u32 index2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index2d++) {
		//// Terrain base.
		// n_continent is roughly -1 to 1.
		float n_continent = continent_area +
			std::fabs(noise_continent->result[index2d]) * 2.0f;
		// Continental variation.
		// n_cont_tanh is mostly -1 or 1, and smooth transition between
		float n_cont_tanh = std::tanh(n_continent * 4.0f);
		// Basic terrain base variation
		float n_base = noise_base->result[index2d];
		// Combine. Set continental and base amplitudes, and master offset here
		float n_tbase = n_cont_tanh * 0.6f + n_base - 0.2f;

		// Add coastal flat areas with varying widths
		float n_flat = std::fmax(noise_flat->result[index2d], 0.0f);
		// n_base_shaped is terrain base, the surface of river courses, and seabed
		float n_base_shaped = base_flat; // Coastal flat areas
		if (n_tbase < base_flat) {
			// Shore and seabed below coastal flat areas
			n_base_shaped = base_flat - (base_flat - n_tbase) * sea_depth;
		} else if (n_tbase > base_flat + n_flat) {
			// Land above coastal flat areas, shape with exponent
			n_base_shaped = base_flat +
				std::pow(n_tbase - (base_flat + n_flat), 1.5f) * land_height;
		}

		//// River valleys.
		// River valley 1.
		float n_river1 = noise_river1->result[index2d];
		float n_river1_abs = std::fabs(n_river1);
		// River valley 2
		float n_river2 = noise_river2->result[index2d];
		float n_river2_abs = std::fabs(n_river2);
		// River valley cross sections sink below 0 to define river area and width.
		// 'sink' is 0 at source level, river_width at sea level.
		float sink = ((source_level - n_base_shaped) / source_level) * river_width;
		float n_valley1_sunk = n_river1_abs - sink;
		float n_valley2_sunk = n_river2_abs - sink;
		// Combine river valleys.
		// Smoothly interpolate between valleys using difference between them.
		float verp = std::tanh((n_valley2_sunk - n_valley1_sunk) * 16.0f) * 0.5f + 0.5f;
		float n_valley_sunk = verp * n_valley1_sunk + (1.0f - verp) * n_valley2_sunk;

		// Hills between rivers, and river channels
		// n_valley_shaped is the hills between rivers and the river channels.
		float n_valley_shaped;
		if (n_valley_sunk > 0.0f) { // Between rivers
			// Smoothly blend valley amplitude into flat areas or coast.
			// Set blend distance here.
			float blend = (n_tbase - (base_flat + n_flat)) / 0.3f;
			float n_val_amp = 0.0f;
			if (blend >= 1.0f) {
				n_val_amp = 1.0f;
			} else if (blend > 0.0f && blend < 1.0f) {
				// Smoothstep blend
				n_val_amp = blend * blend * (3.0f - 2.0f * blend);
			}
			// Hills between rivers
			n_valley_shaped = std::pow(n_valley_sunk, 1.5f) * n_val_amp * hill_height;
		} else { // River channels
			float river_depth_shaped = (n_base_shaped < 0.0f) ?
				// Alter river depth to remove channels below sea level
				std::fmax(river_depth + n_base_shaped * 4.0f, 0.0f) :
				// Normal river depth
				river_depth;
			// River channels
			n_valley_shaped = -std::sqrt(-n_valley_sunk) * river_depth_shaped;
		}

		//// Mountains.
		// Only above river source level.
		float n_mount_amp = n_base_shaped - source_level;
		// n_mount is the mountains
		float n_mount = -1000.0f;
		if (n_mount_amp > 0.0f) {
			float n_mountain = noise_mountain->result[index2d];
			n_mount = n_mountain * n_mount_amp * n_mount_amp * mount_height;
		}

		//// Plateaus.
		// The 2 terrains switched between:
		// n_lowland is the higher of hills or mountains.
		float n_lowland = n_base_shaped + std::fmax(n_valley_shaped, n_mount);
		// n_plateau is the higher of plateaus and n_lowland
		float n_plateau = std::fmax(noise_plateau->result[index2d], n_lowland);

		// Plateau select.
		// 3D noise is added in Y loop.
		// Plateau select for plateau areas.
		float n_plat_select = noise_plat_select->result[index2d];
		// Plateau select for coastal cliffs
		float n_plat_sel_coast = (n_tbase + 0.1f) * 16.0f;

		//// Y loop
		// Indexes at column base
		cache_index = 0;
		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);
		u32 index3d = (z - node_min.Z) * zstride_1u1d + (x - node_min.X);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1;
				y++,
				VoxelArea::add_y(em, vi, 1),
				index3d += ystride,
				cache_index++) {
			if (vm->m_data[vi].getContent() != CONTENT_IGNORE)
				continue;

			// 3D noise
			float n_3d = noise_3d->result[index3d];
			// Plateau select for canyons
			float n_plat_sel_canyon = -1000.0f;
			if (n_valley_sunk > 0)
				n_plat_sel_canyon = n_base_shaped +
					std::pow(n_valley_sunk - std::fabs(n_3d) * 0.1f, 3.0f) * 512.0f;
			// Terrain select, rapidly switches between n_lowland and n_plateau
			float n_select = std::fmin(
				std::fmin(n_plat_select, n_plat_sel_coast) + n_3d * 2.0f,
				n_plat_sel_canyon);
			// Terrain density noise
			float n_terrain = rangelim(n_select, n_lowland, n_plateau);
			// Density gradient
			float density_grad = density_grad_cache[cache_index];
			// Terrain base, river course and seabed level
			float density_base = n_base_shaped + density_grad;
			// Final terrain level
			float density = n_terrain + density_grad;

			if (density >= 0.0f) {
				vm->m_data[vi] = mn_stone; // Stone
				if (y > stone_max_y)
					stone_max_y = y;
			} else if (y <= water_level) {
				vm->m_data[vi] = mn_water; // Sea water
			} else if (density_base >= river_bank) {
				vm->m_data[vi] = mn_river_water; // River water
			} else {
				vm->m_data[vi] = mn_air; // Air
			}
		}
	}

	return stone_max_y;
}
