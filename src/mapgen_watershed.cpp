/*
Minetest
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 paramat, Matt Gregory

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
//#include "profiler.h"  // For TimeTaker
#include "settings.h"  // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "treegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_watershed.h"


FlagDesc flagdesc_mapgen_watershed[] = {
	{NULL,        0}
};

///////////////////////////////////////////////////////////////////////////////


MapgenWatershed::MapgenWatershed(int mapgenid, MapgenParams *params, EmergeManager *emerge)
	: Mapgen(mapgenid, params, emerge)
{
	this->m_emerge = emerge;
	this->bmgr     = emerge->biomemgr;

	//// amount of elements to skip for the next index
	//// for noise/height/biome maps (not vmanip)
	this->ystride = csize.X;
	this->zstride = csize.X * (csize.Y + 2);

	this->biomemap  = new u8[csize.X * csize.Z];
	this->heightmap = new s16[csize.X * csize.Z];
	this->heatmap   = NULL;
	this->humidmap  = NULL;

	MapgenWatershedParams *sp = (MapgenWatershedParams *)params->sparams;
	this->spflags = sp->spflags;

	//// 2D Terrain noise
	noise_ridge        = new Noise(&sp->np_ridge,        seed, csize.X, csize.Z);
	noise_valley_base  = new Noise(&sp->np_valley_base,  seed, csize.X, csize.Z);
	noise_valley       = new Noise(&sp->np_valley,       seed, csize.X, csize.Z);
	noise_valley_amp   = new Noise(&sp->np_valley_amp,   seed, csize.X, csize.Z);
	noise_plateau      = new Noise(&sp->np_plateau,      seed, csize.X, csize.Z);
	noise_mountain_amp = new Noise(&sp->np_mountain_amp, seed, csize.X, csize.Z);
	noise_lake         = new Noise(&sp->np_lake,         seed, csize.X, csize.Z);

	//// 3D terrain noise
	noise_mountain = new Noise(&sp->np_mountain, seed, csize.X, csize.Y + 2, csize.Z);
	noise_cave1    = new Noise(&sp->np_cave1,    seed, csize.X, csize.Y + 2, csize.Z);
	noise_cave2    = new Noise(&sp->np_cave2,    seed, csize.X, csize.Y + 2, csize.Z);
	noise_cave3    = new Noise(&sp->np_cave3,    seed, csize.X, csize.Y + 2, csize.Z);

	//// Biome noise
	noise_heat           = new Noise(&params->np_biome_heat,           seed, csize.X, csize.Z);
	noise_humidity       = new Noise(&params->np_biome_humidity,       seed, csize.X, csize.Z);
	noise_heat_blend     = new Noise(&params->np_biome_heat_blend,     seed, csize.X, csize.Z);
	noise_humidity_blend = new Noise(&params->np_biome_humidity_blend, seed, csize.X, csize.Z);

	//// Resolve nodes to be used
	INodeDefManager *ndef = emerge->ndef;

	c_stone                = ndef->getId("mapgen_stone");
	c_sand                 = ndef->getId("mapgen_sand");
	c_water_source         = ndef->getId("mapgen_water_source");
	c_river_water_source   = ndef->getId("mapgen_river_water_source");
	c_lava_source          = ndef->getId("mapgen_lava_source");
	c_desert_stone         = ndef->getId("mapgen_desert_stone");
	c_ice                  = ndef->getId("mapgen_ice");
	c_sandstone            = ndef->getId("mapgen_sandstone");

	c_cobble               = ndef->getId("mapgen_cobble");
	c_stair_cobble         = ndef->getId("mapgen_stair_cobble");
	c_mossycobble          = ndef->getId("mapgen_mossycobble");
	c_sandstonebrick       = ndef->getId("mapgen_sandstonebrick");
	c_stair_sandstonebrick = ndef->getId("mapgen_stair_sandstonebrick");

	if (c_ice == CONTENT_IGNORE)
		c_ice = CONTENT_AIR;
	if (c_mossycobble == CONTENT_IGNORE)
		c_mossycobble = c_cobble;
	if (c_stair_cobble == CONTENT_IGNORE)
		c_stair_cobble = c_cobble;
	if (c_sandstonebrick == CONTENT_IGNORE)
		c_sandstonebrick = c_sandstone;
	if (c_stair_sandstonebrick == CONTENT_IGNORE)
		c_stair_sandstonebrick = c_sandstone;
}


MapgenWatershed::~MapgenWatershed()
{
	delete noise_ridge;
	delete noise_valley_base;
	delete noise_valley;
	delete noise_valley_amp;
	delete noise_plateau;
	delete noise_mountain_amp;
	delete noise_lake;

	delete noise_mountain;
	delete noise_cave1;
	delete noise_cave2;
	delete noise_cave3;

	delete noise_heat;
	delete noise_humidity;
	delete noise_heat_blend;
	delete noise_humidity_blend;

	delete[] heightmap;
	delete[] biomemap;
}


MapgenWatershedParams::MapgenWatershedParams()
{
	spflags = 0;

	np_ridge        = NoiseParams(0.5,  0.0,  v3f(6144, 6144, 6144), 27,      3, 0.6,  2.0);
	np_valley_base  = NoiseParams(0.0,  1.0,  v3f(3072, 3072, 3072), 106,     3, 0.4,  2.0);
	np_valley       = NoiseParams(0.0,  1.0,  v3f(1536, 1536, 1536), 2177,    5, 0.5,  2.0);
	np_valley_amp   = NoiseParams(1.0,  1.0,  v3f(1536, 1536, 1536), 11206,   1, 0.5,  2.0);
	np_plateau      = NoiseParams(1.0,  0.6,  v3f(1536, 1536, 1536), 63,      5, 0.4,  2.0);
	np_mountain_amp = NoiseParams(0.0,  2.0,  v3f(1536, 1536, 1536), 2170070, 1, 0.5,  2.0);
	np_lake         = NoiseParams(0.0,  1.0,  v3f(384,  384,  384),  7553,    3, 0.4,  2.0);

	np_mountain     = NoiseParams(0.0,  1.0,  v3f(384,  384,  384),  700334,  5, 0.67, 2.0);
	np_cave1        = NoiseParams(0.0,  1.0,  v3f(191,  191,  191),  52534,   4, 0.5,  2.0);
	np_cave2        = NoiseParams(0.0,  1.0,  v3f(193,  193,  193),  325,     4, 0.5,  2.0);
	np_cave3        = NoiseParams(0.0,  1.0,  v3f(197,  197,  197),  109848,  4, 0.5,  2.0);
}


void MapgenWatershedParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed);

	settings->getNoiseParams("mgwatershed_np_ridge",        np_ridge);
	settings->getNoiseParams("mgwatershed_np_valley_base",  np_valley_base);
	settings->getNoiseParams("mgwatershed_np_valley",       np_valley);
	settings->getNoiseParams("mgwatershed_np_valley_amp",   np_valley_amp);
	settings->getNoiseParams("mgwatershed_np_plateau",      np_plateau);
	settings->getNoiseParams("mgwatershed_np_mountain_amp", np_mountain_amp);
	settings->getNoiseParams("mgwatershed_np_lake",         np_lake);

	settings->getNoiseParams("mgwatershed_np_mountain",     np_mountain);
	settings->getNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->getNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->getNoiseParams("mgwatershed_np_cave3",        np_cave3);
}


void MapgenWatershedParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgwatershed_spflags", spflags, flagdesc_mapgen_watershed, U32_MAX);

	settings->setNoiseParams("mgwatershed_np_ridge",        np_ridge);
	settings->setNoiseParams("mgwatershed_np_valley_base",  np_valley_base);
	settings->setNoiseParams("mgwatershed_np_valley",       np_valley);
	settings->setNoiseParams("mgwatershed_np_valley_amp",   np_valley_amp);
	settings->setNoiseParams("mgwatershed_np_plateau",      np_plateau);
	settings->setNoiseParams("mgwatershed_np_mountain_amp", np_mountain_amp);
	settings->setNoiseParams("mgwatershed_np_lake",         np_lake);

	settings->setNoiseParams("mgwatershed_np_mountain",     np_mountain);
	settings->setNoiseParams("mgwatershed_np_cave1",        np_cave1);
	settings->setNoiseParams("mgwatershed_np_cave2",        np_cave2);
	settings->setNoiseParams("mgwatershed_np_cave3",        np_cave3);
}


///////////////////////////////////////


int MapgenWatershed::getGroundLevelAtPoint(v2s16 p)
{
	float n_ridge = NoisePerlin2D(&noise_ridge->np, p.X, p.Y, seed);
	float n_valley_base =
		-pow(fabs(NoisePerlin2D(&noise_valley_base->np, p.X, p.Y, seed)), 0.8);
	float n_valley = fabs(NoisePerlin2D(&noise_valley->np, p.X, p.Y, seed));
	float n_valley_amp = NoisePerlin2D(&noise_valley_amp->np, p.X, p.Y, seed);
	float n_plateau = NoisePerlin2D(&noise_plateau->np, p.X, p.Y, seed);
	float n_mountain_amp =
		MYMAX(NoisePerlin2D(&noise_mountain_amp->np, p.X, p.Y, seed), 0.0);
	float n_lake = NoisePerlin2D(&noise_lake->np, p.X, p.Y, seed);

	n_lake = n_lake * n_lake * n_lake * n_lake;
	float lake_area = MYMIN(n_lake, 2.0);
	n_valley = pow(n_valley, 1.0 + lake_area);

	n_valley += n_valley_base * 0.05;
	float mountain_amp;
	if (n_valley > 0.0) {
		n_valley = MYMIN(pow(n_valley, 1.5) * n_valley_amp, n_plateau);
		mountain_amp = n_mountain_amp * n_valley * MYMAX(1.0 + n_valley_base, 0.0);
	} else {  // In river, prevent spawn
		return MAX_MAP_GENERATION_LIMIT;
	}

	bool solid_found = false;
	u8 spawn_space = 0;

	for (s16 y = water_level; y <= water_level + 128; y++) {
		float n_mountain = (1.0 + NoisePerlin3D(&noise_mountain->np, p.X, y, p.Y, seed))
			* mountain_amp;
		float density_gradient = -y / 128.0;
		float density_valley_base = n_ridge + n_valley_base + density_gradient;
		float density = density_valley_base + n_valley + n_mountain;

		if (density > 0.0) {
			solid_found = true;
		} else if (solid_found) {  // density is <= 0.0
			spawn_space += 1;
			if (spawn_space == 4)
				return y - 4;
		}
	}

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

	// Generate base terrain, mountains, and ridges with initial heightmaps
	s16 stone_surface_max_y = generateTerrain();

	// Create heightmap
	updateHeightmap(node_min, node_max);

	// Create biomemap at heightmap surface
	bmgr->calcBiomes(csize.X, csize.Z, noise_heat->result,
		noise_humidity->result, heightmap, biomemap);

	// Actually place the biome-specific nodes
	MgStoneType stone_type = generateBiomes(noise_heat->result, noise_humidity->result);

	if (flags & MG_CAVES)
		generateCaves(stone_surface_max_y);

	if ((flags & MG_DUNGEONS) && (stone_surface_max_y >= node_min.Y)) {
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

	//printf("makeChunk: %dms\n", t.stop());

	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	if (flags & MG_LIGHT)
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);

	this->generating = false;
}


void MapgenWatershed::calculateNoise()
{
	//TimeTaker t("calculateNoise", NULL, PRECISION_MICRO);
	int x = node_min.X;
	int y = node_min.Y - 1;
	int z = node_min.Z;

	noise_ridge->perlinMap2D(x, z);
	noise_valley_base->perlinMap2D(x, z);
	noise_valley->perlinMap2D(x, z);
	noise_valley_amp->perlinMap2D(x, z);
	noise_plateau->perlinMap2D(x, z);
	noise_mountain_amp->perlinMap2D(x, z);
	noise_lake->perlinMap2D(x, z);

	noise_mountain->perlinMap3D(x, y, z);

	if (flags & MG_CAVES) {
		noise_cave1->perlinMap3D(x, y, z);
		noise_cave2->perlinMap3D(x, y, z);
		noise_cave3->perlinMap3D(x, y, z);
	}

	noise_heat->perlinMap2D(x, z);
	noise_humidity->perlinMap2D(x, z);
	noise_heat_blend->perlinMap2D(x, z);
	noise_humidity_blend->perlinMap2D(x, z);

	for (s32 i = 0; i < csize.X * csize.Z; i++) {
		noise_heat->result[i] += noise_heat_blend->result[i];
		noise_humidity->result[i] += noise_humidity_blend->result[i];
	}

	heatmap = noise_heat->result;
	humidmap = noise_humidity->result;
	//printf("calculateNoise: %dus\n", t.stop());
}


int MapgenWatershed::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);
	MapNode n_river_water(c_river_water_source);
	MapNode n_sand(c_sand);

	int stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	v3s16 em = vm->m_area.getExtent();
	u32 index2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index2d++) {
		// alter height of ridge summit
		float n_ridge = noise_ridge->result[index2d];
		// large scale ridges structure, negative because inverted
		float n_valley_base = -pow(fabs(noise_valley_base->result[index2d]), 0.8);
		// river course and valley shape
		float n_valley = fabs(noise_valley->result[index2d]);
		// vary amplitude of valley sides
		float n_valley_amp = noise_valley_amp->result[index2d];
		// chop valley sides at a varying plateau height
		float n_plateau = noise_plateau->result[index2d];
		// vary mountain amplitude
		float n_mountain_amp = MYMAX(noise_mountain_amp->result[index2d], 0.0);
		// define lake areas
		float n_lake = noise_lake->result[index2d];

		n_lake = n_lake * n_lake * n_lake * n_lake;  // make positive
		float lake_area = MYMIN(n_lake, 2.0);
		n_valley = pow(n_valley, 1.0 + lake_area);  // alter valley shape

		// valley cross section sinks a little below 0 to define river area (width)
		n_valley += n_valley_base * 0.05;
		float mountain_amp;
		if (n_valley > 0.0) {
			// valley sides with varying height, chopped by plateaus
			n_valley = MYMIN(pow(n_valley, 1.5) * n_valley_amp, n_plateau);
			// multiply by n_valley to keep mountains away from rivers
			// also decrease mountain height away from ridges
			mountain_amp = n_mountain_amp * n_valley * MYMAX(1.0 + n_valley_base, 0.0);
		} else {
			// river channel
			// make n_valley positive to allow fractional exponent
			// create river cross section shape and multiply for depth
			n_valley = -pow(-n_valley, 0.25) * 0.1;
			mountain_amp = 0.0;
		}

		u32 i = vm->m_area.index(x, node_min.Y - 1, z);
		u32 index3d = (z - node_min.Z) * zstride + (x - node_min.X);
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			float n_mountain = (1.0 + noise_mountain->result[index3d]) * mountain_amp;
			float density_gradient = -y / 128.0;
			// actual valley base, roughly river surface level
			float density_valley_base = n_ridge + n_valley_base + density_gradient;
			// actual solid terrain level
			float density = density_valley_base + n_valley + n_mountain;

			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (density > 0.0) {
					if (n_valley < 0.0 && density < 0.02)
						vm->m_data[i] = n_sand;  // riverbed sand
					else
						vm->m_data[i] = n_stone;
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				} else if (y <= water_level) {
					vm->m_data[i] = n_water;
				} else if (density_valley_base > 0.005) {  // river lower than bank
					vm->m_data[i] = n_river_water;
				} else {
					vm->m_data[i] = n_air;
				}
			}
			vm->m_area.add_y(em, i, 1);
			index3d += ystride;
		}
	}

	return stone_surface_max_y;
}


MgStoneType MapgenWatershed::generateBiomes(float *heat_map, float *humidity_map)
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
		bool water_above = (c_above == c_water_source) ||
			(c_above == c_river_water_source);

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
					((c == c_water_source || c == c_river_water_source) &&
					(air_above || !biome))) {
				biome = bmgr->getBiome(heat_map[index], humidity_map[index], y);
				depth_top = biome->depth_top;
				base_filler = depth_top + biome->depth_filler;
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
				if (c_below == CONTENT_AIR || c_below == c_water_source ||
						c_below == c_river_water_source)
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
				vm->m_data[vi] = MapNode((y > (s32)(water_level - depth_water_top)) ?
						biome->c_water_top : biome->c_water);
				nplaced = 0;  // Enable top/filler placement for next surface
				air_above = false;
				water_above = true;
			} else if (c == c_river_water_source) {
				vm->m_data[vi] = MapNode(biome->c_river_water);
				nplaced = U16_MAX; // Disable because riverbed sand is present
				air_above = false;
				water_above = true;
			} else if (c == CONTENT_AIR) {
				nplaced = 0;  // Enable top/filler placement for next surface
				air_above = true;
				water_above = false;
			} else {  // Riverbed sand or possible various nodes overgenerated
				// from neighbouring mapchunks
				nplaced = U16_MAX;  // Disable top/filler placement
				air_above = false;
				water_above = false;
			}

			vm->m_area.add_y(em, vi, -1);
		}
	}

	return stone_type;
}


void MapgenWatershed::dustTopNodes()
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


void MapgenWatershed::generateCaves(int max_stone_y)
{
	if (max_stone_y >= node_min.Y) {

		// YZX order to avoid repeating y-dependent calculations
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			float cave_amp = MYMIN(-y / 128.0, 1.0);
			float cavem_off = -0.00015 * pow(y - MGWATERSHED_LAVA_CAVE_LEVEL, 2.0);
			u32 ni2d = 0;
			for (s16 z = node_min.Z; z <= node_max.Z; z++) {
				u32 ni3d = (z - node_min.Z) * zstride + (y - node_min.Y + 1) * ystride;
				u32 vi = vm->m_area.index(node_min.X, y, z);
				for (s16 x = node_min.X; x <= node_max.X; x++, vi++, ni2d++, ni3d++) {
					// Check 2D noises to keep tunnels away from rivers
					float n_valley = fabs(noise_valley->result[ni2d]);

					content_t c = vm->m_data[vi].getContent();
					if (!ndef->get(c).is_ground_content || c == CONTENT_AIR ||
							(n_valley < 0.1 && y > water_level)) // Near river
						continue;

					float n_cavem = noise_mountain->result[ni3d];
					float nabs_cavem = fabs(n_cavem);
					float nabs_cave1 = fabs(noise_cave1->result[ni3d]);
					float nabs_cave2 = fabs(noise_cave2->result[ni3d]);
					float nabs_cave3 = fabs(noise_cave3->result[ni3d]);

					bool is_cavem =
						n_cavem + cavem_off > MGWATERSHED_LAVA_CAVE_THRESHOLD;
					bool is_cave =
						nabs_cave1 * cave_amp > MGWATERSHED_CAVE_THRESHOLD ||
						nabs_cave2 * cave_amp > MGWATERSHED_CAVE_THRESHOLD ||
						nabs_cave3 * cave_amp > MGWATERSHED_CAVE_THRESHOLD;
					bool is_tunnel =
						nabs_cave1 + nabs_cave2 < MGWATERSHED_TUNNEL_WIDTH ||
						nabs_cave2 + nabs_cave3 < MGWATERSHED_TUNNEL_WIDTH ||
						nabs_cave3 + nabs_cavem < MGWATERSHED_TUNNEL_WIDTH ||
						nabs_cavem + nabs_cave1 < MGWATERSHED_TUNNEL_WIDTH;

					// If lava cave or overlap of lava cave and normal cave
					if ((is_cavem || (is_cavem && is_cave))
							&& y <= MGWATERSHED_LAVA_LEVEL)
						vm->m_data[vi] = MapNode(c_lava_source);
					else if (is_cave || is_cavem || is_tunnel)
						vm->m_data[vi] = MapNode(CONTENT_AIR);
				}
			}
		}
	}
}
