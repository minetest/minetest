/*
Minetest Valleys C
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 paramat, Matt Gregory
Copyright (C) 2016 Duane Robertson <duane@duanerobertson.com>

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
#include "content_sao.h"
#include "nodedef.h"
#include "voxelalgorithms.h"
#include "settings.h" // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_valleys.h"
#include "cavegen.h"


//#undef NDEBUG
//#include "assert.h"

//#include "util/timetaker.h"
//#include "profiler.h"


//static Profiler mapgen_prof;
//Profiler *mapgen_profiler = &mapgen_prof;

static FlagDesc flagdesc_mapgen_valleys[] = {
	{"altitude_chill", MGVALLEYS_ALT_CHILL},
	{"humid_rivers",   MGVALLEYS_HUMID_RIVERS},
	{NULL,             0}
};

///////////////////////////////////////////////////////////////////////////////


MapgenValleys::MapgenValleys(int mapgenid, MapgenValleysParams *params, EmergeManager *emerge)
	: MapgenBasic(mapgenid, params, emerge)
{
	// NOTE: MapgenValleys has a hard dependency on BiomeGenOriginal
	this->m_bgen = (BiomeGenOriginal *)biomegen;

	BiomeParamsOriginal *bp = (BiomeParamsOriginal *)params->bparams;

	this->spflags            = params->spflags;
	this->altitude_chill     = params->altitude_chill;
	this->large_cave_depth   = params->large_cave_depth;
	this->lava_features_lim  = rangelim(params->lava_features, 0, 10);
	this->massive_cave_depth = params->massive_cave_depth;
	this->river_depth_bed    = params->river_depth + 1.f;
	this->river_size_factor  = params->river_size / 100.f;
	this->water_features_lim = rangelim(params->water_features, 0, 10);
	this->cave_width         = params->cave_width;

	//// 2D Terrain noise
	noise_filler_depth       = new Noise(&params->np_filler_depth,       seed, csize.X, csize.Z);
	noise_inter_valley_slope = new Noise(&params->np_inter_valley_slope, seed, csize.X, csize.Z);
	noise_rivers             = new Noise(&params->np_rivers,             seed, csize.X, csize.Z);
	noise_terrain_height     = new Noise(&params->np_terrain_height,     seed, csize.X, csize.Z);
	noise_valley_depth       = new Noise(&params->np_valley_depth,       seed, csize.X, csize.Z);
	noise_valley_profile     = new Noise(&params->np_valley_profile,     seed, csize.X, csize.Z);

	//// 3D Terrain noise
	// 1-up 1-down overgeneration
	noise_inter_valley_fill = new Noise(&params->np_inter_valley_fill, seed, csize.X, csize.Y + 2, csize.Z);
	// 1-down overgeneraion
	noise_cave1             = new Noise(&params->np_cave1,             seed, csize.X, csize.Y + 1, csize.Z);
	noise_cave2             = new Noise(&params->np_cave2,             seed, csize.X, csize.Y + 1, csize.Z);
	noise_massive_caves     = new Noise(&params->np_massive_caves,     seed, csize.X, csize.Y + 1, csize.Z);

	this->humid_rivers       = (spflags & MGVALLEYS_HUMID_RIVERS);
	this->use_altitude_chill = (spflags & MGVALLEYS_ALT_CHILL);
	this->humidity_adjust    = bp->np_humidity.offset - 50.f;

	// a small chance of overflows if the settings are very high
	this->cave_water_max_height = water_level + MYMAX(0, water_features_lim - 4) * 50;
	this->lava_max_height       = water_level + MYMAX(0, lava_features_lim - 4) * 50;

	tcave_cache = new float[csize.Y + 2];
}


MapgenValleys::~MapgenValleys()
{
	delete noise_cave1;
	delete noise_cave2;
	delete noise_filler_depth;
	delete noise_inter_valley_fill;
	delete noise_inter_valley_slope;
	delete noise_rivers;
	delete noise_massive_caves;
	delete noise_terrain_height;
	delete noise_valley_depth;
	delete noise_valley_profile;

	delete[] tcave_cache;
}


MapgenValleysParams::MapgenValleysParams()
{
	spflags            = MGVALLEYS_HUMID_RIVERS | MGVALLEYS_ALT_CHILL;
	altitude_chill     = 90; // The altitude at which temperature drops by 20C.
	large_cave_depth   = -33;
	lava_features      = 0;  // How often water will occur in caves.
	massive_cave_depth = -256;  // highest altitude of massive caves
	river_depth        = 4;  // How deep to carve river channels.
	river_size         = 5;  // How wide to make rivers.
	water_features     = 0;  // How often water will occur in caves.
	cave_width         = 0.09;

	np_cave1              = NoiseParams(0,     12,   v3f(61,   61,   61),   52534, 3, 0.5,   2.0);
	np_cave2              = NoiseParams(0,     12,   v3f(67,   67,   67),   10325, 3, 0.5,   2.0);
	np_filler_depth       = NoiseParams(0.f,   1.2f, v3f(256,  256,  256),  1605,  3, 0.5f,  2.f);
	np_inter_valley_fill  = NoiseParams(0.f,   1.f,  v3f(256,  512,  256),  1993,  6, 0.8f,  2.f);
	np_inter_valley_slope = NoiseParams(0.5f,  0.5f, v3f(128,  128,  128),  746,   1, 1.f,   2.f);
	np_rivers             = NoiseParams(0.f,   1.f,  v3f(256,  256,  256),  -6050, 5, 0.6f,  2.f);
	np_massive_caves      = NoiseParams(0.f,   1.f,  v3f(768,  256,  768),  59033, 6, 0.63f, 2.f);
	np_terrain_height     = NoiseParams(-10.f, 50.f, v3f(1024, 1024, 1024), 5202,  6, 0.4f,  2.f);
	np_valley_depth       = NoiseParams(5.f,   4.f,  v3f(512,  512,  512),  -1914, 1, 1.f,   2.f);
	np_valley_profile     = NoiseParams(0.6f,  0.5f, v3f(512,  512,  512),  777,   1, 1.f,   2.f);
}


void MapgenValleysParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgvalleys_spflags",        spflags, flagdesc_mapgen_valleys);
	settings->getU16NoEx("mgvalleys_altitude_chill",     altitude_chill);
	settings->getS16NoEx("mgvalleys_large_cave_depth",   large_cave_depth);
	settings->getU16NoEx("mgvalleys_lava_features",      lava_features);
	settings->getS16NoEx("mgvalleys_massive_cave_depth", massive_cave_depth);
	settings->getU16NoEx("mgvalleys_river_depth",        river_depth);
	settings->getU16NoEx("mgvalleys_river_size",         river_size);
	settings->getU16NoEx("mgvalleys_water_features",     water_features);
	settings->getFloatNoEx("mgvalleys_cave_width",       cave_width);

	settings->getNoiseParams("mgvalleys_np_cave1",              np_cave1);
	settings->getNoiseParams("mgvalleys_np_cave2",              np_cave2);
	settings->getNoiseParams("mgvalleys_np_filler_depth",       np_filler_depth);
	settings->getNoiseParams("mgvalleys_np_inter_valley_fill",  np_inter_valley_fill);
	settings->getNoiseParams("mgvalleys_np_inter_valley_slope", np_inter_valley_slope);
	settings->getNoiseParams("mgvalleys_np_rivers",             np_rivers);
	settings->getNoiseParams("mgvalleys_np_massive_caves",      np_massive_caves);
	settings->getNoiseParams("mgvalleys_np_terrain_height",     np_terrain_height);
	settings->getNoiseParams("mgvalleys_np_valley_depth",       np_valley_depth);
	settings->getNoiseParams("mgvalleys_np_valley_profile",     np_valley_profile);
}


void MapgenValleysParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgvalleys_spflags",        spflags, flagdesc_mapgen_valleys, U32_MAX);
	settings->setU16("mgvalleys_altitude_chill",     altitude_chill);
	settings->setS16("mgvalleys_large_cave_depth",   large_cave_depth);
	settings->setU16("mgvalleys_lava_features",      lava_features);
	settings->setS16("mgvalleys_massive_cave_depth", massive_cave_depth);
	settings->setU16("mgvalleys_river_depth",        river_depth);
	settings->setU16("mgvalleys_river_size",         river_size);
	settings->setU16("mgvalleys_water_features",     water_features);
	settings->setFloat("mgvalleys_cave_width",       cave_width);

	settings->setNoiseParams("mgvalleys_np_cave1",              np_cave1);
	settings->setNoiseParams("mgvalleys_np_cave2",              np_cave2);
	settings->setNoiseParams("mgvalleys_np_filler_depth",       np_filler_depth);
	settings->setNoiseParams("mgvalleys_np_inter_valley_fill",  np_inter_valley_fill);
	settings->setNoiseParams("mgvalleys_np_inter_valley_slope", np_inter_valley_slope);
	settings->setNoiseParams("mgvalleys_np_rivers",             np_rivers);
	settings->setNoiseParams("mgvalleys_np_massive_caves",      np_massive_caves);
	settings->setNoiseParams("mgvalleys_np_terrain_height",     np_terrain_height);
	settings->setNoiseParams("mgvalleys_np_valley_depth",       np_valley_depth);
	settings->setNoiseParams("mgvalleys_np_valley_profile",     np_valley_profile);
}


///////////////////////////////////////


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

	this->generating = true;
	this->vm = data->vmanip;
	this->ndef = data->nodedef;

	//TimeTaker t("makeChunk");

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate biome noises.  Note this must be executed strictly before
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
	MgStoneType stone_type = generateBiomes();

	// Cave creation.
	if (flags & MG_CAVES)
		generateCaves(stone_surface_max_y, large_cave_depth);

	// Dungeon creation
	if ((flags & MG_DUNGEONS) && node_max.Y < 50)
		generateDungeons(stone_surface_max_y, stone_type);

	// Generate the registered decorations
	if (flags & MG_DECORATIONS)
		m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Generate the registered ores
	m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	dustTopNodes();

	//TimeTaker tll("liquid_lighting");

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT)
		calcLighting(
				node_min - v3s16(0, 1, 0),
				node_max + v3s16(0, 1, 0),
				full_node_min,
				full_node_max);

	//mapgen_profiler->avg("liquid_lighting", tll.stop() / 1000.f);
	//mapgen_profiler->avg("makeChunk", t.stop() / 1000.f);

	this->generating = false;
}


// Populate the noise tables and do most of the
// calculation necessary to determine terrain height.
void MapgenValleys::calculateNoise()
{
	//TimeTaker t("calculateNoise", NULL, PRECISION_MICRO);

	int x = node_min.X;
	int y = node_min.Y - 1;
	int z = node_min.Z;

	//TimeTaker tcn("actualNoise");

	noise_inter_valley_slope->perlinMap2D(x, z);
	noise_rivers->perlinMap2D(x, z);
	noise_terrain_height->perlinMap2D(x, z);
	noise_valley_depth->perlinMap2D(x, z);
	noise_valley_profile->perlinMap2D(x, z);

	noise_inter_valley_fill->perlinMap3D(x, y, z);

	//mapgen_profiler->avg("noisemaps", tcn.stop() / 1000.f);

	float heat_offset = 0.f;
	float humidity_scale = 1.f;

	// Altitude chill tends to reduce the average heat.
	if (use_altitude_chill)
		heat_offset = 5.f;

	// River humidity tends to increase the humidity range.
	if (humid_rivers) {
		humidity_scale = 0.8f;
	}

	for (s32 index = 0; index < csize.X * csize.Z; index++) {
		m_bgen->heatmap[index] += heat_offset;
		m_bgen->humidmap[index] *= humidity_scale;
	}

	TerrainNoise tn;

	u32 index = 0;
	for (tn.z = node_min.Z; tn.z <= node_max.Z; tn.z++)
	for (tn.x = node_min.X; tn.x <= node_max.X; tn.x++, index++) {
		// The parameters that we actually need to generate terrain
		//  are passed by address (and the return value).
		tn.terrain_height    = noise_terrain_height->result[index];
		// River noise is replaced with base terrain, which
		// is basically the height of the water table.
		tn.rivers            = &noise_rivers->result[index];
		// Valley depth noise is replaced with the valley
		// number that represents the height of terrain
		// over rivers and is used to determine about
		// how close a river is for humidity calculation.
		tn.valley            = &noise_valley_depth->result[index];
		tn.valley_profile    = noise_valley_profile->result[index];
		// Slope noise is replaced by the calculated slope
		// which is used to get terrain height in the slow
		// method, to create sharper mountains.
		tn.slope             = &noise_inter_valley_slope->result[index];
		tn.inter_valley_fill = noise_inter_valley_fill->result[index];

		// This is the actual terrain height.
		float mount = terrainLevelFromNoise(&tn);
		noise_terrain_height->result[index] = mount;
	}
}


// This keeps us from having to maintain two similar sets of
//  complicated code to determine ground level.
float MapgenValleys::terrainLevelFromNoise(TerrainNoise *tn)
{
	// The square function changes the behaviour of this noise:
	//  very often small, and sometimes very high.
	float valley_d = MYSQUARE(*tn->valley);

	// valley_d is here because terrain is generally higher where valleys
	//  are deep (mountains). base represents the height of the
	//  rivers, most of the surface is above.
	float base = tn->terrain_height + valley_d;

	// "river" represents the distance from the river, in arbitrary units.
	float river = fabs(*tn->rivers) - river_size_factor;

	// Use the curve of the function 1-exp(-(x/a)^2) to model valleys.
	//  Making "a" vary (0 < a <= 1) changes the shape of the valleys.
	//  Try it with a geometry software !
	//   (here x = "river" and a = valley_profile).
	//  "valley" represents the height of the terrain, from the rivers.
	{
		float t = river / tn->valley_profile;
		*tn->valley = valley_d * (1.f - exp(- MYSQUARE(t)));
	}

	// approximate height of the terrain at this point
	float mount = base + *tn->valley;

	*tn->slope *= *tn->valley;

	// Rivers are placed where "river" is negative, so where the original
	//  noise value is close to zero.
	// Base ground is returned as rivers since it's basically the water table.
	*tn->rivers = base;
	if (river < 0.f) {
		// Use the the function -sqrt(1-x^2) which models a circle.
		float depth;
		{
			float t = river / river_size_factor + 1;
			depth = (river_depth_bed * sqrt(MYMAX(0, 1.f - MYSQUARE(t))));
		}

		// base - depth : height of the bottom of the river
		// water_level - 3 : don't make rivers below 3 nodes under the surface
		// We use three because that's as low as the swamp biomes go.
		// There is no logical equivalent to this using rangelim.
		mount = MYMIN(MYMAX(base - depth, (float)(water_level - 3)), mount);

		// Slope has no influence on rivers.
		*tn->slope = 0.f;
	}

	return mount;
}


// This avoids duplicating the code in terrainLevelFromNoise, adding
// only the final step of terrain generation without a noise map.
float MapgenValleys::adjustedTerrainLevelFromNoise(TerrainNoise *tn)
{
	float mount = terrainLevelFromNoise(tn);
	s16 y_start = myround(mount);

	for (s16 y = y_start; y <= y_start + 1000; y++) {
		float fill = NoisePerlin3D(&noise_inter_valley_fill->np, tn->x, y, tn->z, seed);

		if (fill * *tn->slope < y - mount) {
			mount = MYMAX(y - 1, mount);
			break;
		}
	}

	return mount;
}


int MapgenValleys::getSpawnLevelAtPoint(v2s16 p)
{
	// Check to make sure this isn't a request for a location in a river.
	float rivers = NoisePerlin2D(&noise_rivers->np, p.X, p.Y, seed);
	if (fabs(rivers) < river_size_factor)
		return MAX_MAP_GENERATION_LIMIT;  // Unsuitable spawn point

	s16 level_at_point = terrainLevelAtPoint(p.X, p.Y);
	if (level_at_point <= water_level ||
			level_at_point > water_level + 32)
		return MAX_MAP_GENERATION_LIMIT;  // Unsuitable spawn point
	else
		return level_at_point;
}


float MapgenValleys::terrainLevelAtPoint(s16 x, s16 z)
{
	TerrainNoise tn;

	float rivers = NoisePerlin2D(&noise_rivers->np, x, z, seed);
	float valley = NoisePerlin2D(&noise_valley_depth->np, x, z, seed);
	float inter_valley_slope = NoisePerlin2D(&noise_inter_valley_slope->np, x, z, seed);

	tn.x                 = x;
	tn.z                 = z;
	tn.terrain_height    = NoisePerlin2D(&noise_terrain_height->np, x, z, seed);
	tn.rivers            = &rivers;
	tn.valley            = &valley;
	tn.valley_profile    = NoisePerlin2D(&noise_valley_profile->np, x, z, seed);
	tn.slope             = &inter_valley_slope;
	tn.inter_valley_fill = 0.f;

	return adjustedTerrainLevelFromNoise(&tn);
}


int MapgenValleys::generateTerrain()
{
	// Raising this reduces the rate of evaporation.
	static const float evaporation = 300.f;
	// from the lua
	static const float humidity_dropoff = 4.f;
	// constant to convert altitude chill (compatible with lua) to heat
	static const float alt_to_heat = 20.f;
	// humidity reduction by altitude
	static const float alt_to_humid = 10.f;

	MapNode n_air(CONTENT_AIR);
	MapNode n_river_water(c_river_water_source);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	v3s16 em = vm->m_area.getExtent();
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
			surface_max_y = ceil(surface_y);

		if (humid_rivers) {
			// Derive heat from (base) altitude. This will be most correct
			// at rivers, since other surface heights may vary below.
			if (use_altitude_chill && (surface_y > 0.f || river_y > 0.f))
				t_heat -= alt_to_heat * MYMAX(surface_y, river_y) / altitude_chill;

			// If humidity is low or heat is high, lower the water table.
			float delta = m_bgen->humidmap[index_2d] - 50.f;
			if (delta < 0.f) {
				float t_evap = (t_heat - 32.f) / evaporation;
				river_y += delta * MYMAX(t_evap, 0.08f);
			}
		}

		u32 index_3d = (z - node_min.Z) * zstride_1u1d + (x - node_min.X);
		u32 index_data = vm->m_area.index(x, node_min.Y - 1, z);

		// Mapgens concern themselves with stone and water.
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[index_data].getContent() == CONTENT_IGNORE) {
				float fill = noise_inter_valley_fill->result[index_3d];
				float surface_delta = (float)y - surface_y;
				bool river = y + 1 < river_y;

				if (slope * fill > surface_delta) {
					// ground
					vm->m_data[index_data] = n_stone;
					if (y > heightmap[index_2d])
						heightmap[index_2d] = y;
					if (y > surface_max_y)
						surface_max_y = y;
				} else if (y <= water_level) {
					// sea
					vm->m_data[index_data] = n_water;
				} else if (river) {
					// river
					vm->m_data[index_data] = n_river_water;
				} else {  // air
					vm->m_data[index_data] = n_air;
				}
			}

			vm->m_area.add_y(em, index_data, 1);
			index_3d += ystride;
		}

		if (heightmap[index_2d] == -MAX_MAP_GENERATION_LIMIT) {
			s16 surface_y_int = myround(surface_y);
			if (surface_y_int > node_max.Y + 1 || surface_y_int < node_min.Y - 1) {
				// If surface_y is outside the chunk, it's good enough.
				heightmap[index_2d] = surface_y_int;
			} else {
				// If the ground is outside of this chunk, but surface_y
				// is within the chunk, give a value outside.
				heightmap[index_2d] = node_min.Y - 2;
			}
		}

		if (humid_rivers) {
			// Use base ground (water table) in a riverbed, to
			// avoid an unnatural rise in humidity.
			float t_alt = MYMAX(noise_rivers->result[index_2d], (float)heightmap[index_2d]);
			float humid = m_bgen->humidmap[index_2d];
			float water_depth = (t_alt - river_y) / humidity_dropoff;
			humid *= 1.f + pow(0.5f, MYMAX(water_depth, 1.f));

			// Reduce humidity with altitude (ignoring riverbeds).
			// This is similar to the lua version's seawater adjustment,
			// but doesn't increase the base humidity, which causes
			// problems with the default biomes.
			if (t_alt > 0.f)
				humid -= alt_to_humid * t_alt / altitude_chill;

			m_bgen->humidmap[index_2d] = humid;
		}

		// Assign the heat adjusted by any changed altitudes.
		// The altitude will change about half the time.
		if (use_altitude_chill) {
			// ground height ignoring riverbeds
			float t_alt = MYMAX(noise_rivers->result[index_2d], (float)heightmap[index_2d]);
			if (humid_rivers && heightmap[index_2d] == (s16)myround(surface_y))
				// The altitude hasn't changed. Use the first result.
				m_bgen->heatmap[index_2d] = t_heat;
			else if (t_alt > 0.f)
				m_bgen->heatmap[index_2d] -= alt_to_heat * t_alt / altitude_chill;
		}
	}

	return surface_max_y;
}

void MapgenValleys::generateCaves(s16 max_stone_y, s16 large_cave_depth)
{
	if (max_stone_y < node_min.Y)
		return;

	noise_cave1->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_cave2->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	PseudoRandom ps(blockseed + 72202);

	MapNode n_air(CONTENT_AIR);
	MapNode n_lava(c_lava_source);
	MapNode n_water(c_river_water_source);

	v3s16 em = vm->m_area.getExtent();

	// Cave blend distance near YMIN, YMAX
	const float massive_cave_blend = 128.f;
	// noise threshold for massive caves
	const float massive_cave_threshold = 0.6f;
	// mct: 1 = small rare caves, 0.5 1/3rd ground volume, 0 = 1/2 ground volume.

	float yblmin = -mapgen_limit + massive_cave_blend * 1.5f;
	float yblmax = massive_cave_depth - massive_cave_blend * 1.5f;
	bool made_a_big_one = false;

	// Cache the tcave values as they only vary by altitude.
	if (node_max.Y <= massive_cave_depth) {
		noise_massive_caves->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

		for (s16 y = node_min.Y - 1; y <= node_max.Y; y++) {
			float tcave = massive_cave_threshold;

			if (y < yblmin) {
				float t = (yblmin - y) / massive_cave_blend;
				tcave += MYSQUARE(t);
			} else if (y > yblmax) {
				float t = (y - yblmax) / massive_cave_blend;
				tcave += MYSQUARE(t);
			}

			tcave_cache[y - node_min.Y + 1] = tcave;
		}
	}

	// lava_depth varies between one and ten as you approach
	//  the bottom of the world.
	s16 lava_depth = ceil((lava_max_height - node_min.Y + 1) * 10.f / mapgen_limit);
	// This allows random lava spawns to be less common at the surface.
	s16 lava_chance = MYCUBE(lava_features_lim) * lava_depth;
	// water_depth varies between ten and one on the way down.
	s16 water_depth = ceil((mapgen_limit - abs(node_min.Y) + 1) * 10.f / mapgen_limit);
	// This allows random water spawns to be more common at the surface.
	s16 water_chance = MYCUBE(water_features_lim) * water_depth;

	// Reduce the odds of overflows even further.
	if (node_max.Y > water_level) {
		lava_chance /= 3;
		water_chance /= 3;
	}

	u32 index_2d = 0;
	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index_2d++) {
		Biome *biome = (Biome *)m_bmgr->getRaw(biomemap[index_2d]);
		bool tunnel_air_above = false;
		bool is_under_river = false;
		bool underground = false;
		u32 index_data = vm->m_area.index(x, node_max.Y, z);
		u32 index_3d = (z - node_min.Z) * zstride_1d + csize.Y * ystride + (x - node_min.X);

		// Dig caves on down loop to check for air above.
		// Don't excavate the overgenerated stone at node_max.Y + 1,
		// this creates a 'roof' over the tunnel, preventing light in
		// tunnels at mapchunk borders when generating mapchunks upwards.
		// This 'roof' is removed when the mapchunk above is generated.
		for (s16 y = node_max.Y; y >= node_min.Y - 1; y--,
				index_3d -= ystride,
				vm->m_area.add_y(em, index_data, -1)) {

			float terrain = noise_terrain_height->result[index_2d];

			// Saves some time.
			if (y > terrain + 10)
				continue;
			else if (y < terrain - 40)
				underground = true;

			// Dig massive caves.
			if (node_max.Y <= massive_cave_depth
					&& noise_massive_caves->result[index_3d]
					> tcave_cache[y - node_min.Y + 1]) {
				vm->m_data[index_data] = n_air;
				made_a_big_one = true;
				continue;
			}

			content_t c = vm->m_data[index_data].getContent();
			// Detect river water to place riverbed nodes in tunnels
			if (c == biome->c_river_water)
				is_under_river = true;

			float d1 = contour(noise_cave1->result[index_3d]);
			float d2 = contour(noise_cave2->result[index_3d]);

			if (d1 * d2 > cave_width && ndef->get(c).is_ground_content) {
				// in a tunnel
				vm->m_data[index_data] = n_air;
				tunnel_air_above = true;
			} else if (c == biome->c_filler || c == biome->c_stone) {
				if (tunnel_air_above) {
					// at the tunnel floor
					s16 sr = ps.range(0, 39);
					u32 j = index_data;
					vm->m_area.add_y(em, j, 1);

					if (sr > terrain - y) {
						// Put biome nodes in tunnels near the surface
						if (is_under_river)
							vm->m_data[index_data] = MapNode(biome->c_riverbed);
						else if (underground)
							vm->m_data[index_data] = MapNode(biome->c_filler);
						else
							vm->m_data[index_data] = MapNode(biome->c_top);
					} else if (sr < 3 && underground) {
						sr = abs(ps.next());
						if (lava_features_lim > 0 && y <= lava_max_height
								&& c == biome->c_stone && sr < lava_chance)
							vm->m_data[j] = n_lava;

						sr -= lava_chance;

						// If sr < 0 then we should have already placed lava --
						// don't immediately dump water on it.
						if (water_features_lim > 0 && y <= cave_water_max_height
								&& sr >= 0 && sr < water_chance)
							vm->m_data[j] = n_water;
					}
				}

				tunnel_air_above = false;
				underground = true;
			} else {
				tunnel_air_above = false;
			}
		}
	}

	if (node_max.Y <= large_cave_depth && !made_a_big_one) {
		u32 bruises_count = ps.range(0, 2);
		for (u32 i = 0; i < bruises_count; i++) {
			CavesRandomWalk cave(ndef, &gennotify, seed, water_level,
				c_water_source, c_lava_source);

			cave.makeCave(vm, node_min, node_max, &ps, true, max_stone_y, heightmap);
		}
	}
}
