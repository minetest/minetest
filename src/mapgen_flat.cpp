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
//#include "profiler.h" // For TimeTaker
#include "settings.h" // For g_settings
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "treegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_flat.h"


FlagDesc flagdesc_mapgen_flat[] = {
	{"lakes", MGFLAT_LAKES},
	{"hills", MGFLAT_HILLS},
	{NULL,    0}
};

///////////////////////////////////////////////////////////////////////////////////////


MapgenFlat::MapgenFlat(int mapgenid, MapgenParams *params, EmergeManager *emerge)
	: Mapgen(mapgenid, params, emerge)
{
	this->m_emerge = emerge;
	this->bmgr     = emerge->biomemgr;

	//// amount of elements to skip for the next index
	//// for noise/height/biome maps (not vmanip)
	this->ystride = csize.X;
	// 1-down overgeneration
	this->zstride_1d = csize.X * (csize.Y + 1);

	this->biomemap        = new u8[csize.X * csize.Z];
	this->heightmap       = new s16[csize.X * csize.Z];
	this->heatmap         = NULL;
	this->humidmap        = NULL;

	MapgenFlatParams *sp = (MapgenFlatParams *)params->sparams;

	this->spflags          = sp->spflags;
	this->ground_level     = sp->ground_level;
	this->large_cave_depth = sp->large_cave_depth;
	this->cave_width       = sp->cave_width;
	this->lake_threshold   = sp->lake_threshold;
	this->lake_steepness   = sp->lake_steepness;
	this->hill_threshold   = sp->hill_threshold;
	this->hill_steepness   = sp->hill_steepness;

	//// 2D noise
	noise_terrain      = new Noise(&sp->np_terrain,      seed, csize.X, csize.Z);
	noise_filler_depth = new Noise(&sp->np_filler_depth, seed, csize.X, csize.Z);

	//// 3D noise
	// 1-down overgeneraion
	noise_cave1 = new Noise(&sp->np_cave1, seed, csize.X, csize.Y + 1, csize.Z);
	noise_cave2 = new Noise(&sp->np_cave2, seed, csize.X, csize.Y + 1, csize.Z);

	//// Biome noise
	noise_heat           = new Noise(&params->np_biome_heat,           seed, csize.X, csize.Z);
	noise_humidity       = new Noise(&params->np_biome_humidity,       seed, csize.X, csize.Z);
	noise_heat_blend     = new Noise(&params->np_biome_heat_blend,     seed, csize.X, csize.Z);
	noise_humidity_blend = new Noise(&params->np_biome_humidity_blend, seed, csize.X, csize.Z);

	//// Resolve nodes to be used
	INodeDefManager *ndef = emerge->ndef;

	c_stone                = ndef->getId("mapgen_stone");
	c_water_source         = ndef->getId("mapgen_water_source");
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


MapgenFlat::~MapgenFlat()
{
	delete noise_terrain;
	delete noise_filler_depth;
	delete noise_cave1;
	delete noise_cave2;

	delete noise_heat;
	delete noise_humidity;
	delete noise_heat_blend;
	delete noise_humidity_blend;

	delete[] heightmap;
	delete[] biomemap;
}


MapgenFlatParams::MapgenFlatParams()
{
	spflags          = 0;
	ground_level     = 8;
	large_cave_depth = -33;
	cave_width       = 0.3;
	lake_threshold   = -0.45;
	lake_steepness   = 48.0;
	hill_threshold   = 0.45;
	hill_steepness   = 64.0;

	np_terrain      = NoiseParams(0, 1,   v3f(600, 600, 600), 7244,  5, 0.6, 2.0);
	np_filler_depth = NoiseParams(0, 1.2, v3f(150, 150, 150), 261,   3, 0.7, 2.0);
	np_cave1        = NoiseParams(0, 12,  v3f(96,  96,  96),  52534, 4, 0.5, 2.0);
	np_cave2        = NoiseParams(0, 12,  v3f(96,  96,  96),  10325, 4, 0.5, 2.0);
}


void MapgenFlatParams::readParams(const Settings *settings)
{
	settings->getFlagStrNoEx("mgflat_spflags",      spflags, flagdesc_mapgen_flat);
	settings->getS16NoEx("mgflat_ground_level",     ground_level);
	settings->getS16NoEx("mgflat_large_cave_depth", large_cave_depth);
	settings->getFloatNoEx("mgflat_cave_width",     cave_width);
	settings->getFloatNoEx("mgflat_lake_threshold", lake_threshold);
	settings->getFloatNoEx("mgflat_lake_steepness", lake_steepness);
	settings->getFloatNoEx("mgflat_hill_threshold", hill_threshold);
	settings->getFloatNoEx("mgflat_hill_steepness", hill_steepness);

	settings->getNoiseParams("mgflat_np_terrain",      np_terrain);
	settings->getNoiseParams("mgflat_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgflat_np_cave1",        np_cave1);
	settings->getNoiseParams("mgflat_np_cave2",        np_cave2);
}


void MapgenFlatParams::writeParams(Settings *settings) const
{
	settings->setFlagStr("mgflat_spflags",      spflags, flagdesc_mapgen_flat, U32_MAX);
	settings->setS16("mgflat_ground_level",     ground_level);
	settings->setS16("mgflat_large_cave_depth", large_cave_depth);
	settings->setFloat("mgflat_cave_width",     cave_width);
	settings->setFloat("mgflat_lake_threshold", lake_threshold);
	settings->setFloat("mgflat_lake_steepness", lake_steepness);
	settings->setFloat("mgflat_hill_threshold", hill_threshold);
	settings->setFloat("mgflat_hill_steepness", hill_steepness);

	settings->setNoiseParams("mgflat_np_terrain",      np_terrain);
	settings->setNoiseParams("mgflat_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgflat_np_cave1",        np_cave1);
	settings->setNoiseParams("mgflat_np_cave2",        np_cave2);
}


/////////////////////////////////////////////////////////////////


int MapgenFlat::getSpawnLevelAtPoint(v2s16 p)
{
	s16 level_at_point = ground_level;
	float n_terrain = NoisePerlin2D(&noise_terrain->np, p.X, p.Y, seed);

	if ((spflags & MGFLAT_LAKES) && n_terrain < lake_threshold) {
		level_at_point = ground_level -
			(lake_threshold - n_terrain) * lake_steepness;
	} else if ((spflags & MGFLAT_HILLS) && n_terrain > hill_threshold) {
		level_at_point = ground_level +
			(n_terrain - hill_threshold) * hill_steepness;
	}

	if (ground_level < water_level)  // Ocean world, allow spawn in water
		return MYMAX(level_at_point, water_level);
	else if (level_at_point > water_level)
		return level_at_point;  // Spawn on land
	else
		return MAX_MAP_GENERATION_LIMIT;  // Unsuitable spawn point
}


void MapgenFlat::makeChunk(BlockMakeData *data)
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
	if (flags & MG_DECORATIONS)
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

	//setLighting(node_min - v3s16(1, 0, 1) * MAP_BLOCKSIZE,
	//			node_max + v3s16(1, 0, 1) * MAP_BLOCKSIZE, 0xFF);

	this->generating = false;
}


void MapgenFlat::calculateNoise()
{
	//TimeTaker t("calculateNoise", NULL, PRECISION_MICRO);
	s16 x = node_min.X;
	s16 z = node_min.Z;

	if ((spflags & MGFLAT_LAKES) || (spflags & MGFLAT_HILLS))
		noise_terrain->perlinMap2D(x, z);

	// Cave noises are calculated in generateCaves()
	// only if solid terrain is present in mapchunk

	noise_filler_depth->perlinMap2D(x, z);
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


s16 MapgenFlat::generateTerrain()
{
	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	v3s16 em = vm->m_area.getExtent();
	s16 stone_surface_max_y = -MAX_MAP_GENERATION_LIMIT;
	u32 ni2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, ni2d++) {
		s16 stone_level = ground_level;
		float n_terrain = 0.0f;

		if ((spflags & MGFLAT_LAKES) || (spflags & MGFLAT_HILLS))
			n_terrain = noise_terrain->result[ni2d];

		if ((spflags & MGFLAT_LAKES) && n_terrain < lake_threshold) {
			s16 depress = (lake_threshold - n_terrain) * lake_steepness;
			stone_level = ground_level - depress;
		} else if ((spflags & MGFLAT_HILLS) && n_terrain > hill_threshold) {
			s16 rise = (n_terrain - hill_threshold) * hill_steepness;
		 	stone_level = ground_level + rise;
		}

		u32 vi = vm->m_area.index(x, node_min.Y - 1, z);
		for (s16 y = node_min.Y - 1; y <= node_max.Y + 1; y++) {
			if (vm->m_data[vi].getContent() == CONTENT_IGNORE) {
				if (y <= stone_level) {
					vm->m_data[vi] = n_stone;
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				} else if (y <= water_level) {
					vm->m_data[vi] = n_water;
				} else {
					vm->m_data[vi] = n_air;
				}
			}
			vm->m_area.add_y(em, vi, 1);
		}
	}

	return stone_surface_max_y;
}


MgStoneType MapgenFlat::generateBiomes(float *heat_map, float *humidity_map)
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
		bool water_above = c_above == c_water_source;

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
					(c == c_water_source && (air_above || !biome))) {
				biome = bmgr->getBiome(heat_map[index], humidity_map[index], y);
				depth_top = biome->depth_top;
				base_filler = MYMAX(depth_top + biome->depth_filler
					+ noise_filler_depth->result[index], 0);
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
				if (c_below == CONTENT_AIR || c_below == c_water_source)
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


void MapgenFlat::dustTopNodes()
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


void MapgenFlat::generateCaves(s16 max_stone_y)
{
	if (max_stone_y < node_min.Y)
		return;

	noise_cave1->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);
	noise_cave2->perlinMap3D(node_min.X, node_min.Y - 1, node_min.Z);

	v3s16 em = vm->m_area.getExtent();
	u32 index2d = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index2d++) {
		bool column_is_open = false;  // Is column open to overground
		bool is_tunnel = false;  // Is tunnel or tunnel floor
		u32 vi = vm->m_area.index(x, node_max.Y, z);
		u32 index3d = (z - node_min.Z) * zstride_1d + csize.Y * ystride +
			(x - node_min.X);
		// Biome of column
		Biome *biome = (Biome *)bmgr->getRaw(biomemap[index2d]);

		// Don't excavate the overgenerated stone at node_max.Y + 1,
		// this creates a 'roof' over the tunnel, preventing light in
		// tunnels at mapchunk borders when generating mapchunks upwards.
		// This 'roof' is removed when the mapchunk above is generated.
		for (s16 y = node_max.Y; y >= node_min.Y - 1; y--,
				index3d -= ystride,
				vm->m_area.add_y(em, vi, -1)) {

			content_t c = vm->m_data[vi].getContent();
			if (c == CONTENT_AIR || c == biome->c_water_top ||
					c == biome->c_water) {
				column_is_open = true;
				continue;
			}
			// Ground
			float d1 = contour(noise_cave1->result[index3d]);
			float d2 = contour(noise_cave2->result[index3d]);

			if (d1 * d2 > cave_width && ndef->get(c).is_ground_content) {
				// In tunnel and ground content, excavate
				vm->m_data[vi] = MapNode(CONTENT_AIR);
				is_tunnel = true;
			} else {
				// Not in tunnel or not ground content
				if (is_tunnel && column_is_open &&
						(c == biome->c_filler || c == biome->c_stone))
					// Tunnel entrance floor
					vm->m_data[vi] = MapNode(biome->c_top);

				column_is_open = false;
				is_tunnel = false;
			}
		}
	}

	if (node_max.Y > large_cave_depth)
		return;

	PseudoRandom ps(blockseed + 21343);
	u32 bruises_count = ps.range(0, 2);
	for (u32 i = 0; i < bruises_count; i++) {
		CaveV5 cave(this, &ps);
		cave.makeCave(node_min, node_max, max_stone_y);
	}
}
