/*
Minetest
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "cavegen.h"

FlagDesc flagdesc_mapgen[] = {
	{"trees",       MG_TREES},
	{"caves",       MG_CAVES},
	{"dungeons",    MG_DUNGEONS},
	{"flat",        MG_FLAT},
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


////
//// Mapgen
////

Mapgen::Mapgen()
{
	generating  = false;
	id          = -1;
	seed        = 0;
	water_level = 0;
	flags       = 0;

	vm        = NULL;
	ndef      = NULL;
	biomegen  = NULL;
	biomemap  = NULL;
	heightmap = NULL;
}


Mapgen::Mapgen(int mapgenid, MapgenParams *params, EmergeManager *emerge) :
	gennotify(emerge->gen_notify_on, &emerge->gen_notify_on_deco_ids)
{
	generating  = false;
	id          = mapgenid;
	seed        = (int)params->seed;
	water_level = params->water_level;
	flags       = params->flags;
	csize       = v3s16(1, 1, 1) * (params->chunksize * MAP_BLOCKSIZE);

	vm        = NULL;
	ndef      = emerge->ndef;
	biomegen  = NULL;
	biomemap  = NULL;
	heightmap = NULL;
}


Mapgen::~Mapgen()
{
}


u32 Mapgen::getBlockSeed(v3s16 p, int seed)
{
	return (u32)seed   +
		p.Z * 38134234 +
		p.Y * 42123    +
		p.X * 23;
}


u32 Mapgen::getBlockSeed2(v3s16 p, int seed)
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
	//printf("updateHeightmap: %dus\n", t.stop());
}


void Mapgen::updateLiquid(UniqueQueue<v3s16> *trans_liquid, v3s16 nmin, v3s16 nmax)
{
	bool isliquid, wasliquid;
	v3s16 em  = vm->m_area.getExtent();

	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 x = nmin.X; x <= nmax.X; x++) {
			wasliquid = true;

			u32 i = vm->m_area.index(x, nmax.Y, z);
			for (s16 y = nmax.Y; y >= nmin.Y; y--) {
				isliquid = ndef->get(vm->m_data[i]).isLiquid();

				// there was a change between liquid and nonliquid, add to queue.
				if (isliquid != wasliquid)
					trans_liquid->push_back(v3s16(x, y, z));

				wasliquid = isliquid;
				vm->m_area.add_y(em, i, -1);
			}
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

}

MgStoneType MapgenBasic::generateBiomes()
{
	v3s16 em = vm->m_area.getExtent();
	u32 index = 0;
	MgStoneType stone_type = MGSTONE_STONE;

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
		bool water_above = (c_above == c_water_source || c_above == c_river_water_source);

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
				biome = biomegen->getBiomeAtIndex(index, y);

				depth_top = biome->depth_top;
				base_filler = MYMAX(depth_top
						+ biome->depth_filler
						+ noise_filler_depth->result[index], 0.f);
				depth_water_top = biome->depth_water_top;

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
				nplaced = depth_top;  // Enable filler placement for next surface
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


void MapgenBasic::dustTopNodes()
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


void MapgenBasic::generateCaves(s16 max_stone_y, s16 large_cave_depth)
{
	if (max_stone_y < node_min.Y)
		return;

	CavesNoiseIntersection caves_noise(ndef, bmgr, csize,
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
	std::map<std::string, std::vector<v3s16> > &event_map,
	bool peek_events)
{
	std::list<GenNotifyEvent>::iterator it;

	for (it = m_notify_events.begin(); it != m_notify_events.end(); ++it) {
		GenNotifyEvent &gn = *it;
		std::string name = (gn.type == GENNOTIFY_DECORATION) ?
			"decoration#"+ itos(gn.id) :
			flagdesc_gennotify[gn.type].name;

		event_map[name].push_back(gn.pos);
	}

	if (!peek_events)
		m_notify_events.clear();
}


////
//// MapgenParams
////


MapgenParams::~MapgenParams()
{
	delete bparams;
	delete sparams;
}


void MapgenParams::load(const Settings &settings)
{
	std::string seed_str;
	const char *seed_name = (&settings == g_settings) ? "fixed_map_seed" : "seed";

	if (settings.getNoEx(seed_name, seed_str) && !seed_str.empty())
		seed = read_seed(seed_str.c_str());
	else
		myrand_bytes(&seed, sizeof(seed));

	settings.getNoEx("mg_name", mg_name);
	settings.getS16NoEx("water_level", water_level);
	settings.getS16NoEx("chunksize", chunksize);
	settings.getFlagStrNoEx("mg_flags", flags, flagdesc_mapgen);

	delete bparams;
	bparams = BiomeManager::createBiomeParams(BIOMEGEN_ORIGINAL);
	if (bparams) {
		bparams->readParams(&settings);
		bparams->seed = seed;
	}

	delete sparams;
	MapgenFactory *mgfactory = EmergeManager::getMapgenFactory(mg_name);
	if (mgfactory) {
		sparams = mgfactory->createMapgenParams();
		sparams->readParams(&settings);
	}
}


void MapgenParams::save(Settings &settings) const
{
	settings.set("mg_name", mg_name);
	settings.setU64("seed", seed);
	settings.setS16("water_level", water_level);
	settings.setS16("chunksize", chunksize);
	settings.setFlagStr("mg_flags", flags, flagdesc_mapgen, U32_MAX);

	if (bparams)
		bparams->writeParams(&settings);

	if (sparams)
		sparams->writeParams(&settings);
}
