/*
Minetest Valleys C
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 paramat, Matt Gregory
Copyright (C) 2016 Duane Robertson <duane@duanerobertson.com>

Based on Valleys Mapgen by Gael de Sailly
 (https://forum.minetest.net/viewtopic.php?f=9&t=11430)
and mapgen_v7 by kwolekr and paramat.

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
#include "treegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_valleys.h"

//#undef NDEBUG
//#include "assert.h"

//#include "util/timetaker.h"
//#include "profiler.h"


//static Profiler mapgen_prof;
//Profiler *mapgen_profiler = &mapgen_prof;

static FlagDesc flagdesc_mapgen_valleys[] = {
	{"altitude_chill",  MG_VALLEYS_ALT_CHILL},
	{"cliffs",          MG_VALLEYS_CLIFFS},
	{"fast",            MG_VALLEYS_FAST},
	{"humid_rivers",    MG_VALLEYS_HUMID_RIVERS},
	{"rugged",          MG_VALLEYS_RUGGED},
	{NULL,              0}
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

	this->biomemap  = new u8[csize.X * csize.Z];
	this->heightmap = new s16[csize.X * csize.Z];
	this->heatmap   = NULL;
	this->humidmap  = NULL;

	MapgenValleysParams *sp = (MapgenValleysParams *)params->sparams;
	this->spflags = sp->spflags;

	this->cliff_terrain      = (spflags & MG_VALLEYS_CLIFFS);
	this->fast_terrain       = (spflags & MG_VALLEYS_FAST);
	this->humid_rivers       = (spflags & MG_VALLEYS_HUMID_RIVERS);
	this->rugged_terrain     = (spflags & MG_VALLEYS_RUGGED);
	this->use_altitude_chill = (spflags & MG_VALLEYS_ALT_CHILL);

	this->altitude_chill        = sp->altitude_chill;
	this->cave_water_max_height = sp->cave_water_max_height;
	this->humidity_adjust       = sp->humidity - 50.f;
	this->humidity_break_point  = sp->humidity_break_point;
	this->lava_max_height       = sp->lava_max_height;
	this->river_depth           = sp->river_depth + 1.f;
	this->river_size            = sp->river_size / 100.f;
	this->temperature_adjust    = sp->temperature - 50.f;
	this->water_features        = MYMAX(1, MYMIN(11, 11 - sp->water_features));

	//// 2D Terrain noise
	noise_cliffs             = new Noise(&sp->np_cliffs,             seed, csize.X, csize.Z);
	noise_corr               = new Noise(&sp->np_corr,               seed, csize.X, csize.Z);
	noise_filler_depth       = new Noise(&sp->np_filler_depth,       seed, csize.X, csize.Z);
	noise_inter_valley_slope = new Noise(&sp->np_inter_valley_slope, seed, csize.X, csize.Z);
	noise_rivers             = new Noise(&sp->np_rivers,             seed, csize.X, csize.Z);
	noise_terrain_height     = new Noise(&sp->np_terrain_height,     seed, csize.X, csize.Z);
	noise_valley_depth       = new Noise(&sp->np_valley_depth,       seed, csize.X, csize.Z);
	noise_valley_profile     = new Noise(&sp->np_valley_profile,     seed, csize.X, csize.Z);

	if (this->fast_terrain)
		noise_inter_valley_fill = new Noise(&sp->np_inter_valley_fill, seed, csize.X, csize.Z);

	//// 3D Terrain noise
	noise_simple_caves_1 = new Noise(&sp->np_simple_caves_1, seed, csize.X, csize.Y + 2, csize.Z);
	noise_simple_caves_2 = new Noise(&sp->np_simple_caves_2, seed, csize.X, csize.Y + 2, csize.Z);

	if (!this->fast_terrain)
		noise_inter_valley_fill = new Noise(&sp->np_inter_valley_fill, seed, csize.X, csize.Y + 2, csize.Z);

	//// Biome noise
	noise_heat_blend     = new Noise(&params->np_biome_heat_blend,     seed, csize.X, csize.Z);
	noise_heat           = new Noise(&params->np_biome_heat,           seed, csize.X, csize.Z);
	noise_humidity_blend = new Noise(&params->np_biome_humidity_blend, seed, csize.X, csize.Z);
	noise_humidity       = new Noise(&params->np_biome_humidity,       seed, csize.X, csize.Z);

	//// Resolve nodes to be used
	INodeDefManager *ndef = emerge->ndef;

	c_cobble               = ndef->getId("mapgen_cobble");
	c_desert_stone         = ndef->getId("mapgen_desert_stone");
	c_dirt                 = ndef->getId("mapgen_dirt");
	c_lava_source          = ndef->getId("mapgen_lava_source");
	c_mossycobble          = ndef->getId("mapgen_mossycobble");
	c_river_water_source   = ndef->getId("mapgen_river_water_source");
	c_sand                 = ndef->getId("mapgen_sand");
	c_sandstonebrick       = ndef->getId("mapgen_sandstonebrick");
	c_sandstone            = ndef->getId("mapgen_sandstone");
	c_stair_cobble         = ndef->getId("mapgen_stair_cobble");
	c_stair_sandstonebrick = ndef->getId("mapgen_stair_sandstonebrick");
	c_stone                = ndef->getId("mapgen_stone");
	c_water_source         = ndef->getId("mapgen_water_source");

	if (c_mossycobble == CONTENT_IGNORE)
		c_mossycobble = c_cobble;
	if (c_river_water_source == CONTENT_IGNORE)
		c_river_water_source = c_water_source;
	if (c_sand == CONTENT_IGNORE)
		c_sand = c_stone;
	if (c_sandstonebrick == CONTENT_IGNORE)
		c_sandstonebrick = c_sandstone;
	if (c_stair_cobble == CONTENT_IGNORE)
		c_stair_cobble = c_cobble;
	if (c_stair_sandstonebrick == CONTENT_IGNORE)
		c_stair_sandstonebrick = c_sandstone;
}


MapgenValleys::~MapgenValleys()
{
	delete noise_cliffs;
	delete noise_corr;
	delete noise_filler_depth;
	delete noise_heat;
	delete noise_heat_blend;
	delete noise_humidity;
	delete noise_humidity_blend;
	delete noise_inter_valley_fill;
	delete noise_inter_valley_slope;
	delete noise_rivers;
	delete noise_simple_caves_1;
	delete noise_simple_caves_2;
	delete noise_terrain_height;
	delete noise_valley_depth;
	delete noise_valley_profile;

	delete[] heightmap;
	delete[] biomemap;
}


MapgenValleysParams::MapgenValleysParams()
{
	spflags = MG_VALLEYS_CLIFFS | MG_VALLEYS_RUGGED
	         	| MG_VALLEYS_HUMID_RIVERS | MG_VALLEYS_ALT_CHILL;

	altitude_chill         =  90; // The altitude at which temperature drops by 20C.
	// Water in caves will never be higher than this.
	cave_water_max_height  =  MAX_MAP_GENERATION_LIMIT;
	humidity               =  50;
	// the maximum humidity around rivers in otherwise dry areas
	humidity_break_point   =  65;
	lava_max_height        =  0;  // Lava will never be higher than this.
	river_depth            =  4;  // How deep to carve river channels.
	river_size             =  5;  // How wide to make rivers.
	temperature            =  50;
	water_features         =  3;  // How often water will occur in caves.

	np_cliffs             = NoiseParams(0.f,   1.f,  v3f(750,  750,  750),  8445,  5, 1.f,  2.f);
	np_corr               = NoiseParams(0.f,   1.f,  v3f(40,   40,   40),   -3536, 4, 1.f,  2.f);
	np_filler_depth       = NoiseParams(0.f,   1.2f, v3f(256,  256,  256),  1605,  3, 0.5f, 2.f);
	np_inter_valley_fill  = NoiseParams(0.f,   1.f,  v3f(256,  512,  256),  1993,  6, 0.8f, 2.f);
	np_inter_valley_slope = NoiseParams(0.5f,  0.5f, v3f(128,  128,  128),  746,   1, 1.f,  2.f);
	np_rivers             = NoiseParams(0.f,   1.f,  v3f(256,  256,  256),  -6050, 5, 0.6f, 2.f);
	np_simple_caves_1     = NoiseParams(0.f,   1.f,  v3f(64,   64,   64),   -8402, 3, 0.5f, 2.f);
	np_simple_caves_2     = NoiseParams(0.f,   1.f,  v3f(64,   64,   64),   3944,  3, 0.5f, 2.f);
	np_terrain_height     = NoiseParams(-10.f, 50.f, v3f(1024, 1024, 1024), 5202,  6, 0.4f, 2.f);
	np_valley_depth       = NoiseParams(5.f,   4.f,  v3f(512,  512,  512),  -1914, 1, 1.f,  2.f);
	np_valley_profile     = NoiseParams(0.6f,  0.5f, v3f(512,  512,  512),  777,   1, 1.f,  2.f);
	}


void MapgenValleysParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mg_valleys_spflags", spflags, flagdesc_mapgen_valleys);

	settings->getU16NoEx("mg_valleys_altitude_chill",        altitude_chill);
	settings->getS16NoEx("mg_valleys_cave_water_max_height", cave_water_max_height);
	settings->getS16NoEx("mg_valleys_humidity",              humidity);
	settings->getS16NoEx("mg_valleys_humidity_break_point",  humidity_break_point);
	settings->getS16NoEx("mg_valleys_lava_max_height",       lava_max_height);
	settings->getU16NoEx("mg_valleys_river_depth",           river_depth);
	settings->getU16NoEx("mg_valleys_river_size",            river_size);
	settings->getS16NoEx("mg_valleys_temperature",           temperature);
	settings->getU16NoEx("mg_valleys_water_features",        water_features);

	settings->getNoiseParams("mg_valleys_np_cliffs",             np_cliffs);
	settings->getNoiseParams("mg_valleys_np_corr",               np_corr);
	settings->getNoiseParams("mg_valleys_np_filler_depth",       np_filler_depth);
	settings->getNoiseParams("mg_valleys_np_inter_valley_fill",  np_inter_valley_fill);
	settings->getNoiseParams("mg_valleys_np_inter_valley_slope", np_inter_valley_slope);
	settings->getNoiseParams("mg_valleys_np_rivers",             np_rivers);
	settings->getNoiseParams("mg_valleys_np_simple_caves_1",     np_simple_caves_1);
	settings->getNoiseParams("mg_valleys_np_simple_caves_2",     np_simple_caves_2);
	settings->getNoiseParams("mg_valleys_np_terrain_height",     np_terrain_height);
	settings->getNoiseParams("mg_valleys_np_valley_depth",       np_valley_depth);
	settings->getNoiseParams("mg_valleys_np_valley_profile",     np_valley_profile);
}


void MapgenValleysParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mg_valleys_spflags", spflags, flagdesc_mapgen_valleys, U32_MAX);

	settings->setU16("mg_valleys_altitude_chill",        altitude_chill);
	settings->setS16("mg_valleys_cave_water_max_height", cave_water_max_height);
	settings->setS16("mg_valleys_humidity",              humidity);
	settings->setS16("mg_valleys_humidity_break_point",  humidity_break_point);
	settings->setS16("mg_valleys_lava_max_height",       lava_max_height);
	settings->setU16("mg_valleys_river_depth",           river_depth);
	settings->setU16("mg_valleys_river_size",            river_size);
	settings->setS16("mg_valleys_temperature",           temperature);
	settings->setU16("mg_valleys_water_features",        water_features);

	settings->setNoiseParams("mg_valleys_np_cliffs",             np_cliffs);
	settings->setNoiseParams("mg_valleys_np_corr",               np_corr);
	settings->setNoiseParams("mg_valleys_np_filler_depth",       np_filler_depth);
	settings->setNoiseParams("mg_valleys_np_inter_valley_fill",  np_inter_valley_fill);
	settings->setNoiseParams("mg_valleys_np_inter_valley_slope", np_inter_valley_slope);
	settings->setNoiseParams("mg_valleys_np_rivers",             np_rivers);
	settings->setNoiseParams("mg_valleys_np_simple_caves_1",     np_simple_caves_1);
	settings->setNoiseParams("mg_valleys_np_simple_caves_2",     np_simple_caves_2);
	settings->setNoiseParams("mg_valleys_np_terrain_height",     np_terrain_height);
	settings->setNoiseParams("mg_valleys_np_valley_depth",       np_valley_depth);
	settings->setNoiseParams("mg_valleys_np_valley_profile",     np_valley_profile);
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

	//TimeTaker t("makeChunk");

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
	bmgr->calcBiomes(csize.X, csize.Z, heatmap, humidmap, heightmap, biomemap);

	// Actually place the biome-specific nodes
	MgStoneType stone_type = generateBiomes(heatmap, humidmap);

	// Cave creation.
	if (flags & MG_CAVES)
		generateSimpleCaves(stone_surface_max_y);

	// Dungeon creation
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
			dp.mossratio     = 3.f;
			dp.holesize      = v3s16(1, 2, 1);
			dp.roomsize      = v3s16(0, 0, 0);
			dp.notifytype    = GENNOTIFY_DUNGEON;
		} else if (stone_type == DESERT_STONE) {
			dp.c_cobble = c_desert_stone;
			dp.c_moss   = c_desert_stone;
			dp.c_stair  = c_desert_stone;

			dp.diagonal_dirs = true;
			dp.mossratio     = 0.f;
			dp.holesize      = v3s16(2, 3, 2);
			dp.roomsize      = v3s16(2, 5, 2);
			dp.notifytype    = GENNOTIFY_TEMPLE;
		} else if (stone_type == SANDSTONE) {
			dp.c_cobble = c_sandstonebrick;
			dp.c_moss   = c_sandstonebrick;
			dp.c_stair  = c_sandstonebrick;

			dp.diagonal_dirs = false;
			dp.mossratio     = 0.f;
			dp.holesize      = v3s16(2, 2, 2);
			dp.roomsize      = v3s16(2, 0, 2);
			dp.notifytype    = GENNOTIFY_DUNGEON;
		}

		DungeonGen dgen(this, &dp);
		dgen.generate(blockseed, full_node_min, full_node_max);
	}

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

	noise_filler_depth->perlinMap2D(x, z);
	noise_heat_blend->perlinMap2D(x, z);
	noise_heat->perlinMap2D(x, z);
	noise_humidity_blend->perlinMap2D(x, z);
	noise_humidity->perlinMap2D(x, z);
	noise_inter_valley_slope->perlinMap2D(x, z);
	noise_rivers->perlinMap2D(x, z);
	noise_terrain_height->perlinMap2D(x, z);
	noise_valley_depth->perlinMap2D(x, z);
	noise_valley_profile->perlinMap2D(x, z);

	if (fast_terrain) {
		// Make this 2D for speed, if requested.
		noise_inter_valley_fill->perlinMap2D(x, z);

		if (cliff_terrain)
			noise_cliffs->perlinMap2D(x, z);
		if (rugged_terrain)
			noise_corr->perlinMap2D(x, z);
	} else {
		noise_inter_valley_fill->perlinMap3D(x, y, z);
	}

	if (flags & MG_CAVES) {
		noise_simple_caves_1->perlinMap3D(x, y, z);
		noise_simple_caves_2->perlinMap3D(x, y, z);
	}

	//mapgen_profiler->avg("noisemaps", tcn.stop() / 1000.f);

	for (s32 index = 0; index < csize.X * csize.Z; index++) {
		noise_heat->result[index] += noise_heat_blend->result[index];
		noise_heat->result[index] += temperature_adjust;
		noise_humidity->result[index] += noise_humidity_blend->result[index];
	}

	TerrainNoise tn;

	u32 index = 0;
	for (tn.z = node_min.Z; tn.z <= node_max.Z; tn.z++)
	for (tn.x = node_min.X; tn.x <= node_max.X; tn.x++, index++) {
		// The parameters that we actually need to generate terrain
		//  are passed by address (and the return value).
		tn.terrain_height = noise_terrain_height->result[index];
		// River noise is replaced with base terrain, which
		// is basically the height of the water table.
		tn.rivers = &noise_rivers->result[index];
		// Valley depth noise is replaced with the valley
		// number that represents the height of terrain
		// over rivers and is used to determine about
		// how close a river is for humidity calculation.
		tn.valley = &noise_valley_depth->result[index];
		tn.valley_profile = noise_valley_profile->result[index];
		// Slope noise is replaced by the calculated slope
		// which is used to get terrain height in the slow
		// method, to create sharper mountains.
		tn.slope = &noise_inter_valley_slope->result[index];
		tn.inter_valley_fill = noise_inter_valley_fill->result[index];
		tn.cliffs = noise_cliffs->result[index];
		tn.corr = noise_corr->result[index];

		// This is the actual terrain height.
		float mount = terrainLevelFromNoise(&tn);
		noise_terrain_height->result[index] = mount;

		if (fast_terrain) {
			// Assign humidity adjusted by water proximity.
			// I can't think of a reason why a mod would expect base humidity
			//  from noise or at any altitude other than ground level.
			noise_humidity->result[index] = humidityByTerrain(
					noise_humidity->result[index],
					mount,
					noise_rivers->result[index],
					noise_valley_depth->result[index]);

			// Assign heat adjusted by altitude. See humidity, above.
			if (use_altitude_chill && mount > 0.f)
				noise_heat->result[index] *= pow(0.5f, (mount - altitude_chill / 3.f) / altitude_chill);
		}
	}

	heatmap = noise_heat->result;
	humidmap = noise_humidity->result;
}


// This keeps us from having to maintain two similar sets of
//  complicated code to determine ground level.
float MapgenValleys::terrainLevelFromNoise(TerrainNoise *tn)
{
	float inter_valley_slope = *tn->slope;

	// The square function changes the behaviour of this noise:
	//  very often small, and sometimes very high.
	float valley_d = pow(*tn->valley, 2);

	// valley_d is here because terrain is generally higher where valleys
	//  are deep (mountains). base represents the height of the
	//  rivers, most of the surface is above.
	float base = tn->terrain_height + valley_d;

	// "river" represents the distance from the river, in arbitrary units.
	float river = fabs(*tn->rivers) - river_size;

	// Use the curve of the function 1−exp(−(x/a)²) to model valleys.
	//  Making "a" vary (0 < a ≤ 1) changes the shape of the valleys.
	//  Try it with a geometry software !
	//   (here x = "river" and a = valley_profile).
	//  "valley" represents the height of the terrain, from the rivers.
	*tn->valley = valley_d * (1.f - exp(- pow(river / tn->valley_profile, 2)));

	// approximate height of the terrain at this point
	float mount = base + *tn->valley;

	*tn->slope *= *tn->valley;

	// Rivers are placed where "river" is negative, so where the original
	//  noise value is close to zero.
	// Base ground is returned as rivers since it's basically the water table.
	*tn->rivers = base;
	if (river < 0.f) {
		// Use the the function −sqrt(1−x²) which models a circle.
		float depth = (river_depth * sqrt(1.f - pow((river / river_size + 1), 2)));

		// base - depth : height of the bottom of the river
		// water_level - 2 : don't make rivers below 2 nodes under the surface
		u16 min_bottom = 2;
		if (!fast_terrain)
			min_bottom = 6;
		mount = fmin(fmax(base - depth, (float) (water_level - min_bottom)), mount);

		// Slope has no influence on rivers.
		*tn->slope = 0.f;
	}

	if (fast_terrain) {
		// The penultimate step builds up the heights, but we reduce it 
		//  occasionally to create cliffs.
		float delta = sin(tn->inter_valley_fill) * *tn->slope;
		if (cliff_terrain && tn->cliffs >= 0.2f)
			mount += delta * 0.66f;
		else
			mount += delta;

		// Use yet another noise to make the heights look more rugged.
		if (rugged_terrain 
				&& mount > water_level 
				&& fabs(inter_valley_slope * tn->inter_valley_fill) < 0.3f)
			mount += ((delta < 0.f) ? -1.f : 1.f) * pow(fabs(delta), 0.5f) * fabs(sin(tn->corr));
	}

	return mount;
}


// This avoids duplicating the code in terrainLevelFromNoise, adding
// only the final step of terrain generation without a noise map.
float MapgenValleys::adjustedTerrainLevelFromNoise(TerrainNoise *tn)
{
	float mount = terrainLevelFromNoise(tn);

	if (!fast_terrain) {
		for (s16 y = round(mount); y <= round(mount) + 1000; y++) {
			float fill = NoisePerlin3D(&noise_inter_valley_fill->np, tn->x, y, tn->z, seed);

			if (fill * *tn->slope <= y - mount) {
				mount = fmax(y - 1, mount);
				break;
			}
		}
	}

	return mount;
}


float MapgenValleys::humidityByTerrain(
		float humidity_base, 
		float mount, 
		float rivers, 
		float valley)
{
	// Although the original valleys adjusts humidity by distance
	// from seawater, this causes problems with the default biomes.
	// Adjust only by freshwater proximity.
	float humidity = humidity_base + humidity_adjust;

	if (humid_rivers && mount > water_level) {
		// Offset to make everything average the same.
		humidity -= (humidity_break_point - humidity_adjust) / 3.f;

		// This method is from the original lua.
		float water_table = pow(0.5f, fmax(rivers / 3.f, 0.f));
		// This adds humidity next to rivers and lakes.
		float river_water = pow(0.5f, fmax(valley / 12.f, 0.f));
		// Combine the two.
		float water = water_table + (1.f - water_table) * river_water;
		humidity = fmax(humidity, (humidity_break_point * water));
	}

	return humidity;
}


inline int MapgenValleys::getGroundLevelAtPoint(v2s16 p)
{
	// Base terrain calculation
	return terrainLevelAtPoint(p.X, p.Y);
}


float MapgenValleys::terrainLevelAtPoint(s16 x, s16 z)
{
	TerrainNoise tn;

	float rivers = NoisePerlin2D(&noise_rivers->np, x, z, seed);
	float valley = NoisePerlin2D(&noise_valley_depth->np, x, z, seed);
	float inter_valley_slope = NoisePerlin2D(&noise_inter_valley_slope->np, x, z, seed);

	tn.x = x;
	tn.z = z;
	tn.terrain_height = NoisePerlin2D(&noise_terrain_height->np, x, z, seed);
	tn.rivers = &rivers;
	tn.valley = &valley;
	tn.valley_profile = NoisePerlin2D(&noise_valley_profile->np, x, z, seed);
	tn.slope = &inter_valley_slope;
	tn.inter_valley_fill = 0.f;
	tn.cliffs = 0.f;
	tn.corr = 0.f;

	if (fast_terrain) {
		tn.inter_valley_fill = NoisePerlin2D(&noise_inter_valley_fill->np, x, z, seed);

		if (cliff_terrain)
			tn.cliffs = NoisePerlin2D(&noise_cliffs->np, x, z, seed);
		if (rugged_terrain)
			tn.corr = NoisePerlin2D(&noise_corr->np, x, z, seed);
	}

	return adjustedTerrainLevelFromNoise(&tn);
}


int MapgenValleys::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_river_water(c_river_water_source);
	MapNode n_sand(c_sand);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	v3s16 em = vm->m_area.getExtent();
	s16 surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 index_2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index_2d++) {
		s16 river_y = round(noise_rivers->result[index_2d]);
		s16 surface_y = round(noise_terrain_height->result[index_2d]);
		float slope = noise_inter_valley_slope->result[index_2d];

		heightmap[index_2d] = surface_y;

		if (surface_y > surface_max_y)
			surface_max_y = surface_y;

		u32 index_3d = 0;
		if (!fast_terrain)
			index_3d = (z - node_min.Z) * zstride + (x - node_min.X);

		u32 index_data = vm->m_area.index(x, node_min.Y - 1, z);

		// Mapgens concern themselves with stone and water.
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			float fill = 0.f;
			if (!fast_terrain)
				fill = noise_inter_valley_fill->result[index_3d];

			if (vm->m_data[index_data].getContent() == CONTENT_IGNORE) {
				bool river = (river_y > surface_y);

				if (river && y == surface_y) {
					// river bottom
					vm->m_data[index_data] = n_sand;
				} else if ((fast_terrain || river) && y <= surface_y) {
					// ground
					vm->m_data[index_data] = n_stone;
				} else if (river && y < river_y) {
					// river
					vm->m_data[index_data] = n_river_water;
				} else if ((!fast_terrain) && (!river) && fill * slope > y - surface_y) {
					// ground (slow method)
					vm->m_data[index_data] = n_stone;
					heightmap[index_2d] = surface_max_y = y;
				} else if (y <= water_level) {
					// sea
					vm->m_data[index_data] = n_water;
				} else {
					vm->m_data[index_data] = n_air;
				}
			}

			vm->m_area.add_y(em, index_data, 1);
			if (!fast_terrain)
				index_3d += ystride;
		}

		if (!fast_terrain) {
			// Assign the humidity adjusted by water proximity.
			noise_humidity->result[index_2d] = humidityByTerrain(
					noise_humidity->result[index_2d], 
					surface_max_y, 
					noise_rivers->result[index_2d], 
					noise_valley_depth->result[index_2d]);

			// Assign the heat adjusted by altitude. See humidity, above.
			if (use_altitude_chill && surface_max_y > 0)
				noise_heat->result[index_2d] 
					*= pow(0.5f, (surface_max_y - altitude_chill / 3.f) / altitude_chill);
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
			if ((c == c_stone && (air_above || water_above || !biome)) 
					|| ((c == c_water_source || c == c_river_water_source) 
							&& (air_above || !biome))) {
				// Both heat and humidity have already been adjusted for altitude.
				biome = bmgr->getBiome(heat_map[index], humidity_map[index], y);

				depth_top = biome->depth_top;
				base_filler = MYMAX(depth_top 
						+ biome->depth_filler 
						+ noise_filler_depth->result[index], 0.f);
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
				if (c_below == CONTENT_AIR 
						|| c_below == c_water_source 
						|| c_below == c_river_water_source)
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
				vm->m_data[vi] = MapNode((y > (s32)(water_level - depth_water_top)) 
						? biome->c_water_top : biome->c_water);
				nplaced = 0;  // Enable top/filler placement for next surface
				air_above = false;
				water_above = true;
			} else if (c == c_river_water_source) {
				vm->m_data[vi] = MapNode(biome->c_river_water);
				nplaced = U16_MAX;  // Sand was already placed under rivers.
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


void MapgenValleys::generateSimpleCaves(s16 max_stone_y)
{
	PseudoRandom ps(blockseed + 72202);

	MapNode n_air(CONTENT_AIR);
	MapNode n_dirt(c_dirt);
	MapNode n_lava(c_lava_source);
	MapNode n_water(c_river_water_source);

	v3s16 em = vm->m_area.getExtent();

	s16 base_water_chance = 0;
	if (water_features < 11)
		base_water_chance = ceil(MAX_MAP_GENERATION_LIMIT / (water_features * 1000));

	if (max_stone_y >= node_min.Y) {
		u32 index_2d = 0;
		u32 index_3d = 0;
		for (s16 z = node_min.Z; z <= node_max.Z; z++)
		for (s16 x = node_min.X; x <= node_max.X; x++, index_2d++) {
			bool air_above = false;
			//bool underground = false;
			u32 index_data = vm->m_area.index(x, node_max.Y + 1, z);

			index_3d = (z - node_min.Z) * zstride + (csize.Y + 1) * ystride + (x - node_min.X);

			// Dig caves on down loop to check for air above.
			for (s16 y = node_max.Y + 1; 
					y >= node_min.Y - 1; 
					y--, index_3d -= ystride, vm->m_area.add_y(em, index_data, -1)) {
				float terrain = noise_terrain_height->result[index_2d];

				// Saves some time and prevents removing above ground nodes.
				if (y > terrain + 1) {
					air_above = true;
					continue;
				}

				content_t c = vm->m_data[index_data].getContent();
				bool n1 = (fabs(noise_simple_caves_1->result[index_3d]) < 0.07f);
				bool n2 = (fabs(noise_simple_caves_2->result[index_3d]) < 0.07f);

				// River water is (foolishly) not set as ground content
				// in the default game. This can produce strange results
				// when a cave undercuts a river. However, that's not for
				// the mapgen to correct. Fix it in lua.

				if (c == CONTENT_AIR) {
					air_above = true;
				} else if (n1 && n2 && ndef->get(c).is_ground_content) {
					// When both n's are true, we're in a cave.
					vm->m_data[index_data] = n_air;
					air_above = true;
				} else if (air_above 
						&& (c == c_stone || c == c_sandstone || c == c_desert_stone)) {
					// At the cave floor
					s16 sr = ps.next() & 1023;
					u32 j = index_data;
					vm->m_area.add_y(em, j, 1);

					if (sr > (terrain - y) * 25) {
						// Put dirt in caves near the surface.
						Biome *biome = (Biome *)bmgr->getRaw(biomemap[index_2d]);
						vm->m_data[index_data] = MapNode(biome->c_filler);
					} else {
						s16 lava_chance = 0;

						if (y <= lava_max_height && c == c_stone) {
							// Lava spawns increase with depth.
							lava_chance = ceil((lava_max_height - y + 1) / 10000);

							if (sr < lava_chance)
								vm->m_data[j] = n_lava;
						}

						if (base_water_chance > 0 && y <= cave_water_max_height) {
							s16 water_chance = base_water_chance 
								- (abs(y - water_level) / (water_features * 1000));

							// Waterfalls may get out of control above ground.
							sr -= lava_chance;
							// If sr < 0 then we should have already placed lava --
							// don't immediately dump water on it.
							if (sr >= 0 && sr < water_chance)
								vm->m_data[j] = n_water;
						}
					}

					air_above = false;
				}

				// If we're not in a cave, there's no open space.
				if (!(n1 && n2))
					air_above = false;
			}
		}
	}
}
