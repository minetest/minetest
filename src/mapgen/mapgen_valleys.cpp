/*
Minetest
Copyright (C) 2016-2018 Duane Robertson <duane@duanerobertson.com>
Copyright (C) 2016-2018 paramat

Based on Valleys Mapgen by Gael de Sailly
(https://forum.minetest.net/viewtopic.php?f=9&t=11430)
and mapgen_v7, mapgen_flat by kwolekr and paramat.

Licensing changed by permission of Gael de Sailly.

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
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_valleys.h"
#include "cavegen.h"
#include <cmath>


static FlagDesc flagdesc_mapgen_valleys[] = {
	{"altitude_chill", MGVALLEYS_ALT_CHILL},
	{"humid_rivers",   MGVALLEYS_HUMID_RIVERS},
	{NULL,             0}
};


////////////////////////////////////////////////////////////////////////////////


MapgenValleys::MapgenValleys(int mapgenid, MapgenValleysParams *params,
	EmergeManager *emerge)
	: MapgenBasic(mapgenid, params, emerge)
{
	// NOTE: MapgenValleys has a hard dependency on BiomeGenOriginal
	m_bgen = (BiomeGenOriginal *)biomegen;

	BiomeParamsOriginal *bp = (BiomeParamsOriginal *)params->bparams;

	spflags            = params->spflags;
	altitude_chill     = params->altitude_chill;
	river_depth_bed    = params->river_depth + 1.0f;
	river_size_factor  = params->river_size / 100.0f;

	cave_width         = params->cave_width;
	large_cave_depth   = params->large_cave_depth;
	lava_depth         = params->lava_depth;
	cavern_limit       = params->cavern_limit;
	cavern_taper       = params->cavern_taper;
	cavern_threshold   = params->cavern_threshold;
	dungeon_ymin       = params->dungeon_ymin;
	dungeon_ymax       = params->dungeon_ymax;

	//// 2D Terrain noise
	noise_filler_depth       = new Noise(&params->np_filler_depth,       seed, csize.X, csize.Z);
	noise_inter_valley_slope = new Noise(&params->np_inter_valley_slope, seed, csize.X, csize.Z);
	noise_rivers             = new Noise(&params->np_rivers,             seed, csize.X, csize.Z);
	noise_terrain_height     = new Noise(&params->np_terrain_height,     seed, csize.X, csize.Z);
	noise_valley_depth       = new Noise(&params->np_valley_depth,       seed, csize.X, csize.Z);
	noise_valley_profile     = new Noise(&params->np_valley_profile,     seed, csize.X, csize.Z);

	//// 3D Terrain noise
	// 1-up 1-down overgeneration
	noise_inter_valley_fill = new Noise(&params->np_inter_valley_fill,
		seed, csize.X, csize.Y + 2, csize.Z);
	// 1-down overgeneraion
	MapgenBasic::np_cave1   = params->np_cave1;
	MapgenBasic::np_cave2   = params->np_cave2;
	MapgenBasic::np_cavern  = params->np_cavern;

	humid_rivers       = (spflags & MGVALLEYS_HUMID_RIVERS);
	use_altitude_chill = (spflags & MGVALLEYS_ALT_CHILL);
	humidity_adjust    = bp->np_humidity.offset - 50.0f;
}


MapgenValleys::~MapgenValleys()
{
	delete noise_filler_depth;
	delete noise_inter_valley_fill;
	delete noise_inter_valley_slope;
	delete noise_rivers;
	delete noise_terrain_height;
	delete noise_valley_depth;
	delete noise_valley_profile;
}


MapgenValleysParams::MapgenValleysParams():
	np_filler_depth       (0.0,   1.2,  v3f(256,  256,  256),  1605,  3, 0.5,  2.0),
	np_inter_valley_fill  (0.0,   1.0,  v3f(256,  512,  256),  1993,  6, 0.8,  2.0),
	np_inter_valley_slope (0.5,   0.5,  v3f(128,  128,  128),  746,   1, 1.0,  2.0),
	np_rivers             (0.0,   1.0,  v3f(256,  256,  256),  -6050, 5, 0.6,  2.0),
	np_terrain_height     (-10.0, 50.0, v3f(1024, 1024, 1024), 5202,  6, 0.4,  2.0),
	np_valley_depth       (5.0,   4.0,  v3f(512,  512,  512),  -1914, 1, 1.0,  2.0),
	np_valley_profile     (0.6,   0.50, v3f(512,  512,  512),  777,   1, 1.0,  2.0),
	np_cave1              (0.0,   12.0, v3f(61,   61,   61),   52534, 3, 0.5,  2.0),
	np_cave2              (0.0,   12.0, v3f(67,   67,   67),   10325, 3, 0.5,  2.0),
	np_cavern             (0.0,   1.0,  v3f(768,  256,  768),  59033, 6, 0.63, 2.0)
{
}


void MapgenValleysParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgvalleys_spflags",        spflags, flagdesc_mapgen_valleys);
	settings->getU16NoEx("mgvalleys_altitude_chill",     altitude_chill);
	settings->getS16NoEx("mgvalleys_large_cave_depth",   large_cave_depth);
	settings->getS16NoEx("mgvalleys_lava_depth",         lava_depth);
	settings->getU16NoEx("mgvalleys_river_depth",        river_depth);
	settings->getU16NoEx("mgvalleys_river_size",         river_size);
	settings->getFloatNoEx("mgvalleys_cave_width",       cave_width);
	settings->getS16NoEx("mgvalleys_cavern_limit",       cavern_limit);
	settings->getS16NoEx("mgvalleys_cavern_taper",       cavern_taper);
	settings->getFloatNoEx("mgvalleys_cavern_threshold", cavern_threshold);
	settings->getS16NoEx("mgvalleys_dungeon_ymin",       dungeon_ymin);
	settings->getS16NoEx("mgvalleys_dungeon_ymax",       dungeon_ymax);

	settings->getNoiseParams("mgvalleys_np_filler_depth",       np_filler_depth);
	settings->getNoiseParams("mgvalleys_np_inter_valley_fill",  np_inter_valley_fill);
	settings->getNoiseParams("mgvalleys_np_inter_valley_slope", np_inter_valley_slope);
	settings->getNoiseParams("mgvalleys_np_rivers",             np_rivers);
	settings->getNoiseParams("mgvalleys_np_terrain_height",     np_terrain_height);
	settings->getNoiseParams("mgvalleys_np_valley_depth",       np_valley_depth);
	settings->getNoiseParams("mgvalleys_np_valley_profile",     np_valley_profile);

	settings->getNoiseParams("mgvalleys_np_cave1",              np_cave1);
	settings->getNoiseParams("mgvalleys_np_cave2",              np_cave2);
	settings->getNoiseParams("mgvalleys_np_cavern",             np_cavern);
}


void MapgenValleysParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgvalleys_spflags",        spflags, flagdesc_mapgen_valleys, U32_MAX);
	settings->setU16("mgvalleys_altitude_chill",     altitude_chill);
	settings->setS16("mgvalleys_large_cave_depth",   large_cave_depth);
	settings->setS16("mgvalleys_lava_depth",         lava_depth);
	settings->setU16("mgvalleys_river_depth",        river_depth);
	settings->setU16("mgvalleys_river_size",         river_size);
	settings->setFloat("mgvalleys_cave_width",       cave_width);
	settings->setS16("mgvalleys_cavern_limit",       cavern_limit);
	settings->setS16("mgvalleys_cavern_taper",       cavern_taper);
	settings->setFloat("mgvalleys_cavern_threshold", cavern_threshold);
	settings->setS16("mgvalleys_dungeon_ymin",       dungeon_ymin);
	settings->setS16("mgvalleys_dungeon_ymax",       dungeon_ymax);

	settings->setNoiseParams("mgvalleys_np_filler_depth",       np_filler_depth);
	settings->setNoiseParams("mgvalleys_np_inter_valley_fill",  np_inter_valley_fill);
	settings->setNoiseParams("mgvalleys_np_inter_valley_slope", np_inter_valley_slope);
	settings->setNoiseParams("mgvalleys_np_rivers",             np_rivers);
	settings->setNoiseParams("mgvalleys_np_terrain_height",     np_terrain_height);
	settings->setNoiseParams("mgvalleys_np_valley_depth",       np_valley_depth);
	settings->setNoiseParams("mgvalleys_np_valley_profile",     np_valley_profile);

	settings->setNoiseParams("mgvalleys_np_cave1",              np_cave1);
	settings->setNoiseParams("mgvalleys_np_cave2",              np_cave2);
	settings->setNoiseParams("mgvalleys_np_cavern",             np_cavern);
}


////////////////////////////////////////////////////////////////////////////////


void MapgenValleys::makeChunk(BlockMakeData *data)
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
	this->vm = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate biome noises. Note this must be executed strictly before
	// generateTerrain, because generateTerrain depends on intermediate
	// biome-related noises.
	m_bgen->calcBiomeNoise(node_min);

	// Generate noise maps and base terrain height.
	// Modify heat and humidity maps.
	calculateNoise();

	// Generate base terrain with initial heightmaps
	s16 stone_surface_max_y = generateTerrain();

	// Recalculate heightmap
	updateHeightmap(node_min, node_max);

	// Place biome-specific nodes and build biomemap
	if (flags & MG_BIOMES)
		generateBiomes();

	// Generate tunnels, caverns and large randomwalk caves
	if (flags & MG_CAVES) {
		// Generate tunnels first as caverns confuse them
		generateCavesNoiseIntersection(stone_surface_max_y);

		// Generate caverns
		bool near_cavern = generateCavernsNoise(stone_surface_max_y);

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

	// Dungeon creation
	if ((flags & MG_DUNGEONS) && full_node_min.Y >= dungeon_ymin &&
			full_node_max.Y <= dungeon_ymax)
		generateDungeons(stone_surface_max_y);

	// Generate the registered decorations
	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Generate the registered ores
	m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	if (flags & MG_BIOMES)
		dustTopNodes();

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);

	this->generating = false;

	//printf("makeChunk: %lums\n", t.stop());
}


void MapgenValleys::calculateNoise()
{
	int x = node_min.X;
	int y = node_min.Y - 1;
	int z = node_min.Z;

	noise_inter_valley_slope->perlinMap2D(x, z);
	noise_rivers->perlinMap2D(x, z);
	noise_terrain_height->perlinMap2D(x, z);
	noise_valley_depth->perlinMap2D(x, z);
	noise_valley_profile->perlinMap2D(x, z);

	noise_inter_valley_fill->perlinMap3D(x, y, z);

	float heat_offset = 0.0f;
	float humidity_scale = 1.0f;
	// Altitude chill tends to reduce the average heat.
	if (use_altitude_chill)
		heat_offset = 5.0f;
	// River humidity tends to increase the humidity range.
	if (humid_rivers)
		humidity_scale = 0.8f;

	for (s32 index = 0; index < csize.X * csize.Z; index++) {
		m_bgen->heatmap[index] += heat_offset;
		m_bgen->humidmap[index] *= humidity_scale;
	}

	TerrainNoise tn;

	u32 index = 0;
	for (tn.z = node_min.Z; tn.z <= node_max.Z; tn.z++)
	for (tn.x = node_min.X; tn.x <= node_max.X; tn.x++, index++) {
		// The parameters that we actually need to generate terrain are passed
		// by address (and the return value).
		tn.terrain_height    = noise_terrain_height->result[index];
		// River noise is replaced with base terrain, which is basically the
		// height of the water table.
		tn.rivers            = &noise_rivers->result[index];
		// Valley depth noise is replaced with the valley number that represents
		// the height of terrain over rivers and is used to determine how close
		// a river is for humidity calculation.
		tn.valley            = &noise_valley_depth->result[index];
		tn.valley_profile    = noise_valley_profile->result[index];
		// Slope noise is replaced by the calculated slope which is used to get
		// terrain height in the slow method, to create sharper mountains.
		tn.slope             = &noise_inter_valley_slope->result[index];
		tn.inter_valley_fill = noise_inter_valley_fill->result[index];

		// This is the actual terrain height.
		float mount = terrainLevelFromNoise(&tn);
		noise_terrain_height->result[index] = mount;
	}
}


float MapgenValleys::terrainLevelFromNoise(TerrainNoise *tn)
{
	// The square function changes the behaviour of this noise: very often
	// small, and sometimes very high.
	float valley_d = MYSQUARE(*tn->valley);

	// valley_d is here because terrain is generally higher where valleys are
	// deep (mountains). base represents the height of the rivers, most of the
	// surface is above.
	float base = tn->terrain_height + valley_d;

	// "river" represents the distance from the river
	float river = std::fabs(*tn->rivers) - river_size_factor;

	// Use the curve of the function 1-exp(-(x/a)^2) to model valleys.
	// "valley" represents the height of the terrain, from the rivers.
	float tv = std::fmax(river / tn->valley_profile, 0.0f);
	*tn->valley = valley_d * (1.0f - std::exp(-MYSQUARE(tv)));

	// Approximate height of the terrain at this point
	float mount = base + *tn->valley;

	*tn->slope *= *tn->valley;

	// Base ground is returned as rivers since it's basically the water table.
	*tn->rivers = base;

	// Rivers are placed where "river" is negative, so where the original noise
	// value is close to zero.
	if (river < 0.0f) {
		// Use the the function -sqrt(1-x^2) which models a circle
		float tr = river / river_size_factor + 1.0f;
		float depth = (river_depth_bed *
			std::sqrt(std::fmax(0.0f, 1.0f - MYSQUARE(tr))));

		// base - depth : height of the bottom of the river
		// water_level - 3 : don't make rivers below 3 nodes under the surface.
		// We use three because that's as low as the swamp biomes go.
		// There is no logical equivalent to this using rangelim.
		mount =
			std::fmin(std::fmax(base - depth, (float)(water_level - 3)), mount);

		// Slope has no influence on rivers
		*tn->slope = 0.0f;
	}

	return mount;
}


// This avoids duplicating the code in terrainLevelFromNoise, adding only the
// final step of terrain generation without a noise map.

float MapgenValleys::adjustedTerrainLevelFromNoise(TerrainNoise *tn)
{
	float mount = terrainLevelFromNoise(tn);
	s16 y_start = myround(mount);

	for (s16 y = y_start; y <= y_start + 1000; y++) {
		float fill =
			NoisePerlin3D(&noise_inter_valley_fill->np, tn->x, y, tn->z, seed);
		if (fill * *tn->slope < y - mount) {
			mount = std::fmax((float)(y - 1), mount);
			break;
		}
	}

	return mount;
}


int MapgenValleys::getSpawnLevelAtPoint(v2s16 p)
{
	// Check if in a river
	float rivers = NoisePerlin2D(&noise_rivers->np, p.X, p.Y, seed);
	if (std::fabs(rivers) < river_size_factor)
		return MAX_MAP_GENERATION_LIMIT;  // Unsuitable spawn point

	s16 level_at_point = terrainLevelAtPoint(p.X, p.Y);
	if (level_at_point <= water_level ||
			level_at_point > water_level + 16)
		return MAX_MAP_GENERATION_LIMIT;  // Unsuitable spawn point

	return level_at_point;
}


float MapgenValleys::terrainLevelAtPoint(s16 x, s16 z)
{
	TerrainNoise tn;

	float rivers = NoisePerlin2D(&noise_rivers->np, x, z, seed);
	float valley = NoisePerlin2D(&noise_valley_depth->np, x, z, seed);
	float inter_valley_slope =
		NoisePerlin2D(&noise_inter_valley_slope->np, x, z, seed);

	tn.x                 = x;
	tn.z                 = z;
	tn.terrain_height    = NoisePerlin2D(&noise_terrain_height->np, x, z, seed);
	tn.rivers            = &rivers;
	tn.valley            = &valley;
	tn.valley_profile    = NoisePerlin2D(&noise_valley_profile->np, x, z, seed);
	tn.slope             = &inter_valley_slope;
	tn.inter_valley_fill = 0.0f;

	return adjustedTerrainLevelFromNoise(&tn);
}


int MapgenValleys::generateTerrain()
{
	// Raising this reduces the rate of evaporation
	static const float evaporation = 300.0f;
	static const float humidity_dropoff = 4.0f;
	// Constant to convert altitude chill to heat
	static const float alt_to_heat = 20.0f;
	// Humidity reduction by altitude
	static const float alt_to_humid = 10.0f;

	MapNode n_air(CONTENT_AIR);
	MapNode n_river_water(c_river_water_source);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	const v3s16 &em = vm->m_area.getExtent();
	s16 surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 index_2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index_2d++) {
		float river_y = noise_rivers->result[index_2d];
		float surface_y = noise_terrain_height->result[index_2d];
		float slope = noise_inter_valley_slope->result[index_2d];
		float t_heat = m_bgen->heatmap[index_2d];

		heightmap[index_2d] = -MAX_MAP_GENERATION_LIMIT;

		if (surface_y > surface_max_y)
			surface_max_y = std::ceil(surface_y);

		if (humid_rivers) {
			// Derive heat from (base) altitude. This will be most correct
			// at rivers, since other surface heights may vary below.
			if (use_altitude_chill && (surface_y > 0.0f || river_y > 0.0f))
				t_heat -= alt_to_heat *
					std::fmax(surface_y, river_y) / altitude_chill;

			// If humidity is low or heat is high, lower the water table
			float delta = m_bgen->humidmap[index_2d] - 50.0f;
			if (delta < 0.0f) {
				float t_evap = (t_heat - 32.0f) / evaporation;
				river_y += delta * std::fmax(t_evap, 0.08f);
			}
		}

		u32 index_3d = (z - node_min.Z) * zstride_1u1d + (x - node_min.X);
		u32 index_data = vm->m_area.index(x, node_min.Y - 1, z);

		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[index_data].getContent() == CONTENT_IGNORE) {
				float fill = noise_inter_valley_fill->result[index_3d];
				float surface_delta = (float)y - surface_y;
				bool river = y < river_y - 1;

				if (slope * fill > surface_delta) {
					vm->m_data[index_data] = n_stone; // Stone
					if (y > heightmap[index_2d])
						heightmap[index_2d] = y;
					if (y > surface_max_y)
						surface_max_y = y;
				} else if (y <= water_level) {
					vm->m_data[index_data] = n_water; // Water
				} else if (river) {
					vm->m_data[index_data] = n_river_water; // River water
				} else {
					vm->m_data[index_data] = n_air; // Air
				}
			}

			VoxelArea::add_y(em, index_data, 1);
			index_3d += ystride;
		}

		if (heightmap[index_2d] == -MAX_MAP_GENERATION_LIMIT) {
			s16 surface_y_int = myround(surface_y);

			if (surface_y_int > node_max.Y + 1 ||
					surface_y_int < node_min.Y - 1) {
				// If surface_y is outside the chunk, it's good enough
				heightmap[index_2d] = surface_y_int;
			} else {
				// If the ground is outside of this chunk, but surface_y is
				// within the chunk, give a value outside.
				heightmap[index_2d] = node_min.Y - 2;
			}
		}

		if (humid_rivers) {
			// Use base ground (water table) in a riverbed, to avoid an
			// unnatural rise in humidity.
			float t_alt = std::fmax(noise_rivers->result[index_2d],
				(float)heightmap[index_2d]);
			float humid = m_bgen->humidmap[index_2d];
			float water_depth = (t_alt - river_y) / humidity_dropoff;
			humid *= 1.0f + std::pow(0.5f, std::fmax(water_depth, 1.0f));

			// Reduce humidity with altitude (ignoring riverbeds)
			if (t_alt > 0.0f)
				humid -= alt_to_humid * t_alt / altitude_chill;

			m_bgen->humidmap[index_2d] = humid;
		}

		// Assign the heat adjusted by any changed altitudes.
		// The altitude will change about half the time.
		if (use_altitude_chill) {
			// Ground height ignoring riverbeds
			float t_alt = std::fmax(noise_rivers->result[index_2d],
				(float)heightmap[index_2d]);

			if (humid_rivers && heightmap[index_2d] == (s16)myround(surface_y))
				// The altitude hasn't changed. Use the first result
				m_bgen->heatmap[index_2d] = t_heat;
			else if (t_alt > 0.0f)
				m_bgen->heatmap[index_2d] -=
					alt_to_heat * t_alt / altitude_chill;
		}
	}

	return surface_max_y;
}
