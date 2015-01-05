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
#include "gamedef.h"
#include "nodedef.h"
#include "map.h" //for MMVManip
#include "log.h"
#include "util/numeric.h"
#include "main.h"
#include "util/mathconstants.h"
#include "porting.h"

const char *BiomeManager::ELEMENT_TITLE = "biome";


///////////////////////////////////////////////////////////////////////////////

BiomeManager::BiomeManager(IGameDef *gamedef) :
	GenElementManager(gamedef)
{
	// Create default biome to be used in case none exist
	Biome *b = new Biome;

	b->id              = 0;
	b->name            = "Default";
	b->flags           = 0;
	b->depth_top       = 0;
	b->depth_filler    = 0;
	b->height_shore    = 0;
	b->depth_water_top = 0;
	b->y_min           = -MAP_GENERATION_LIMIT;
	b->y_max           = MAP_GENERATION_LIMIT;
	b->heat_point      = 0.0;
	b->humidity_point  = 0.0;

	NodeResolveInfo *nri = new NodeResolveInfo(b);
	nri->nodenames.push_back("air");
	nri->nodenames.push_back("air");
	nri->nodenames.push_back("air");
	nri->nodenames.push_back("air");
	nri->nodenames.push_back("air");
	nri->nodenames.push_back("mapgen_stone");
	nri->nodenames.push_back("mapgen_water_source");
	nri->nodenames.push_back("mapgen_water_source");
	nri->nodenames.push_back("air");
	m_ndef->pendNodeResolve(nri);

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
	for (s32 i = 0; i != sx * sy; i++)
		biomeid_map[i] = getBiome(heat_map[i], humidity_map[i], height_map[i])->id;
}


Biome *BiomeManager::getBiome(float heat, float humidity, s16 y)
{
	Biome *b, *biome_closest = NULL;
	float dist_min = FLT_MAX;

	for (size_t i = 1; i < m_elements.size(); i++) {
		b = (Biome *)m_elements[i];
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

	return biome_closest ? biome_closest : (Biome *)m_elements[0];
}

void BiomeManager::clear()
{

	for (size_t i = 1; i < m_elements.size(); i++) {
		Biome *b = (Biome *)m_elements[i];
		delete b;
	}

	m_elements.resize(1);
}


///////////////////////////////////////////////////////////////////////////////


void Biome::resolveNodeNames(NodeResolveInfo *nri)
{
	m_ndef->getIdFromResolveInfo(nri, "mapgen_dirt_with_grass", CONTENT_AIR,    c_top);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_dirt",            CONTENT_AIR,    c_filler);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_sand",            CONTENT_AIR,    c_shore_top);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_sand",            CONTENT_AIR,    c_shore_filler);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_sand",            CONTENT_AIR,    c_underwater);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_stone",           CONTENT_AIR,    c_stone);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_water_source",    CONTENT_AIR,    c_water_top);
	m_ndef->getIdFromResolveInfo(nri, "mapgen_water_source",    CONTENT_AIR,    c_water);
	m_ndef->getIdFromResolveInfo(nri, "air",                    CONTENT_IGNORE, c_dust);
}

