/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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
#include "profiler.h"
#include "settings.h" // For g_settings
#include "main.h" // For g_profiler
#include "emerge.h"
#include "dungeongen.h"
#include "cavegen.h"
#include "treegen.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mapgen_v5.h"
#include "util/directiontables.h"


FlagDesc flagdesc_mapgen_v5[] = {
	{"blobs", MGV5_BLOBS},
	{NULL,         0}
};


MapgenV5::MapgenV5(int mapgenid, MapgenParams *params, EmergeManager *emerge)
	: Mapgen(mapgenid, params, emerge)
{
	this->m_emerge = emerge;
	this->bmgr     = emerge->biomemgr;

	// amount of elements to skip for the next index
	// for noise/height/biome maps (not vmanip)
	this->ystride = csize.X;
	this->zstride = csize.X * (csize.Y + 2);

	this->biomemap  = new u8[csize.X * csize.Z];
	this->heightmap = new s16[csize.X * csize.Z];

	MapgenV5Params *sp = (MapgenV5Params *)params->sparams;
	this->spflags      = sp->spflags;

	// Terrain noise
	noise_filler_depth = new Noise(&sp->np_filler_depth, seed, csize.X, csize.Z);
	noise_factor       = new Noise(&sp->np_factor,       seed, csize.X, csize.Z);
	noise_height       = new Noise(&sp->np_height,       seed, csize.X, csize.Z);

	// 3D terrain noise
	noise_cave1        = new Noise(&sp->np_cave1,   seed, csize.X, csize.Y + 2, csize.Z);
	noise_cave2        = new Noise(&sp->np_cave2,   seed, csize.X, csize.Y + 2, csize.Z);
	noise_ground       = new Noise(&sp->np_ground,  seed, csize.X, csize.Y + 2, csize.Z);
	noise_crumble      = new Noise(&sp->np_crumble, seed, csize.X, csize.Y + 2, csize.Z);
	noise_wetness      = new Noise(&sp->np_wetness, seed, csize.X, csize.Y + 2, csize.Z);

	// Biome noise
	noise_heat         = new Noise(&params->np_biome_heat,     seed, csize.X, csize.Z);
	noise_humidity     = new Noise(&params->np_biome_humidity, seed, csize.X, csize.Z);

	//// Resolve nodes to be used
	INodeDefManager *ndef = emerge->ndef;

	c_stone           = ndef->getId("mapgen_stone");
	c_dirt            = ndef->getId("mapgen_dirt");
	c_dirt_with_grass = ndef->getId("mapgen_dirt_with_grass");
	c_sand            = ndef->getId("mapgen_sand");
	c_water_source    = ndef->getId("mapgen_water_source");
	c_lava_source     = ndef->getId("mapgen_lava_source");
	c_gravel          = ndef->getId("mapgen_gravel");
	c_cobble          = ndef->getId("mapgen_cobble");
	c_ice             = ndef->getId("default:ice");
	c_mossycobble     = ndef->getId("mapgen_mossycobble");
	c_sandbrick       = ndef->getId("mapgen_sandstonebrick");
	c_stair_cobble    = ndef->getId("mapgen_stair_cobble");
	c_stair_sandstone = ndef->getId("mapgen_stair_sandstone");
	if (c_ice == CONTENT_IGNORE)
		c_ice = CONTENT_AIR;
	if (c_mossycobble == CONTENT_IGNORE)
		c_mossycobble = c_cobble;
	if (c_sandbrick == CONTENT_IGNORE)
		c_sandbrick = c_desert_stone;
	if (c_stair_cobble == CONTENT_IGNORE)
		c_stair_cobble = c_cobble;
	if (c_stair_sandstone == CONTENT_IGNORE)
		c_stair_sandstone = c_sandbrick;
}


MapgenV5::~MapgenV5()
{
	delete noise_filler_depth;
	delete noise_factor;
	delete noise_height;
	delete noise_cave1;
	delete noise_cave2;
	delete noise_ground;
	delete noise_crumble;
	delete noise_wetness;

	delete noise_heat;
	delete noise_humidity;

	delete[] heightmap;
	delete[] biomemap;
}


MapgenV5Params::MapgenV5Params()
{
	spflags = MGV5_BLOBS;

	np_filler_depth = NoiseParams(0, 1,  v3f(150, 150, 150), 261,    4, 0.7,  2.0);
	np_factor       = NoiseParams(0, 1,  v3f(250, 250, 250), 920381, 3, 0.45, 2.0);
	np_height       = NoiseParams(0, 10, v3f(250, 250, 250), 84174,  4, 0.5,  2.0);
	np_cave1        = NoiseParams(0, 12, v3f(50,  50,  50),  52534,  4, 0.5,  2.0);
	np_cave2        = NoiseParams(0, 12, v3f(50,  50,  50),  10325,  4, 0.5,  2.0);
	np_ground       = NoiseParams(0, 40, v3f(80,  80,  80),  983240, 4, 0.55, 2.0, NOISE_FLAG_EASED);
	np_crumble      = NoiseParams(0, 1,  v3f(20,  20,  20),  34413,  3, 1.3,  2.0, NOISE_FLAG_EASED);
	np_wetness      = NoiseParams(0, 1,  v3f(40,  40,  40),  32474,  4, 1.1,  2.0);
}


// Scaling the output of the noise function affects the overdrive of the
// contour function, which affects the shape of the output considerably.

// Two original MT 0.3 parameters for non-eased noise:

//#define CAVE_NOISE_SCALE 12.0
//#define CAVE_NOISE_THRESHOLD (1.5/CAVE_NOISE_SCALE)


void MapgenV5Params::readParams(Settings *settings)
{
	settings->getFlagStrNoEx("mgv5_spflags", spflags, flagdesc_mapgen_v5);

	settings->getNoiseParams("mgv5_np_filler_depth", np_filler_depth);
	settings->getNoiseParams("mgv5_np_factor",       np_factor);
	settings->getNoiseParams("mgv5_np_height",       np_height);
	settings->getNoiseParams("mgv5_np_cave1",        np_cave1);
	settings->getNoiseParams("mgv5_np_cave2",        np_cave2);
	settings->getNoiseParams("mgv5_np_ground",       np_ground);
	settings->getNoiseParams("mgv5_np_crumble",      np_crumble);
	settings->getNoiseParams("mgv5_np_wetness",      np_wetness);
}


void MapgenV5Params::writeParams(Settings *settings)
{
	settings->setFlagStr("mgv5_spflags", spflags, flagdesc_mapgen_v5, (u32)-1);

	settings->setNoiseParams("mgv5_np_filler_depth", np_filler_depth);
	settings->setNoiseParams("mgv5_np_factor",       np_factor);
	settings->setNoiseParams("mgv5_np_height",       np_height);
	settings->setNoiseParams("mgv5_np_cave1",        np_cave1);
	settings->setNoiseParams("mgv5_np_cave2",        np_cave2);
	settings->setNoiseParams("mgv5_np_ground",       np_ground);
	settings->setNoiseParams("mgv5_np_crumble",      np_crumble);
	settings->setNoiseParams("mgv5_np_wetness",      np_wetness);
}


int MapgenV5::getGroundLevelAtPoint(v2s16 p)
{
	//TimeTaker t("getGroundLevelAtPoint", NULL, PRECISION_MICRO);

	float f = 0.55 + NoisePerlin2D(&noise_factor->np, p.X, p.Y, seed);
	if(f < 0.01)
		f = 0.01;
	else if(f >= 1.0)
		f *= 1.6;
	float h = water_level + NoisePerlin2D(&noise_height->np, p.X, p.Y, seed);

	s16 search_top = water_level + 15;
	s16 search_base = water_level;
	// Use these 2 lines instead for a slower search returning highest ground level:
	//s16 search_top = h + f * noise_ground->np->octaves * noise_ground->np->scale;
	//s16 search_base = h - f * noise_ground->np->octaves * noise_ground->np->scale;

	s16 level = -31000;
	for (s16 y = search_top; y >= search_base; y--) {
		float n_ground = NoisePerlin3D(&noise_ground->np, p.X, y, p.Y, seed);
		if(n_ground * f > y - h) {
			if(y >= search_top - 7)
				break;
			else
				level = y;
				break;
		}
	}

	//printf("getGroundLevelAtPoint: %dus\n", t.stop());
	return level;
}


void MapgenV5::makeChunk(BlockMakeData *data)
{
	assert(data->vmanip);
	assert(data->nodedef);
	assert(data->blockpos_requested.X >= data->blockpos_min.X &&
		data->blockpos_requested.Y >= data->blockpos_min.Y &&
		data->blockpos_requested.Z >= data->blockpos_min.Z);
	assert(data->blockpos_requested.X <= data->blockpos_max.X &&
		data->blockpos_requested.Y <= data->blockpos_max.Y &&
		data->blockpos_requested.Z <= data->blockpos_max.Z);

	generating = true;
	vm   = data->vmanip;
	ndef = data->nodedef;
	//TimeTaker t("makeChunk");

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);
	full_node_min = (blockpos_min - 1) * MAP_BLOCKSIZE;
	full_node_max = (blockpos_max + 2) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	// Create a block-specific seed
	blockseed = getBlockSeed2(full_node_min, seed);

	// Make some noise
	calculateNoise();

	// Generate base terrain
	s16 stone_surface_max_y = generateBaseTerrain();
	updateHeightmap(node_min, node_max);

	// Calculate biomes
	bmgr->calcBiomes(csize.X, csize.Z, noise_heat->result,
		noise_humidity->result, heightmap, biomemap);

	// Actually place the biome-specific nodes
	generateBiomes();

	// Generate caves
	if ((flags & MG_CAVES) && (stone_surface_max_y >= node_min.Y))
		generateCaves();

	// Generate dungeons and desert temples
	if ((flags & MG_DUNGEONS) && (stone_surface_max_y >= node_min.Y)) {
		DungeonGen dgen(this, NULL);
		dgen.generate(blockseed, full_node_min, full_node_max);
	}

	// Generate the registered decorations
	m_emerge->decomgr->placeAllDecos(this, blockseed, node_min, node_max);

	// Generate underground dirt, sand, gravel and lava blobs
	if (spflags & MGV5_BLOBS) {
		generateBlobs();
	}

	// Generate the registered ores
	m_emerge->oremgr->placeAllOres(this, blockseed, node_min, node_max);

	// Sprinkle some dust on top after everything else was generated
	dustTopNodes();

	//printf("makeChunk: %dms\n", t.stop());

	// Add top and bottom side of water to transforming_liquid queue
	updateLiquid(&data->transforming_liquid, full_node_min, full_node_max);

	// Calculate lighting
	if (flags & MG_LIGHT) {
		calcLighting(node_min - v3s16(0, 1, 0), node_max + v3s16(0, 1, 0),
			full_node_min, full_node_max);
	}

	this->generating = false;
}


void MapgenV5::calculateNoise()
{
	//TimeTaker t("calculateNoise", NULL, PRECISION_MICRO);
	int x = node_min.X;
	int y = node_min.Y - 1;
	int z = node_min.Z;

	noise_factor->perlinMap2D(x, z);
	noise_height->perlinMap2D(x, z);
	noise_ground->perlinMap3D(x, y, z);

	if (flags & MG_CAVES) {
		noise_cave1->perlinMap3D(x, y, z);
		noise_cave2->perlinMap3D(x, y, z);
	}

	if (spflags & MGV5_BLOBS) {
		noise_crumble->perlinMap3D(x, y, z);
		noise_wetness->perlinMap3D(x, y, z);
	}

	if (node_max.Y >= water_level) {
		noise_filler_depth->perlinMap2D(x, z);
		noise_heat->perlinMap2D(x, z);
		noise_humidity->perlinMap2D(x, z);
	}

	//printf("calculateNoise: %dus\n", t.stop());
}


// Two original MT 0.3 functions:

//bool is_cave(u32 index) {
//	double d1 = contour(noise_cave1->result[index]);
//	double d2 = contour(noise_cave2->result[index]);
//	return d1*d2 > CAVE_NOISE_THRESHOLD;
//}

//bool val_is_ground(v3s16 p, u32 index, u32 index2d) {
//	double f = 0.55 + noise_factor->result[index2d];
//	if(f < 0.01)
//		f = 0.01;
//	else if(f >= 1.0)
//		f *= 1.6;
//	double h = WATER_LEVEL + 10 * noise_height->result[index2d];
//	return (noise_ground->result[index] * f > (double)p.Y - h);
//}


int MapgenV5::generateBaseTerrain()
{
	u32 index = 0;
	u32 index2d = 0;
	int stone_surface_max_y = -MAP_GENERATION_LIMIT;

	for(s16 z=node_min.Z; z<=node_max.Z; z++) {
		for(s16 y=node_min.Y - 1; y<=node_max.Y + 1; y++) {
			u32 i = vm->m_area.index(node_min.X, y, z);
			for(s16 x=node_min.X; x<=node_max.X; x++, i++, index++, index2d++) {
				if(vm->m_data[i].getContent() != CONTENT_IGNORE)
					continue;

				float f = 0.55 + noise_factor->result[index2d];
				if(f < 0.01)
					f = 0.01;
				else if(f >= 1.0)
					f *= 1.6;
				float h = water_level + noise_height->result[index2d];

				if(noise_ground->result[index] * f < y - h) {
					if(y <= water_level)
						vm->m_data[i] = MapNode(c_water_source);
					else
						vm->m_data[i] = MapNode(CONTENT_AIR);
				} else {
					vm->m_data[i] = MapNode(c_stone);
					if (y > stone_surface_max_y)
						stone_surface_max_y = y;
				}
			}
			index2d = index2d - ystride;
		}
		index2d = index2d + ystride;
	}

	return stone_surface_max_y;
}


void MapgenV5::generateBiomes()
{
	if (node_max.Y < water_level)
		return;

	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);
	MapNode n_water(c_water_source);

	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome        = (Biome *)bmgr->get(biomemap[index]);
		s16 dfiller         = biome->depth_filler + noise_filler_depth->result[index];
		s16 y0_top          = biome->depth_top;
		s16 y0_filler       = biome->depth_top + dfiller;
		s16 shore_max       = water_level + biome->height_shore;
		s16 depth_water_top = biome->depth_water_top;

		s16 nplaced = 0;
		u32 i = vm->m_area.index(x, node_max.Y, z);

		content_t c_above = vm->m_data[i + em.X].getContent();
		bool have_air = c_above == CONTENT_AIR;

		for (s16 y = node_max.Y; y >= node_min.Y; y--) {
			content_t c = vm->m_data[i].getContent();

			if (c == c_stone && have_air) {
				content_t c_below = vm->m_data[i - em.X].getContent();

				if (c_below != CONTENT_AIR) {
					if (nplaced < y0_top) {
						if(y < water_level)
							vm->m_data[i] = MapNode(biome->c_underwater);
						else if(y <= shore_max)
							vm->m_data[i] = MapNode(biome->c_shore_top);
						else
							vm->m_data[i] = MapNode(biome->c_top);
						nplaced++;
					} else if (nplaced < y0_filler && nplaced >= y0_top) {
						if(y < water_level)
							vm->m_data[i] = MapNode(biome->c_underwater);
						else if(y <= shore_max)
							vm->m_data[i] = MapNode(biome->c_shore_filler);
						else
							vm->m_data[i] = MapNode(biome->c_filler);
						nplaced++;
					} else if (c == c_stone) {
						have_air = false;
						nplaced  = 0;
						vm->m_data[i] = MapNode(biome->c_stone);
					} else {
						have_air = false;
						nplaced  = 0;
					}
				} else if (c == c_stone) {
					have_air = false;
					nplaced = 0;
					vm->m_data[i] = MapNode(biome->c_stone);
				}
			} else if (c == c_stone) {
				have_air = false;
				nplaced = 0;
				vm->m_data[i] = MapNode(biome->c_stone);
			} else if (c == c_water_source) {
				have_air = true;
				nplaced = 0;
				if(y > water_level - depth_water_top)
					vm->m_data[i] = MapNode(biome->c_water_top);
				else
					vm->m_data[i] = MapNode(biome->c_water);
			} else if (c == CONTENT_AIR) {
				have_air = true;
				nplaced = 0;
			}

			vm->m_area.add_y(em, i, -1);
		}
	}
}


void MapgenV5::generateCaves()
{
	u32 index = 0;
	u32 index2d = 0;

	for(s16 z=node_min.Z; z<=node_max.Z; z++) {
		for(s16 y=node_min.Y - 1; y<=node_max.Y + 1; y++) {
			u32 i = vm->m_area.index(node_min.X, y, z);
			for(s16 x=node_min.X; x<=node_max.X; x++, i++, index++, index2d++) {
				Biome *biome = (Biome *)bmgr->get(biomemap[index2d]);
				content_t c = vm->m_data[i].getContent();
				if(c == CONTENT_AIR
						|| (y <= water_level
						&& c != biome->c_stone
						&& c != c_stone))
					continue;

				float d1 = contour(noise_cave1->result[index]);
				float d2 = contour(noise_cave2->result[index]);
				if(d1*d2 > 0.125)
					vm->m_data[i] = MapNode(CONTENT_AIR);
			}
			index2d = index2d - ystride;
		}
		index2d = index2d + ystride;
	}
}


void MapgenV5::generateBlobs()
{
	u32 index = 0;

	for(s16 z=node_min.Z; z<=node_max.Z; z++) {
		for(s16 y=node_min.Y - 1; y<=node_max.Y + 1; y++) {
			u32 i = vm->m_area.index(node_min.X, y, z);
			for(s16 x=node_min.X; x<=node_max.X; x++, i++, index++) {
				content_t c = vm->m_data[i].getContent();
				if(c != c_stone)
					continue;

				if(noise_crumble->result[index] > 1.3) {
					if(noise_wetness->result[index] > 0.0)
						vm->m_data[i] = MapNode(c_dirt);
					else
						vm->m_data[i] = MapNode(c_sand);
				} else if(noise_crumble->result[index] > 0.7) {
					if(noise_wetness->result[index] < -0.6)
						vm->m_data[i] = MapNode(c_gravel);
				} else if(noise_crumble->result[index] < -3.5 +
						MYMIN(0.1 *
						sqrt((float)MYMAX(0, -y)), 1.5)) {
					vm->m_data[i] = MapNode(c_lava_source);
				}
			}
		}
	}
}


void MapgenV5::dustTopNodes()
{
	if (node_max.Y < water_level)
		return;

	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome = (Biome *)bmgr->get(biomemap[index]);

		if (biome->c_dust == CONTENT_IGNORE)
			continue;

		s16 y = node_max.Y + 1;
		u32 vi = vm->m_area.index(x, y, z);
		for (; y >= node_min.Y; y--) {
			if (vm->m_data[vi].getContent() != CONTENT_AIR)
				break;

			vm->m_area.add_y(em, vi, -1);
		}

		content_t c = vm->m_data[vi].getContent();
		if (!ndef->get(c).buildable_to && c != CONTENT_IGNORE
				&& c != biome->c_dust) {
			if (y == node_max.Y + 1)
				continue;

			vm->m_area.add_y(em, vi, 1);
			vm->m_data[vi] = MapNode(biome->c_dust);
		}
	}
}

