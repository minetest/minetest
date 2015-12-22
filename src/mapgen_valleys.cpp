/*
Minetest Valleys C
Copyright (C) Duane Robertson <duane@duanerobertson.com>

Based on Valleys Mapgen by Gael de Sailly
 (https://forum.minetest.net/viewtopic.php?f=9&t=11430)
and mapgen_v7 by kwolekr, Ryan Kwolek <kwolekr@minetest.net>.

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
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3.0 of the License, or
(at your option) any later version.
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
#include "treegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_valleys.h"

#include "util/timetaker.h"
#include "profiler.h"
//#undef NDEBUG
//#include "assert.h"


static Profiler mapgen_prof;
Profiler *mapgen_profiler = &mapgen_prof;

FlagDesc flagdesc_mapgen_valleys[] = {
	{"cliffs", MG_VALLEYS_CLIFFS},
	{"rugged", MG_VALLEYS_RUGGED},
	{NULL,        0}
};

///////////////////////////////////////////////////////////////////////////////


MapgenValleys::MapgenValleys(int mapgenid, MapgenParams *params, EmergeManager *emerge)
	: Mapgen(mapgenid, params, emerge)
{
	this->m_emerge = emerge;
	this->bmgr = emerge->biomemgr;

	//// amount of elements to skip for the next index
	//// for noise/height/biome maps (not vmanip)
	this->ystride = csize.X;
	this->zstride = csize.X * (csize.Y + 2);

	this->biomemap        = new u8[csize.X * csize.Z];
	this->heightmap       = new s16[csize.X * csize.Z];
	this->heatmap         = NULL;
	this->humidmap        = NULL;

	MapgenValleysParams *sp = (MapgenValleysParams *)params->sparams;
	this->spflags = sp->spflags;

	river_size = (float)(sp->river_size) / 100;
	river_depth = sp->river_depth + 1;
	water_level = params->water_level;
	altitude_chill = sp->altitude_chill;
	lava_max_height = sp->lava_max_height;

	humidity_adjust = sp->humidity - 50;
	temperature_adjust = sp->temperature - 50;

	//// Terrain noise
	noise_filler_depth = new Noise(&sp->np_filler_depth,    seed, csize.X, csize.Z);
	noise_simple_caves_1 = new Noise(&sp->np_simple_caves_1, seed, csize.X, csize.Y + 2, csize.Z);
	noise_simple_caves_2 = new Noise(&sp->np_simple_caves_2, seed, csize.X, csize.Y + 2, csize.Z);
	noise_cliffs = new Noise(&sp->np_cliffs, seed, csize.X, csize.Z);
	noise_corr = new Noise(&sp->np_corr, seed, csize.X, csize.Z);

	//// Biome noise
	noise_heat           = new Noise(&params->np_biome_heat,           seed, csize.X, csize.Z);
	noise_humidity       = new Noise(&params->np_biome_humidity,       seed, csize.X, csize.Z);
	noise_heat_blend     = new Noise(&params->np_biome_heat_blend,     seed, csize.X, csize.Z);
	noise_humidity_blend = new Noise(&params->np_biome_humidity_blend, seed, csize.X, csize.Z);

	// VMG noises
	noise_terrain_height = new Noise(&sp->np_terrain_height, seed, csize.X, csize.Z);
	noise_rivers = new Noise(&sp->np_rivers, seed, csize.X, csize.Z);
	noise_valley_depth = new Noise(&sp->np_valley_depth, seed, csize.X, csize.Z);
	noise_valley_profile = new Noise(&sp->np_valley_profile, seed, csize.X, csize.Z);
	noise_inter_valley_slope = new Noise(&sp->np_inter_valley_slope, seed, csize.X, csize.Z);
	noise_inter_valley_fill = new Noise(&sp->np_inter_valley_fill, seed, csize.X, csize.Y, csize.Z);

	//// Resolve nodes to be used
	INodeDefManager *ndef = emerge->ndef;

	c_stone                = ndef->getId("mapgen_stone");
	c_water_source         = ndef->getId("mapgen_water_source");
	c_lava_source          = ndef->getId("mapgen_lava_source");
	c_desert_stone         = ndef->getId("mapgen_desert_stone");
	c_sandstone            = ndef->getId("mapgen_sandstone");
	c_river_water_source   = ndef->getId("mapgen_river_water_source");
	c_sand                 = ndef->getId("mapgen_sand");

	c_cobble               = ndef->getId("mapgen_cobble");
	c_stair_cobble         = ndef->getId("mapgen_stair_cobble");
	c_mossycobble          = ndef->getId("mapgen_mossycobble");
	c_sandstonebrick       = ndef->getId("mapgen_sandstonebrick");
	c_stair_sandstonebrick = ndef->getId("mapgen_stair_sandstonebrick");

	c_dirt                 = ndef->getId("mapgen_dirt");

	if (c_mossycobble == CONTENT_IGNORE)
		c_mossycobble = c_cobble;
	if (c_stair_cobble == CONTENT_IGNORE)
		c_stair_cobble = c_cobble;
	if (c_sandstonebrick == CONTENT_IGNORE)
		c_sandstonebrick = c_sandstone;
	if (c_stair_sandstonebrick == CONTENT_IGNORE)
		c_stair_sandstonebrick = c_sandstone;
	if (c_river_water_source == CONTENT_IGNORE)
		c_river_water_source = c_water_source;
	if (c_sand == CONTENT_IGNORE)
		c_sand = c_stone;
}


MapgenValleys::~MapgenValleys()
{
	delete noise_filler_depth;

	delete noise_heat;
	delete noise_humidity;
	delete noise_heat_blend;
	delete noise_humidity_blend;

	delete noise_terrain_height;
	delete noise_rivers;
	delete noise_valley_depth;
	delete noise_valley_profile;
	delete noise_inter_valley_slope;
	delete noise_inter_valley_fill;
	delete noise_cliffs;
	delete noise_corr;
	delete noise_simple_caves_1;
	delete noise_simple_caves_2;

	delete[] heightmap;
	delete[] biomemap;
}


MapgenValleysParams::MapgenValleysParams()
{
	spflags = MG_VALLEYS_CLIFFS | MG_VALLEYS_RUGGED;

	np_filler_depth = NoiseParams(0, 1.2, v3f(150, 150, 150), 261, 3, 0.7, 2.0);
	np_simple_caves_1 = NoiseParams(0, 1, v3f(64, 64, 64), -8402, 3, 0.5, 2.0);
	np_simple_caves_2 = NoiseParams(0, 1, v3f(64, 64, 64), 3944, 3, 0.5, 2.0);
	np_cliffs = NoiseParams(0, 1, v3f(750, 750, 750), 8445, 5, 1.0, 2.0);
	np_corr = NoiseParams(0, 1, v3f(40, 40, 40), -3536, 4, 1.0, 2.0);

	// vmg noises
	np_terrain_height = NoiseParams(-10, 50, v3f(1024, 1024, 1024), 5202, 6, 0.4, 2.0);
	np_rivers = NoiseParams(0, 1, v3f(256, 256, 256), -6050, 5, 0.6, 2.0);
	np_valley_depth = NoiseParams(5, 4, v3f(512, 512, 512), -1914, 1, 1.0, 2.0);
	np_valley_profile = NoiseParams(0.6, 0.5, v3f(512, 512, 512), 777, 1, 1.0, 2.0);
	np_inter_valley_slope = NoiseParams(0.5, 0.5, v3f(128, 128, 128), 746, 1, 1.0, 2.0);
	np_inter_valley_fill = NoiseParams(0, 1, v3f(256, 256, 256), 1993, 6, 0.8, 2.0);
}


void MapgenValleysParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mg_valleys_spflags", spflags, flagdesc_mapgen_valleys);

	settings->getNoiseParams("mg_valleys_np_filler_depth",    np_filler_depth);
	settings->getNoiseParams("mg_valleys_np_simple_caves_1",    np_simple_caves_1);
	settings->getNoiseParams("mg_valleys_np_simple_caves_2",    np_simple_caves_2);
	
	if (!settings->getS16NoEx("mg_valleys_temperature", temperature))
		temperature = 50;  // in Fahrenheit, unfortunately
	if (!settings->getS16NoEx("mg_valleys_humidity", humidity))
		humidity = 50;  // as a percentage
	if (!settings->getS16NoEx("mg_valleys_river_size", river_size))
		river_size = 5;  // How wide to make rivers.
	if (!settings->getS16NoEx("mg_valleys_river_depth", river_depth))
		river_depth = 4;  // How deep to carve river channels.
	if (!settings->getS16NoEx("mg_valleys_altitude_chill", altitude_chill))
		altitude_chill = 90;  // The altitude at which temperature drops by 20C.
	if (!settings->getS16NoEx("mg_valleys_lava_max_height", lava_max_height))
		lava_max_height = 0;  // Lava will never be higher than this.
	if (!settings->getS16NoEx("mg_valleys_cave_water_max_height", cave_water_height))
		 cave_water_height = MAX_MAP_GENERATION_LIMIT;  // Water in caves will never be higher than this.

	// vmg noises
	settings->getNoiseParams("mg_valleys_np_terrain_height",    np_terrain_height);
	settings->getNoiseParams("mg_valleys_np_cliffs",    np_cliffs);
	settings->getNoiseParams("mg_valleys_np_corr",    np_corr);
	settings->getNoiseParams("mg_valleys_np_rivers",    np_rivers);
	settings->getNoiseParams("mg_valleys_np_valley_depth",    np_valley_depth);
	settings->getNoiseParams("mg_valleys_np_valley_profile",    np_valley_profile);
	settings->getNoiseParams("mg_valleys_np_inter_valley_slope",    np_inter_valley_slope);
	settings->getNoiseParams("mg_valleys_np_inter_valley_fill",    np_inter_valley_fill);
}


void MapgenValleysParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mg_valleys_spflags", spflags, flagdesc_mapgen_valleys, U32_MAX);

	settings->setNoiseParams("mg_valleys_np_filler_depth",    np_filler_depth);
	settings->setNoiseParams("mg_valleys_np_simple_caves_1",    np_simple_caves_1);
	settings->setNoiseParams("mg_valleys_np_simple_caves_2",    np_simple_caves_2);
	
	settings->setS16("mg_valleys_temperature", temperature);
	settings->setS16("mg_valleys_humidity", humidity);
	settings->setS16("mg_valleys_river_size", river_size);
	settings->setS16("mg_valleys_river_depth", river_depth);
	settings->setS16("mg_valleys_altitude_chill", altitude_chill);
	settings->setS16("mg_valleys_lava_max_height", lava_max_height);
	settings->setS16("mg_valleys_cave_water_max_height", cave_water_height);

	// vmg noises
	settings->setNoiseParams("mg_valleys_np_terrain_height",    np_terrain_height);
	settings->setNoiseParams("mg_valleys_np_cliffs",    np_cliffs);
	settings->setNoiseParams("mg_valleys_np_corr",    np_corr);
	settings->setNoiseParams("mg_valleys_np_rivers",    np_rivers);
	settings->setNoiseParams("mg_valleys_np_valley_depth",    np_valley_depth);
	settings->setNoiseParams("mg_valleys_np_valley_profile",    np_valley_profile);
	settings->setNoiseParams("mg_valleys_np_inter_valley_slope",    np_inter_valley_slope);
	settings->setNoiseParams("mg_valleys_np_inter_valley_fill",    np_inter_valley_fill);
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
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	TimeTaker t("makeChunk");

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	blockseed = getBlockSeed2(full_node_min, seed);

	// Generate noise maps and base terrain height.
	calculateNoise();

	// Generate base terrain with initial heightmaps
	s16 stone_surface_max_y = generateTerrain();

	// Create biomemap at heightmap surface
	calcBiomes(csize.X, csize.Z, heatmap, humidmap, heightmap, biomemap);

	// Actually place the biome-specific nodes
	MgStoneType stone_type = generateBiomes(heatmap, humidmap);

	if (flags & MG_CAVES)
		generateSimpleCaves(stone_surface_max_y);

	if ((flags & MG_DUNGEONS) && node_max.Y < 50 && (stone_surface_max_y >= node_min.Y)) {
		DungeonParams dp;

		dp.np_rarity  = nparams_dungeon_rarity;
		dp.np_density = nparams_dungeon_density;
		dp.np_wetness = nparams_dungeon_wetness;
		dp.c_water    = c_water_source;
		if (stone_type == STONE) {
			dp.c_cobble = c_cobble;
			dp.c_moss   = c_mossycobble;
			dp.c_stair  = c_stair_cobble;

			dp.diagonal_dirs = false;
			dp.mossratio     = 3.0;
			dp.holesize      = v3s16(1, 2, 1);
			dp.roomsize      = v3s16(0, 0, 0);
			dp.notifytype    = GENNOTIFY_DUNGEON;
		} else if (stone_type == DESERT_STONE) {
			dp.c_cobble = c_desert_stone;
			dp.c_moss   = c_desert_stone;
			dp.c_stair  = c_desert_stone;

			dp.diagonal_dirs = true;
			dp.mossratio     = 0.0;
			dp.holesize      = v3s16(2, 3, 2);
			dp.roomsize      = v3s16(2, 5, 2);
			dp.notifytype    = GENNOTIFY_TEMPLE;
		} else if (stone_type == SANDSTONE) {
			dp.c_cobble = c_sandstonebrick;
			dp.c_moss   = c_sandstonebrick;
			dp.c_stair  = c_sandstonebrick;

			dp.diagonal_dirs = false;
			dp.mossratio     = 0.0;
			dp.holesize      = v3s16(2, 2, 2);
			dp.roomsize      = v3s16(2, 0, 2);
			dp.notifytype    = GENNOTIFY_DUNGEON;
		}

		DungeonGen dgen(this, &dp);
		dgen.generate(blockseed, full_node_min, full_node_max);
	}

	// Generate the registered decorations
	m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Generate the registered ores
	m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	dustTopNodes();

	TimeTaker tll("liquid_lighting");

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0), full_node_min, full_node_max);

	mapgen_profiler->avg("liquid_lighting", tll.stop() / 1000.f);
	mapgen_profiler->avg("makeChunk", t.stop() / 1000.f);

	this->generating = false;
}


void MapgenValleys::calcBiomes(s16 sx, s16 sy, float *heat_map, float *humidity_map, s16 *height_map, u8 *biomeid_map)
{
	for (s32 index = 0; index < sx * sy; index++) {
		// Both heat and humidity have already been adjusted for altitude.
		Biome *biome = bmgr->getBiome(heat_map[index], humidity_map[index], height_map[index]);
		biomeid_map[index] = biome->index;
	}
}


void MapgenValleys::calculateNoise()
{
	//TimeTaker t("calculateNoise", NULL, PRECISION_MICRO);
	int x = node_min.X;
	int y = node_min.Y - 1;
	int z = node_min.Z;

	TimeTaker tcn("actualNoise");

	noise_filler_depth->perlinMap2D(x, z);
	noise_heat->perlinMap2D(x, z);
	noise_humidity->perlinMap2D(x, z);
	noise_heat_blend->perlinMap2D(x, z);
	noise_humidity_blend->perlinMap2D(x, z);

	// vmg noises
	noise_terrain_height->perlinMap2D(x, z);
	noise_rivers->perlinMap2D(x, z);
	noise_valley_depth->perlinMap2D(x, z);
	noise_valley_profile->perlinMap2D(x, z);
	noise_inter_valley_slope->perlinMap2D(x, z);
	noise_inter_valley_fill->perlinMap2D(x, z);
	if (spflags & MG_VALLEYS_CLIFFS)
		noise_cliffs->perlinMap2D(x, z);
	if (spflags & MG_VALLEYS_RUGGED)
		noise_corr->perlinMap2D(x, z);

	if (flags & MG_CAVES) {
		noise_simple_caves_1->perlinMap3D(x, y, z);
		noise_simple_caves_2->perlinMap3D(x, y, z);
	}

	mapgen_profiler->avg("noisemaps", tcn.stop() / 1000.f);

	for (s32 index = 0; index < csize.X * csize.Z; index++) {
		noise_heat->result[index] += noise_heat_blend->result[index];
		noise_heat->result[index] += temperature_adjust;
		noise_humidity->result[index] += noise_humidity_blend->result[index];
	}

	float mount, valley, rivers, cliffs, corr;
	valley = cliffs = corr = 0.0;
	u32 index = 0;
	for (s16 zi = node_min.Z; zi <= node_max.Z; zi++)
	for (s16 xi = node_min.X; xi <= node_max.X; xi++, index++) {
		if (spflags & MG_VALLEYS_CLIFFS)
			cliffs = noise_cliffs->result[index];
		if (spflags & MG_VALLEYS_RUGGED)
			corr = noise_corr->result[index];

		// The two parameters that we actually need to generate terrain
		//  are terrain height and river noise.
		rivers = noise_rivers->result[index];
		mount = baseGroundFromNoise(xi, zi, noise_valley_depth->result[index], noise_terrain_height->result[index], &rivers, noise_valley_profile->result[index], noise_inter_valley_slope->result[index], &valley, noise_inter_valley_fill->result[index], cliffs, corr);
		noise_terrain_height->result[index] = mount;
		noise_rivers->result[index] = rivers;

		// Assign the humidity adjusted by water proximity (and thus altitude).
		// I can't think of a reason why a mod would expect base humidity
		//  from noise or at any altitude other than ground level.
		noise_humidity->result[index] = humidityByTerrain(noise_humidity->result[index], mount, valley);

		// Assign the heat adjusted by altitude. See humidity, above.
		if (mount > 0)
			noise_heat->result[index] *= pow(0.5, mount / altitude_chill);
	}

	heatmap = noise_heat->result;
	humidmap = noise_humidity->result;
}


// This ugly function keeps us from having to maintain two similar sets of
//  complicated code to determine ground level.
float MapgenValleys::baseGroundFromNoise(s16 x, s16 z, float valley_depth, float terrain_height, float *rivers, float valley_profile, float inter_valley_slope, float *valley, float inter_valley_fill, float cliffs, float corr)
{
	// The square function changes the behaviour of this noise:
	//  very often small, and sometimes very high.
	float valley_d = pow(valley_depth, 2);

	// valley_d is here because terrain is generally higher where valleys
	//  are deep (mountains). base represents the height of the
	//  rivers, most of the surface is above.
	float base = terrain_height + valley_d;

	// "river" represents the distance from the river, in arbitrary units.
	float river = fabs(*rivers) - river_size;
	*rivers = -31000;

	// Use the curve of the function 1−exp(−(x/a)²) to model valleys.
	//  Making "a" vary (0 < a ≤ 1) changes the shape of the valleys.
	//  Try it with a geometry software !
	//   (here x = "river" and a = valley_profile).
	//  "valley" represents the height of the terrain, from the rivers.
	*valley = valley_d * (1 - exp(- pow(river / valley_profile, 2)));

	// approximate height of the terrain at this point
	float mount = base + *valley;

	float slope = *valley * inter_valley_slope;

	// Rivers are placed where "river" is negative, so where the original
	//  noise value is close to zero.
	if (river < 0) {
		// Use the the function −sqrt(1−x²) which models a circle.
		float depth = (river_depth * sqrt(1 - pow((river / river_size + 1), 2))) + 1;
		*rivers = base;

		// base - depth : height of the bottom of the river
		// water_level - 2 : don't make rivers below 2 nodes under the surface
		mount = fmin(fmax(base - depth, water_level - 2), mount);

		// Slope has no influence on rivers.
		slope = 0;
	}

	// The penultimate step builds up the heights, but we reduce it 
	//  occasionally to create cliffs.
	float delta = sin(inter_valley_fill) * slope;
	if (delta != 0) {
		if ((spflags & MG_VALLEYS_CLIFFS) && cliffs < 0.2)
			mount += delta;
		else
			mount += delta * 0.66;

		// Use yet another noise to make the heights look more rugged.
		if ((spflags & MG_VALLEYS_RUGGED) && mount > water_level && fabs(inter_valley_slope * inter_valley_fill) < 0.3)
			mount += (delta / fabs(delta)) * pow(fabs(delta), 0.5) * fabs(sin(corr));
	}

	return mount;
}


float MapgenValleys::humidityByTerrain(float humidity, float mount, float valley)
{
	// Although the original valleys adjusts humidity by distance
	// from seawater, this causes problems with the default biomes.
	// Adjust only by freshwater proximity.
	humidity += humidity_adjust;
	if (mount > water_level) {
		float river_water = pow(0.5, fmax(valley / 12, 0));
		humidity = fmax(humidity, (65 * river_water));
	}

	return humidity;
}


// This is very slow.
Biome *MapgenValleys::getBiomeAtPoint(v3s16 p)
{
	float heat = NoisePerlin2D(&noise_heat->np, p.X, p.Z, seed) + NoisePerlin2D(&noise_heat_blend->np, p.X, p.Z, seed);
	heat *= pow(0.5, p.Y / altitude_chill);

	float terrain_height = NoisePerlin2D(&noise_terrain_height->np, p.X, p.Z, seed);
	float rivers = NoisePerlin2D(&noise_rivers->np, p.X, p.Z, seed);
	float valley_depth = NoisePerlin2D(&noise_valley_depth->np, p.X, p.Z, seed);
	float valley_profile = NoisePerlin2D(&noise_valley_profile->np, p.X, p.Z, seed);
	float inter_valley_slope = NoisePerlin2D(&noise_inter_valley_slope->np, p.X, p.Z, seed);
	float valley = 0.0;
	float inter_valley_fill = NoisePerlin2D(&noise_inter_valley_fill->np, p.X, p.Z, seed);

	float cliffs = 0.0;
	if (spflags & MG_VALLEYS_CLIFFS)
		cliffs = NoisePerlin2D(&noise_cliffs->np, p.X, p.Z, seed);

	float corr = 0.0;
	if (spflags & MG_VALLEYS_RUGGED)
		corr = NoisePerlin2D(&noise_corr->np, p.X, p.Z, seed);

	float mount = baseGroundFromNoise(p.X, p.Z, valley_depth, terrain_height, &rivers, valley_profile, inter_valley_slope, &valley, inter_valley_fill, cliffs, corr);
	s16 groundlevel = (s16) mount;

	float humidity = NoisePerlin2D(&noise_humidity->np, p.X, p.Z, seed) + NoisePerlin2D(&noise_humidity_blend->np, p.X, p.Z, seed);
	humidity = humidityByTerrain(humidity, mount, valley);

	return bmgr->getBiome(heat, humidity, groundlevel);
}


int MapgenValleys::getGroundLevelAtPoint(v2s16 p)
{
	// Base terrain calculation
	s16 y = baseTerrainLevelAtPoint(p.X, p.Y);

	return y;
}


float MapgenValleys::baseTerrainLevelAtPoint(s16 x, s16 z)
{
	float terrain_height = NoisePerlin2D(&noise_terrain_height->np, x, z, seed);
	float rivers = NoisePerlin2D(&noise_rivers->np, x, z, seed);
	float valley_depth = NoisePerlin2D(&noise_valley_depth->np, x, z, seed);
	float valley_profile = NoisePerlin2D(&noise_valley_profile->np, x, z, seed);
	float inter_valley_slope = NoisePerlin2D(&noise_inter_valley_slope->np, x, z, seed);
	float inter_valley_fill = NoisePerlin2D(&noise_inter_valley_fill->np, x, z, seed);
	float valley = 0.0;

	float cliffs = 0.0;
	if (spflags & MG_VALLEYS_CLIFFS)
		cliffs = NoisePerlin2D(&noise_cliffs->np, x, z, seed);

	float corr = 0.0;
	if (spflags & MG_VALLEYS_RUGGED)
		corr = NoisePerlin2D(&noise_corr->np, x, z, seed);

	return MapgenValleys::baseGroundFromNoise(x, z, valley_depth, terrain_height, &rivers, valley_profile, inter_valley_slope, &valley, inter_valley_fill, cliffs, corr);
}


int MapgenValleys::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);
	MapNode n_river_water(c_river_water_source);
	MapNode n_sand(c_sand);

	v3s16 em = vm->m_area.getExtent();
	s16 surface_min_y = MAX_MAP_GENERATION_LIMIT;
	s16 surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		s16 river_y = (s16) noise_rivers->result[index];
		s16 surface_y = (s16) noise_terrain_height->result[index];

		heightmap[index] = surface_y;

		if (surface_y < surface_min_y)
			surface_min_y = surface_y;

		if (surface_y > surface_max_y)
			surface_max_y = surface_y;

		// Mapgens concern themselves with stone and water.
		u32 i = vm->m_area.index(x, node_min.Y - 1, z);
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (river_y > surface_y && y == surface_y + 1) {
					// river bottom
					vm->m_data[i] = n_sand;
				} else if (y <= surface_y)
					// ground
					vm->m_data[i] = n_stone;
				else if (river_y > surface_y && y < river_y)
					// river
					vm->m_data[i] = n_river_water;
				else if (y <= water_level)
					// sea
					vm->m_data[i] = n_water;
				else
					vm->m_data[i] = n_air;
			}
			vm->m_area.add_y(em, i, 1);
		}
	}

	return surface_max_y;
}


MgStoneType MapgenValleys::generateBiomes(float *heat_map, float *humidity_map)
{
	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;
	MgStoneType stone_type = STONE;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome = NULL;
		u16 depth_top = 0;
		u16 base_filler = 0;
		u16 depth_water_top = 0;
		u32 vi = vm->m_area.index(x, node_max.Y, z);

		// Check node at base of mapchunk above, either a node of a previously
		// generated mapchunk or if not, a node of overgenerated base terrain.
		content_t c_above = vm->m_data[vi + em.X].getContent();
		bool air_above = c_above == CONTENT_AIR;
		bool water_above = (c_above == c_water_source);

		// If there is air or water above enable top/filler placement, otherwise force
		// nplaced to stone level by setting a number exceeding any possible filler depth.
		u16 nplaced = (air_above || water_above) ? 0 : U16_MAX;

		for (s16 y = node_max.Y; y >= node_min.Y; y--) {
			content_t c = vm->m_data[vi].getContent();

			// Biome is recalculated each time an upper surface is detected while
			// working down a column. The selected biome then remains in effect for
			// all nodes below until the next surface and biome recalculation.
			// Biome is recalculated:
			// 1. At the surface of stone below air or water.
			// 2. At the surface of water below air.
			// 3. When stone or water is detected but biome has not yet been calculated.
			if ((c == c_stone && (air_above || water_above || !biome)) ||
					((c == c_water_source || c == c_river_water_source) && (air_above || !biome))) {
				// Both heat and humidity have already been adjusted for altitude.
				biome = bmgr->getBiome(heat_map[index], humidity_map[index], y);

				depth_top = biome->depth_top;
				base_filler = MYMAX(depth_top + biome->depth_filler + noise_filler_depth->result[index], 0);
				depth_water_top = biome->depth_water_top;

				// Detect stone type for dungeons during every biome calculation.
				// This is more efficient than detecting per-node and will not
				// miss any desert stone or sandstone biomes.
				if (biome->c_stone == c_desert_stone)
					stone_type = DESERT_STONE;
				else if (biome->c_stone == c_sandstone)
					stone_type = SANDSTONE;
			}

			if (c == c_stone) {
				content_t c_below = vm->m_data[vi - em.X].getContent();

				// If the node below isn't solid, make this node stone, so that
				// any top/filler nodes above are structurally supported.
				// This is done by aborting the cycle of top/filler placement
				// immediately by forcing nplaced to stone level.
				if (c_below == CONTENT_AIR || c_below == c_water_source || c_below == c_river_water_source)
					nplaced = U16_MAX;

				if (nplaced < depth_top) {
					vm->m_data[vi] = MapNode(biome->c_top);
					nplaced++;
				} else if (nplaced < base_filler) {
					vm->m_data[vi] = MapNode(biome->c_filler);
					nplaced++;
				} else {
					vm->m_data[vi] = MapNode(biome->c_stone);
				}

				air_above = false;
				water_above = false;
			} else if (c == c_water_source) {
				vm->m_data[vi] = MapNode((y > (s32)(water_level - depth_water_top)) ? biome->c_water_top : biome->c_water);
				nplaced = 0;  // Enable top/filler placement for next surface
				air_above = false;
				water_above = true;
			} else if (c == c_river_water_source) {
				vm->m_data[vi] = MapNode(biome->c_river_water);
				nplaced = U16_MAX;
				air_above = false;
				water_above = true;
			} else if (c == CONTENT_AIR) {
				nplaced = 0;  // Enable top/filler placement for next surface
				air_above = true;
				water_above = false;
			} else {  // Possible various nodes overgenerated from neighbouring mapchunks
				nplaced = U16_MAX;  // Disable top/filler placement
				air_above = false;
				water_above = false;
			}

			vm->m_area.add_y(em, vi, -1);
		}
	}

	return stone_type;
}


void MapgenValleys::dustTopNodes()
{
	if (node_max.Y < water_level)
		return;

	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome = (Biome *)bmgr->getRaw(biomemap[index]);

		if (biome->c_dust == CONTENT_IGNORE)
			continue;

		u32 vi = vm->m_area.index(x, full_node_max.Y, z);
		content_t c_full_max = vm->m_data[vi].getContent();
		s16 y_start;

		if (c_full_max == CONTENT_AIR) {
			y_start = full_node_max.Y - 1;
		} else if (c_full_max == CONTENT_IGNORE) {
			vi = vm->m_area.index(x, node_max.Y + 1, z);
			content_t c_max = vm->m_data[vi].getContent();

			if (c_max == CONTENT_AIR)
				y_start = node_max.Y;
			else
				continue;
		} else {
			continue;
		}

		vi = vm->m_area.index(x, y_start, z);
		for (s16 y = y_start; y >= node_min.Y - 1; y--) {
			if (vm->m_data[vi].getContent() != CONTENT_AIR)
				break;

			vm->m_area.add_y(em, vi, -1);
		}

		content_t c = vm->m_data[vi].getContent();
		if (!ndef->get(c).buildable_to && c != CONTENT_IGNORE && c != biome->c_dust) {
			vm->m_area.add_y(em, vi, 1);
			vm->m_data[vi] = MapNode(biome->c_dust);
		}
	}
}


// These look more like real caves to me.
void MapgenValleys::generateSimpleCaves(s16 max_stone_y)
{
	PseudoRandom ps(blockseed + 72202);

	MapNode n_air(CONTENT_AIR);
	MapNode n_dirt(c_dirt);
	MapNode n_water(c_water_source);
	MapNode n_lava(c_lava_source);

	v3s16 em = vm->m_area.getExtent();
	u16 sr = 1000;

	if (max_stone_y >= node_min.Y) {
		u32 index_2d = 0;
		u32 index_3d = 0;
		for (s16 z = node_min.Z; z <= node_max.Z; z++)
			for (s16 x = node_min.X; x <= node_max.X; x++, index_2d++) {
				index_3d = (z - node_min.Z) * csize.X * (csize.Y + 2) + (csize.Y + 1) * csize.X + (x - node_min.X);
				content_t ca = CONTENT_IGNORE;
				bool underground = false;
				u16 air_count = 0;
				u32 index_data = vm->m_area.index(x, node_max.Y + 1, z);

				// Dig caves on down loop to check for stone above.
				for (s16 y = node_max.Y+1; y >= node_min.Y-1; y--, index_3d -= csize.X) {
					content_t c = vm->m_data[index_data].getContent();
					if (ca == c_stone || y < -10)
						underground = true;

					bool n1 = (fabs(noise_simple_caves_1->result[index_3d]) < 0.07);
					bool n2 = (fabs(noise_simple_caves_2->result[index_3d]) < 0.07);

					if (n1 && n2 && c != CONTENT_AIR && c != c_water_source) {
							vm->m_data[index_data] = n_air;
							air_count++;
					} else if (air_count > 0 && c == c_stone) {
						// At the cave floor...
						u32 j = index_data;
						vm->m_area.add_y(em, j, 1);
						sr = ps.range(0,999);
						if (humidmap[index_2d] > 0)
							sr = (u16) (sr * 100.f / humidmap[index_2d]);

						if (!underground) {
							// Dirt falls in if the cave is exposed.
							Biome *biome = (Biome *)bmgr->getRaw(biomemap[index_2d]);
							vm->m_data[index_data] = MapNode(biome->c_filler);
							underground = true;
						} else if (sr < 3) {
							// Waterfalls may get out of control above ground.
							if (y < cave_water_height)
								vm->m_data[j] = n_water;
						} else if (sr < 1000 && 999 - sr < ceil(-y / 10000.f)) {
							if (y < lava_max_height)
								vm->m_data[j] = n_lava;
						}
					}

					if (!(n1 && n2))
						air_count = 0;

					ca = vm->m_data[index_data].getContent();
					vm->m_area.add_y(em, index_data, -1);
				}
			}
	}
}
