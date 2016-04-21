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

#include "mg_biome.h"
#include "mg_decoration.h"
#include "emerge.h"
#include "gamedef.h"
#include "nodedef.h"
#include "map.h" //for MMVManip
#include "log.h"
#include "util/numeric.h"
#include "util/mathconstants.h"
#include "porting.h"


///////////////////////////////////////////////////////////////////////////////


BiomeManager::BiomeManager(IGameDef *gamedef) :
	ObjDefManager(gamedef, OBJDEF_BIOME)
{
	m_gamedef = gamedef;

	// Create default biome to be used in case none exist
	Biome *b = new Biome;

	b->name            = "Default";
	b->flags           = 0;
	b->depth_top       = 0;
	b->depth_filler    = -MAX_MAP_GENERATION_LIMIT;
	b->depth_water_top = 0;
	b->y_min           = -MAX_MAP_GENERATION_LIMIT;
	b->y_max           = MAX_MAP_GENERATION_LIMIT;
	b->heat_point      = 0.0;
	b->humidity_point  = 0.0;

	b->m_nodenames.push_back("mapgen_stone");
	b->m_nodenames.push_back("mapgen_stone");
	b->m_nodenames.push_back("mapgen_stone");
	b->m_nodenames.push_back("mapgen_water_source");
	b->m_nodenames.push_back("mapgen_water_source");
	b->m_nodenames.push_back("mapgen_river_water_source");
	b->m_nodenames.push_back("ignore");
	m_ndef->pendNodeResolve(b);

	add(b);
}



BiomeManager::~BiomeManager()
{
	//if (biomecache)
	//	delete[] biomecache;
}



// just a PoC, obviously needs optimization later on (precalculate this)
void BiomeManager::calcBiomes(s16 sx, s16 sy, float *heat_map,
	float *humidity_map, s16 *height_map, u8 *biomeid_map)
{
	for (s32 i = 0; i != sx * sy; i++) {
		Biome *biome = getBiome(heat_map[i], humidity_map[i], height_map[i]);
		biomeid_map[i] = biome->index;
	}
}


Biome *BiomeManager::getBiome(float heat, float humidity, s16 y)
{
	Biome *b, *biome_closest = NULL;
	float dist_min = FLT_MAX;

	for (size_t i = 1; i < m_objects.size(); i++) {
		b = (Biome *)m_objects[i];
		if (!b || y > b->y_max || y < b->y_min)
			continue;

		float d_heat     = heat     - b->heat_point;
		float d_humidity = humidity - b->humidity_point;
		float dist = (d_heat * d_heat) +
					 (d_humidity * d_humidity);
		if (dist < dist_min) {
			dist_min = dist;
			biome_closest = b;
		}
	}

	return biome_closest ? biome_closest : (Biome *)m_objects[0];
}

void BiomeManager::clear()
{
	EmergeManager *emerge = m_gamedef->getEmergeManager();

	// Remove all dangling references in Decorations
	DecorationManager *decomgr = emerge->decomgr;
	for (size_t i = 0; i != decomgr->getNumObjects(); i++) {
		Decoration *deco = (Decoration *)decomgr->getRaw(i);
		deco->biomes.clear();
	}

	// Don't delete the first biome
	for (size_t i = 1; i < m_objects.size(); i++) {
		Biome *b = (Biome *)m_objects[i];
		delete b;
	}

	m_objects.resize(1);
}


///////////////////////////////////////////////////////////////////////////////


void Biome::resolveNodeNames()
{
	getIdFromNrBacklog(&c_top,         "mapgen_stone",              CONTENT_AIR);
	getIdFromNrBacklog(&c_filler,      "mapgen_stone",              CONTENT_AIR);
	getIdFromNrBacklog(&c_stone,       "mapgen_stone",              CONTENT_AIR);
	getIdFromNrBacklog(&c_water_top,   "mapgen_water_source",       CONTENT_AIR);
	getIdFromNrBacklog(&c_water,       "mapgen_water_source",       CONTENT_AIR);
	getIdFromNrBacklog(&c_river_water, "mapgen_river_water_source", CONTENT_AIR);
	getIdFromNrBacklog(&c_dust,        "ignore",                    CONTENT_IGNORE);
}
