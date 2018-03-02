/*
Minetest
Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2015-2017 paramat

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
#include "gamedef.h"
#include "mg_biome.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "content_sao.h"
#include "nodedef.h"
#include "emerge.h"
#include "voxelalgorithms.h"
#include "porting.h"
#include "profiler.h"
#include "settings.h"
#include "treegen.h"
#include "serialization.h"
#include "util/serialize.h"
#include "util/numeric.h"
#include "filesys.h"
#include "log.h"
#include "mapgen_flat.h"
#include "mapgen_fractal.h"
#include "mapgen_v5.h"
#include "mapgen_v6.h"
#include "mapgen_v7.h"
#include "mapgen_valleys.h"
#include "mapgen_singlenode.h"
#include "cavegen.h"
#include "dungeongen.h"

FlagDesc flagdesc_mapgen[] = {
	{"caves",       MG_CAVES},
	{"dungeons",    MG_DUNGEONS},
	{"light",       MG_LIGHT},
	{"decorations", MG_DECORATIONS},
	{NULL,       0}
};

FlagDesc flagdesc_gennotify[] = {
	{"dungeon",          1 << GENNOTIFY_DUNGEON},
	{"temple",           1 << GENNOTIFY_TEMPLE},
	{"cave_begin",       1 << GENNOTIFY_CAVE_BEGIN},
	{"cave_end",         1 << GENNOTIFY_CAVE_END},
	{"large_cave_begin", 1 << GENNOTIFY_LARGECAVE_BEGIN},
	{"large_cave_end",   1 << GENNOTIFY_LARGECAVE_END},
	{"decoration",       1 << GENNOTIFY_DECORATION},
	{NULL,               0}
};

struct MapgenDesc {
	const char *name;
	bool is_user_visible;
};

////
//// Built-in mapgens
////

static MapgenDesc g_reg_mapgens[] = {
	{"v5",         true},
	{"v6",         true},
	{"v7",         true},
	{"flat",       true},
	{"fractal",    true},
	{"valleys",    true},
	{"singlenode", true},
};

STATIC_ASSERT(
	ARRLEN(g_reg_mapgens) == MAPGEN_INVALID,
	registered_mapgens_is_wrong_size);

////
//// Mapgen
////

Mapgen::Mapgen()
{
	generating   = false;
	id           = -1;
	seed         = 0;
	water_level  = 0;
	mapgen_limit = 0;
	flags        = 0;

	vm        = NULL;
	ndef      = NULL;
	biomegen  = NULL;
	biomemap  = NULL;
	heightmap = NULL;
}


Mapgen::Mapgen(int mapgenid, MapgenParams *params, EmergeManager *emerge) :
	gennotify(emerge->gen_notify_on, &emerge->gen_notify_on_deco_ids)
{
	generating   = false;
	id           = mapgenid;
	water_level  = params->water_level;
	mapgen_limit = params->mapgen_limit;
	flags        = params->flags;
	csize        = v3s16(1, 1, 1) * (params->chunksize * MAP_BLOCKSIZE);

	/*
		We are losing half our entropy by doing this, but it is necessary to
		preserve reverse compatibility.  If the top half of our current 64 bit
		seeds ever starts getting used, existing worlds will break due to a
		different hash outcome and no way to differentiate between versions.

		A solution could be to add a new bit to designate that the top half of
		the seed value should be used, essentially a 1-bit version code, but
		this would require increasing the total size of a seed to 9 bytes (yuck)

		It's probably okay if this never gets fixed.  4.2 billion possibilities
		ought to be enough for anyone.
	*/
	seed = (s32)params->seed;

	vm        = NULL;
	ndef      = emerge->ndef;
	biomegen  = NULL;
	biomemap  = NULL;
	heightmap = NULL;
}


Mapgen::~Mapgen()
{
}


MapgenType Mapgen::getMapgenType(const std::string &mgname)
{
	for (size_t i = 0; i != ARRLEN(g_reg_mapgens); i++) {
		if (mgname == g_reg_mapgens[i].name)
			return (MapgenType)i;
	}

	return MAPGEN_INVALID;
}


const char *Mapgen::getMapgenName(MapgenType mgtype)
{
	size_t index = (size_t)mgtype;
	if (index == MAPGEN_INVALID || index >= ARRLEN(g_reg_mapgens))
		return "invalid";

	return g_reg_mapgens[index].name;
}


Mapgen *Mapgen::createMapgen(MapgenType mgtype, int mgid,
	MapgenParams *params, EmergeManager *emerge)
{
	switch (mgtype) {
	case MAPGEN_FLAT:
		return new MapgenFlat(mgid, (MapgenFlatParams *)params, emerge);
	case MAPGEN_FRACTAL:
		return new MapgenFractal(mgid, (MapgenFractalParams *)params, emerge);
	case MAPGEN_SINGLENODE:
		return new MapgenSinglenode(mgid, (MapgenSinglenodeParams *)params, emerge);
	case MAPGEN_V5:
		return new MapgenV5(mgid, (MapgenV5Params *)params, emerge);
	case MAPGEN_V6:
		return new MapgenV6(mgid, (MapgenV6Params *)params, emerge);
	case MAPGEN_V7:
		return new MapgenV7(mgid, (MapgenV7Params *)params, emerge);
	case MAPGEN_VALLEYS:
		return new MapgenValleys(mgid, (MapgenValleysParams *)params, emerge);
	default:
		return NULL;
	}
}


MapgenParams *Mapgen::createMapgenParams(MapgenType mgtype)
{
	switch (mgtype) {
	case MAPGEN_FLAT:
		return new MapgenFlatParams;
	case MAPGEN_FRACTAL:
		return new MapgenFractalParams;
	case MAPGEN_SINGLENODE:
		return new MapgenSinglenodeParams;
	case MAPGEN_V5:
		return new MapgenV5Params;
	case MAPGEN_V6:
		return new MapgenV6Params;
	case MAPGEN_V7:
		return new MapgenV7Params;
	case MAPGEN_VALLEYS:
		return new MapgenValleysParams;
	default:
		return NULL;
	}
}


void Mapgen::getMapgenNames(std::vector<const char *> *mgnames, bool include_hidden)
{
	for (u32 i = 0; i != ARRLEN(g_reg_mapgens); i++) {
		if (include_hidden || g_reg_mapgens[i].is_user_visible)
			mgnames->push_back(g_reg_mapgens[i].name);
	}
}


u32 Mapgen::getBlockSeed(v3s16 p, s32 seed)
{
	return (u32)seed   +
		p.Z * 38134234 +
		p.Y * 42123    +
		p.X * 23;
}


u32 Mapgen::getBlockSeed2(v3s16 p, s32 seed)
{
	u32 n = 1619 * p.X + 31337 * p.Y + 52591 * p.Z + 1013 * seed;
	n = (n >> 13) ^ n;
	return (n * (n * n * 60493 + 19990303) + 1376312589);
}


// Returns Y one under area minimum if not found
s16 Mapgen::findGroundLevelFull(v2s16 p2d)
{
	v3s16 em = vm->m_area.getExtent();
	s16 y_nodes_max = vm->m_area.MaxEdge.Y;
	s16 y_nodes_min = vm->m_area.MinEdge.Y;
	u32 i = vm->m_area.index(p2d.X, y_nodes_max, p2d.Y);
	s16 y;

	for (y = y_nodes_max; y >= y_nodes_min; y--) {
		MapNode &n = vm->m_data[i];
		if (ndef->get(n).walkable)
			break;

		vm->m_area.add_y(em, i, -1);
	}
	return (y >= y_nodes_min) ? y : y_nodes_min - 1;
}


// Returns -MAX_MAP_GENERATION_LIMIT if not found
s16 Mapgen::findGroundLevel(v2s16 p2d, s16 ymin, s16 ymax)
{
	v3s16 em = vm->m_area.getExtent();
	u32 i = vm->m_area.index(p2d.X, ymax, p2d.Y);
	s16 y;

	for (y = ymax; y >= ymin; y--) {
		MapNode &n = vm->m_data[i];
		if (ndef->get(n).walkable)
			break;

		vm->m_area.add_y(em, i, -1);
	}
	return (y >= ymin) ? y : -MAX_MAP_GENERATION_LIMIT;
}


// Returns -MAX_MAP_GENERATION_LIMIT if not found or if ground is found first
s16 Mapgen::findLiquidSurface(v2s16 p2d, s16 ymin, s16 ymax)
{
	v3s16 em = vm->m_area.getExtent();
	u32 i = vm->m_area.index(p2d.X, ymax, p2d.Y);
	s16 y;

	for (y = ymax; y >= ymin; y--) {
		MapNode &n = vm->m_data[i];
		if (ndef->get(n).walkable)
			return -MAX_MAP_GENERATION_LIMIT;
		else if (ndef->get(n).isLiquid())
			break;

		vm->m_area.add_y(em, i, -1);
	}
	return (y >= ymin) ? y : -MAX_MAP_GENERATION_LIMIT;
}


void Mapgen::updateHeightmap(v3s16 nmin, v3s16 nmax)
{
	if (!heightmap)
		return;

	//TimeTaker t("Mapgen::updateHeightmap", NULL, PRECISION_MICRO);
	int index = 0;
	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 x = nmin.X; x <= nmax.X; x++, index++) {
			s16 y = findGroundLevel(v2s16(x, z), nmin.Y, nmax.Y);

			heightmap[index] = y;
		}
	}
}

inline bool Mapgen::isLiquidHorizontallyFlowable(u32 vi, v3s16 em)
{
	u32 vi_neg_x = vi;
	vm->m_area.add_x(em, vi_neg_x, -1);
	if (vm->m_data[vi_neg_x].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_nx = ndef->get(vm->m_data[vi_neg_x]);
		if (c_nx.floodable && !c_nx.isLiquid())
			return true;
	}
	u32 vi_pos_x = vi;
	vm->m_area.add_x(em, vi_pos_x, +1);
	if (vm->m_data[vi_pos_x].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_px = ndef->get(vm->m_data[vi_pos_x]);
		if (c_px.floodable && !c_px.isLiquid())
			return true;
	}
	u32 vi_neg_z = vi;
	vm->m_area.add_z(em, vi_neg_z, -1);
	if (vm->m_data[vi_neg_z].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_nz = ndef->get(vm->m_data[vi_neg_z]);
		if (c_nz.floodable && !c_nz.isLiquid())
			return true;
	}
	u32 vi_pos_z = vi;
	vm->m_area.add_z(em, vi_pos_z, +1);
	if (vm->m_data[vi_pos_z].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_pz = ndef->get(vm->m_data[vi_pos_z]);
		if (c_pz.floodable && !c_pz.isLiquid())
			return true;
	}
	return false;
}

void Mapgen::updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax)
{
	bool isignored, isliquid, wasignored, wasliquid, waschecked, waspushed;
	v3s16 em  = vm->m_area.getExtent();

	for (s16 z = nmin.Z + 1; z <= nmax.Z - 1; z++)
	for (s16 x = nmin.X + 1; x <= nmax.X - 1; x++) {
		wasignored = true;
		wasliquid = false;
		waschecked = false;
		waspushed = false;

		u32 vi = vm->m_area.index(x, nmax.Y, z);
		for (s16 y = nmax.Y; y >= nmin.Y; y--) {
			isignored = vm->m_data[vi].getContent() == CONTENT_IGNORE;
			isliquid = ndef->get(vm->m_data[vi]).isLiquid();

			if (isignored || wasignored || isliquid == wasliquid) {
				// Neither topmost node of liquid column nor topmost node below column
				waschecked = false;
				waspushed = false;
			} else if (isliquid) {
				// This is the topmost node in the column
				bool ispushed = false;
				if (isLiquidHorizontallyFlowable(vi, em)) {
					trans_liquid->push_back(v3s16(x, y, z));
					ispushed = true;
				}
				// Remember waschecked and waspushed to avoid repeated
				// checks/pushes in case the column consists of only this node
				waschecked = true;
				waspushed = ispushed;
			} else {
				// This is the topmost node below a liquid column
				u32 vi_above = vi;
				vm->m_area.add_y(em, vi_above, 1);
				if (!waspushed && (ndef->get(vm->m_data[vi]).floodable ||
						(!waschecked && isLiquidHorizontallyFlowable(vi_above, em)))) {
					// Push back the lowest node in the column which is one
					// node above this one
					trans_liquid->push_back(v3s16(x, y + 1, z));
				}
			}

			wasliquid = isliquid;
			wasignored = isignored;
			vm->m_area.add_y(em, vi, -1);
		}
	}
}


void Mapgen::setLighting(u8 light, v3s16 nmin, v3s16 nmax)
{
	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	VoxelArea a(nmin, nmax);

	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int y = a.MinEdge.Y; y <= a.MaxEdge.Y; y++) {
			u32 i = vm->m_area.index(a.MinEdge.X, y, z);
			for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++, i++)
				vm->m_data[i].param1 = light;
		}
	}
}


void Mapgen::lightSpread(VoxelArea &a, v3s16 p, u8 light)
{
	if (light <= 1 || !a.contains(p))
		return;

	u32 vi = vm->m_area.index(p);
	MapNode &n = vm->m_data[vi];

	// Decay light in each of the banks separately
	u8 light_day = light & 0x0F;
	if (light_day > 0)
		light_day -= 0x01;

	u8 light_night = light & 0xF0;
	if (light_night > 0)
		light_night -= 0x10;

	// Bail out only if we have no more light from either bank to propogate, or
	// we hit a solid block that light cannot pass through.
	if ((light_day  <= (n.param1 & 0x0F) &&
		light_night <= (n.param1 & 0xF0)) ||
		!ndef->get(n).light_propagates)
		return;

	// Since this recursive function only terminates when there is no light from
	// either bank left, we need to take the max of both banks into account for
	// the case where spreading has stopped for one light bank but not the other.
	light = MYMAX(light_day, n.param1 & 0x0F) |
			MYMAX(light_night, n.param1 & 0xF0);

	n.param1 = light;

	lightSpread(a, p + v3s16(0, 0, 1), light);
	lightSpread(a, p + v3s16(0, 1, 0), light);
	lightSpread(a, p + v3s16(1, 0, 0), light);
	lightSpread(a, p - v3s16(0, 0, 1), light);
	lightSpread(a, p - v3s16(0, 1, 0), light);
	lightSpread(a, p - v3s16(1, 0, 0), light);
}


void Mapgen::calcLighting(v3s16 nmin, v3s16 nmax, v3s16 full_nmin, v3s16 full_nmax,
	bool propagate_shadow)
{
	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update", SPT_AVG);
	//TimeTaker t("updateLighting");

	propagateSunlight(nmin, nmax, propagate_shadow);
	spreadLight(full_nmin, full_nmax);

	//printf("updateLighting: %dms\n", t.stop());
}


void Mapgen::propagateSunlight(v3s16 nmin, v3s16 nmax, bool propagate_shadow)
{
	//TimeTaker t("propagateSunlight");
	VoxelArea a(nmin, nmax);
	bool block_is_underground = (water_level >= nmax.Y);
	v3s16 em = vm->m_area.getExtent();

	// NOTE: Direct access to the low 4 bits of param1 is okay here because,
	// by definition, sunlight will never be in the night lightbank.

	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++) {
			// see if we can get a light value from the overtop
			u32 i = vm->m_area.index(x, a.MaxEdge.Y + 1, z);
			if (vm->m_data[i].getContent() == CONTENT_IGNORE) {
				if (block_is_underground)
					continue;
			} else if ((vm->m_data[i].param1 & 0x0F) != LIGHT_SUN &&
					propagate_shadow) {
				continue;
			}
			vm->m_area.add_y(em, i, -1);

			for (int y = a.MaxEdge.Y; y >= a.MinEdge.Y; y--) {
				MapNode &n = vm->m_data[i];
				if (!ndef->get(n).sunlight_propagates)
					break;
				n.param1 = LIGHT_SUN;
				vm->m_area.add_y(em, i, -1);
			}
		}
	}
	//printf("propagateSunlight: %dms\n", t.stop());
}


void Mapgen::spreadLight(v3s16 nmin, v3s16 nmax)
{
	//TimeTaker t("spreadLight");
	VoxelArea a(nmin, nmax);

	for (int z = a.MinEdge.Z; z <= a.MaxEdge.Z; z++) {
		for (int y = a.MinEdge.Y; y <= a.MaxEdge.Y; y++) {
			u32 i = vm->m_area.index(a.MinEdge.X, y, z);
			for (int x = a.MinEdge.X; x <= a.MaxEdge.X; x++, i++) {
				MapNode &n = vm->m_data[i];
				if (n.getContent() == CONTENT_IGNORE)
					continue;

				const ContentFeatures &cf = ndef->get(n);
				if (!cf.light_propagates)
					continue;

				// TODO(hmmmmm): Abstract away direct param1 accesses with a
				// wrapper, but something lighter than MapNode::get/setLight

				u8 light_produced = cf.light_source;
				if (light_produced)
					n.param1 = light_produced | (light_produced << 4);

				u8 light = n.param1;
				if (light) {
					lightSpread(a, v3s16(x,     y,     z + 1), light);
					lightSpread(a, v3s16(x,     y + 1, z    ), light);
					lightSpread(a, v3s16(x + 1, y,     z    ), light);
					lightSpread(a, v3s16(x,     y,     z - 1), light);
					lightSpread(a, v3s16(x,     y - 1, z    ), light);
					lightSpread(a, v3s16(x - 1, y,     z    ), light);
				}
			}
		}
	}

	//printf("spreadLight: %dms\n", t.stop());
}


////
//// MapgenBasic
////

MapgenBasic::MapgenBasic(int mapgenid, MapgenParams *params, EmergeManager *emerge)
	: Mapgen(mapgenid, params, emerge)
{
	this->m_emerge = emerge;
	this->m_bmgr   = emerge->biomemgr;

	//// Here, 'stride' refers to the number of elements needed to skip to index
	//// an adjacent element for that coordinate in noise/height/biome maps
	//// (*not* vmanip content map!)

	// Note there is no X stride explicitly defined.  Items adjacent in the X
	// coordinate are assumed to be adjacent in memory as well (i.e. stride of 1).

	// Number of elements to skip to get to the next Y coordinate
	this->ystride = csize.X;

	// Number of elements to skip to get to the next Z coordinate
	this->zstride = csize.X * csize.Y;

	// Z-stride value for maps oversized for 1-down overgeneration
	this->zstride_1d = csize.X * (csize.Y + 1);

	// Z-stride value for maps oversized for 1-up 1-down overgeneration
	this->zstride_1u1d = csize.X * (csize.Y + 2);

	//// Allocate heightmap
	this->heightmap = new s16[csize.X * csize.Z];

	//// Initialize biome generator
	// TODO(hmmmm): should we have a way to disable biomemanager biomes?
	biomegen = m_bmgr->createBiomeGen(BIOMEGEN_ORIGINAL, params->bparams, csize);
	biomemap = biomegen->biomemap;

	//// Look up some commonly used content
	c_stone              = ndef->getId("mapgen_stone");
	c_desert_stone       = ndef->getId("mapgen_desert_stone");
	c_sandstone          = ndef->getId("mapgen_sandstone");
	c_water_source       = ndef->getId("mapgen_water_source");
	c_river_water_source = ndef->getId("mapgen_river_water_source");
	c_lava_source        = ndef->getId("mapgen_lava_source");

	// Fall back to more basic content if not defined
	// river_water_source cannot fallback to water_source because river water
	// needs to be non-renewable and have a short flow range.
	if (c_desert_stone == CONTENT_IGNORE)
		c_desert_stone = c_stone;
	if (c_sandstone == CONTENT_IGNORE)
		c_sandstone = c_stone;

	//// Content used for dungeon generation
	c_cobble                = ndef->getId("mapgen_cobble");
	c_mossycobble           = ndef->getId("mapgen_mossycobble");
	c_stair_cobble          = ndef->getId("mapgen_stair_cobble");
	c_stair_desert_stone    = ndef->getId("mapgen_stair_desert_stone");
	c_sandstonebrick        = ndef->getId("mapgen_sandstonebrick");
	c_stair_sandstone_block = ndef->getId("mapgen_stair_sandstone_block");

	// Fall back to more basic content if not defined
	if (c_mossycobble == CONTENT_IGNORE)
		c_mossycobble = c_cobble;
	if (c_stair_cobble == CONTENT_IGNORE)
		c_stair_cobble = c_cobble;
	if (c_stair_desert_stone == CONTENT_IGNORE)
		c_stair_desert_stone = c_desert_stone;
	if (c_sandstonebrick == CONTENT_IGNORE)
		c_sandstonebrick = c_sandstone;
	if (c_stair_sandstone_block == CONTENT_IGNORE)
		c_stair_sandstone_block = c_sandstonebrick;
}


MapgenBasic::~MapgenBasic()
{
	delete biomegen;
	delete []heightmap;
}


MgStoneType MapgenBasic::generateBiomes()
{
	// can't generate biomes without a biome generator!
	assert(biomegen);
	assert(biomemap);

	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;
	MgStoneType stone_type = MGSTONE_STONE;

	noise_filler_depth->perlinMap2D(node_min.X, node_min.Z);

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome = NULL;
		u16 depth_top = 0;
		u16 base_filler = 0;
		u16 depth_water_top = 0;
		u16 depth_riverbed = 0;
		u32 vi = vm->m_area.index(x, node_max.Y, z);

		// Check node at base of mapchunk above, either a node of a previously
		// generated mapchunk or if not, a node of overgenerated base terrain.
		content_t c_above = vm->m_data[vi + em.X].getContent();
		bool air_above = c_above == CONTENT_AIR;
		bool river_water_above = c_above == c_river_water_source;
		bool water_above = c_above == c_water_source || river_water_above;

		biomemap[index] = BIOME_NONE;

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
			bool is_stone_surface = (c == c_stone) &&
				(air_above || water_above || !biome);

			bool is_water_surface =
				(c == c_water_source || c == c_river_water_source) &&
				(air_above || !biome);

			if (is_stone_surface || is_water_surface) {
				biome = biomegen->getBiomeAtIndex(index, y);

				if (biomemap[index] == BIOME_NONE && is_stone_surface)
					biomemap[index] = biome->index;

				depth_top = biome->depth_top;
				base_filler = MYMAX(depth_top +
					biome->depth_filler +
					noise_filler_depth->result[index], 0.f);
				depth_water_top = biome->depth_water_top;
				depth_riverbed = biome->depth_riverbed;

				// Detect stone type for dungeons during every biome calculation.
				// This is more efficient than detecting per-node and will not
				// miss any desert stone or sandstone biomes.
				if (biome->c_stone == c_desert_stone)
					stone_type = MGSTONE_DESERT_STONE;
				else if (biome->c_stone == c_sandstone)
					stone_type = MGSTONE_SANDSTONE;
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

				if (river_water_above) {
					if (nplaced < depth_riverbed) {
						vm->m_data[vi] = MapNode(biome->c_riverbed);
						nplaced++;
					} else {
						nplaced = U16_MAX;  // Disable top/filler placement
						river_water_above = false;
					}
				} else if (nplaced < depth_top) {
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
				nplaced = 0;  // Enable riverbed placement for next surface
				air_above = false;
				water_above = true;
				river_water_above = true;
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


void MapgenBasic::dustTopNodes()
{
	if (node_max.Y < water_level)
		return;

	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;

	for (s16 z = node_min.Z; z <= node_max.Z; z++)
	for (s16 x = node_min.X; x <= node_max.X; x++, index++) {
		Biome *biome = (Biome *)m_bmgr->getRaw(biomemap[index]);

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
		NodeDrawType dtype = ndef->get(c).drawtype;
		// Only place on walkable cubic non-liquid nodes
		// Dust check needed due to vertical overgeneration
		if ((dtype == NDT_NORMAL ||
				dtype == NDT_ALLFACES_OPTIONAL ||
				dtype == NDT_GLASSLIKE_FRAMED_OPTIONAL ||
				dtype == NDT_GLASSLIKE ||
				dtype == NDT_GLASSLIKE_FRAMED ||
				dtype == NDT_ALLFACES) &&
				ndef->get(c).walkable && c != biome->c_dust) {
			vm->m_area.add_y(em, vi, 1);
			vm->m_data[vi] = MapNode(biome->c_dust);
		}
	}
}


void MapgenBasic::generateCaves(s16 max_stone_y, s16 large_cave_depth)
{
	if (max_stone_y < node_min.Y)
		return;

	CavesNoiseIntersection caves_noise(ndef, m_bmgr, csize,
		&np_cave1, &np_cave2, seed, cave_width);

	caves_noise.generateCaves(vm, node_min, node_max, biomemap);

	if (node_max.Y > large_cave_depth)
		return;

	PseudoRandom ps(blockseed + 21343);
	u32 bruises_count = ps.range(0, 2);
	for (u32 i = 0; i < bruises_count; i++) {
		CavesRandomWalk cave(ndef, &gennotify, seed, water_level,
			c_water_source, CONTENT_IGNORE);

		cave.makeCave(vm, node_min, node_max, &ps, true, max_stone_y, heightmap);
	}
}


bool MapgenBasic::generateCaverns(s16 max_stone_y)
{
	if (node_min.Y > max_stone_y || node_min.Y > cavern_limit)
		return false;

	CavernsNoise caverns_noise(ndef, csize, &np_cavern,
		seed, cavern_limit, cavern_taper, cavern_threshold);

	return caverns_noise.generateCaverns(vm, node_min, node_max);
}


void MapgenBasic::generateDungeons(s16 max_stone_y, MgStoneType stone_type)
{
	if (max_stone_y < node_min.Y)
		return;

	DungeonParams dp;

	dp.seed             = seed;
	dp.c_water          = c_water_source;
	dp.c_river_water    = c_river_water_source;

	dp.only_in_ground   = true;
	dp.corridor_len_min = 1;
	dp.corridor_len_max = 13;
	dp.rooms_min        = 2;
	dp.rooms_max        = 16;
	dp.y_min            = -MAX_MAP_GENERATION_LIMIT;
	dp.y_max            = MAX_MAP_GENERATION_LIMIT;

	dp.np_density       = nparams_dungeon_density;
	dp.np_alt_wall      = nparams_dungeon_alt_wall;

	switch (stone_type) {
	default:
	case MGSTONE_STONE:
		dp.c_wall              = c_cobble;
		dp.c_alt_wall          = c_mossycobble;
		dp.c_stair             = c_stair_cobble;

		dp.diagonal_dirs       = false;
		dp.holesize            = v3s16(1, 2, 1);
		dp.room_size_min       = v3s16(4, 4, 4);
		dp.room_size_max       = v3s16(8, 6, 8);
		dp.room_size_large_min = v3s16(8, 8, 8);
		dp.room_size_large_max = v3s16(16, 16, 16);
		dp.notifytype          = GENNOTIFY_DUNGEON;
		break;
	case MGSTONE_DESERT_STONE:
		dp.c_wall              = c_desert_stone;
		dp.c_alt_wall          = CONTENT_IGNORE;
		dp.c_stair             = c_stair_desert_stone;

		dp.diagonal_dirs       = true;
		dp.holesize            = v3s16(2, 3, 2);
		dp.room_size_min       = v3s16(6, 9, 6);
		dp.room_size_max       = v3s16(10, 11, 10);
		dp.room_size_large_min = v3s16(10, 13, 10);
		dp.room_size_large_max = v3s16(18, 21, 18);
		dp.notifytype          = GENNOTIFY_TEMPLE;
		break;
	case MGSTONE_SANDSTONE:
		dp.c_wall              = c_sandstonebrick;
		dp.c_alt_wall          = CONTENT_IGNORE;
		dp.c_stair             = c_stair_sandstone_block;

		dp.diagonal_dirs       = false;
		dp.holesize            = v3s16(2, 2, 2);
		dp.room_size_min       = v3s16(6, 4, 6);
		dp.room_size_max       = v3s16(10, 6, 10);
		dp.room_size_large_min = v3s16(10, 8, 10);
		dp.room_size_large_max = v3s16(18, 16, 18);
		dp.notifytype          = GENNOTIFY_DUNGEON;
		break;
	}

	DungeonGen dgen(ndef, &gennotify, &dp);
	dgen.generate(vm, blockseed, full_node_min, full_node_max);
}


////
//// GenerateNotifier
////

GenerateNotifier::GenerateNotifier()
{
	m_notify_on = 0;
}


GenerateNotifier::GenerateNotifier(u32 notify_on,
	std::set<u32> *notify_on_deco_ids)
{
	m_notify_on = notify_on;
	m_notify_on_deco_ids = notify_on_deco_ids;
}


void GenerateNotifier::setNotifyOn(u32 notify_on)
{
	m_notify_on = notify_on;
}


void GenerateNotifier::setNotifyOnDecoIds(std::set<u32> *notify_on_deco_ids)
{
	m_notify_on_deco_ids = notify_on_deco_ids;
}


bool GenerateNotifier::addEvent(GenNotifyType type, v3s16 pos, u32 id)
{
	if (!(m_notify_on & (1 << type)))
		return false;

	if (type == GENNOTIFY_DECORATION &&
		m_notify_on_deco_ids->find(id) == m_notify_on_deco_ids->end())
		return false;

	GenNotifyEvent gne;
	gne.type = type;
	gne.pos  = pos;
	gne.id   = id;
	m_notify_events.push_back(gne);

	return true;
}


void GenerateNotifier::getEvents(
	std::map<std::string, std::vector<v3s16> > &event_map)
{
	std::list<GenNotifyEvent>::iterator it;

	for (it = m_notify_events.begin(); it != m_notify_events.end(); ++it) {
		GenNotifyEvent &gn = *it;
		std::string name = (gn.type == GENNOTIFY_DECORATION) ?
			"decoration#"+ itos(gn.id) :
			flagdesc_gennotify[gn.type].name;

		event_map[name].push_back(gn.pos);
	}
}


void GenerateNotifier::clearEvents()
{
	m_notify_events.clear();
}


////
//// MapgenParams
////


MapgenParams::~MapgenParams()
{
	delete bparams;
}


void MapgenParams::readParams(const Settings *settings)
{
	std::string seed_str;
	const char *seed_name = (settings == g_settings) ? "fixed_map_seed" : "seed";

	if (settings->getNoEx(seed_name, seed_str)) {
		if (!seed_str.empty())
			seed = read_seed(seed_str.c_str());
		else
			myrand_bytes(&seed, sizeof(seed));
	}

	std::string mg_name;
	if (settings->getNoEx("mg_name", mg_name)) {
		mgtype = Mapgen::getMapgenType(mg_name);
		if (mgtype == MAPGEN_INVALID)
			mgtype = MAPGEN_DEFAULT;
	}

	settings->getS16NoEx("water_level", water_level);
	settings->getS16NoEx("mapgen_limit", mapgen_limit);
	settings->getS16NoEx("chunksize", chunksize);
	settings->getFlagStrNoEx("mg_flags", flags, flagdesc_mapgen);

	delete bparams;
	bparams = BiomeManager::createBiomeParams(BIOMEGEN_ORIGINAL);
	if (bparams) {
		bparams->readParams(settings);
		bparams->seed = seed;
	}
}


void MapgenParams::writeParams(Settings *settings) const
{
	settings->set("mg_name", Mapgen::getMapgenName(mgtype));
	settings->setU64("seed", seed);
	settings->setS16("water_level", water_level);
	settings->setS16("mapgen_limit", mapgen_limit);
	settings->setS16("chunksize", chunksize);
	settings->setFlagStr("mg_flags", flags, flagdesc_mapgen, U32_MAX);

	if (bparams)
		bparams->writeParams(settings);
}


// Calculate exact edges of the outermost mapchunks that are within the
// set 'mapgen_limit'.
void MapgenParams::calcMapgenEdges()
{
	// Central chunk offset, in blocks
	s16 ccoff_b = -chunksize / 2;
	// Chunksize, in nodes
	s32 csize_n = chunksize * MAP_BLOCKSIZE;
	// Minp/maxp of central chunk, in nodes
	s16 ccmin = ccoff_b * MAP_BLOCKSIZE;
	s16 ccmax = ccmin + csize_n - 1;
	// Fullminp/fullmaxp of central chunk, in nodes
	s16 ccfmin = ccmin - MAP_BLOCKSIZE;
	s16 ccfmax = ccmax + MAP_BLOCKSIZE;
	// Effective mapgen limit, in blocks
	// Uses same calculation as ServerMap::blockpos_over_mapgen_limit(v3s16 p)
	s16 mapgen_limit_b = rangelim(mapgen_limit,
		0, MAX_MAP_GENERATION_LIMIT) / MAP_BLOCKSIZE;
	// Effective mapgen limits, in nodes
	s16 mapgen_limit_min = -mapgen_limit_b * MAP_BLOCKSIZE;
	s16 mapgen_limit_max = (mapgen_limit_b + 1) * MAP_BLOCKSIZE - 1;
	// Number of complete chunks from central chunk fullminp/fullmaxp
	// to effective mapgen limits.
	s16 numcmin = MYMAX((ccfmin - mapgen_limit_min) / csize_n, 0);
	s16 numcmax = MYMAX((mapgen_limit_max - ccfmax) / csize_n, 0);
	// Mapgen edges, in nodes
	mapgen_edge_min = ccmin - numcmin * csize_n;
	mapgen_edge_max = ccmax + numcmax * csize_n;

	m_mapgen_edges_calculated = true;
}


s32 MapgenParams::getSpawnRangeMax()
{
	if (!m_mapgen_edges_calculated)
		calcMapgenEdges();

	return MYMIN(-mapgen_edge_min, mapgen_edge_max);
}
