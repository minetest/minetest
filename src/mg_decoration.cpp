/*
Minetest
Copyright (C) 2010-2014 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include "mg_decoration.h"
#include "mg_schematic.h"
#include "mapgen.h"
#include "noise.h"
#include "map.h"
#include "log.h"
#include "util/numeric.h"

FlagDesc flagdesc_deco[] = {
	{"place_center_x", DECO_PLACE_CENTER_X},
	{"place_center_y", DECO_PLACE_CENTER_Y},
	{"place_center_z", DECO_PLACE_CENTER_Z},
	{"force_placement", DECO_FORCE_PLACEMENT},
	{"liquid_surface", DECO_LIQUID_SURFACE},
	{NULL,             0}
};


///////////////////////////////////////////////////////////////////////////////


DecorationManager::DecorationManager(IGameDef *gamedef) :
	ObjDefManager(gamedef, OBJDEF_DECORATION)
{
}


size_t DecorationManager::placeAllDecos(Mapgen *mg, u32 blockseed,
	v3s16 nmin, v3s16 nmax)
{
	size_t nplaced = 0;

	for (size_t i = 0; i != m_objects.size(); i++) {
		Decoration *deco = (Decoration *)m_objects[i];
		if (!deco)
			continue;

		nplaced += deco->placeDeco(mg, blockseed, nmin, nmax);
		blockseed++;
	}

	return nplaced;
}


///////////////////////////////////////////////////////////////////////////////


Decoration::Decoration()
{
	mapseed    = 0;
	fill_ratio = 0;
	sidelen    = 1;
	flags      = 0;
}


Decoration::~Decoration()
{
}


void Decoration::resolveNodeNames()
{
	getIdsFromNrBacklog(&c_place_on);
}


size_t Decoration::placeDeco(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax)
{
	PcgRandom ps(blockseed + 53);
	int carea_size = nmax.X - nmin.X + 1;

	// Divide area into parts
	// If chunksize is changed it may no longer be divisable by sidelen
	if (carea_size % sidelen)
		sidelen = carea_size;

	s16 divlen = carea_size / sidelen;
	int area = sidelen * sidelen;

	for (s16 z0 = 0; z0 < divlen; z0++)
	for (s16 x0 = 0; x0 < divlen; x0++) {
		v2s16 p2d_center( // Center position of part of division
			nmin.X + sidelen / 2 + sidelen * x0,
			nmin.Z + sidelen / 2 + sidelen * z0
		);
		v2s16 p2d_min( // Minimum edge of part of division
			nmin.X + sidelen * x0,
			nmin.Z + sidelen * z0
		);
		v2s16 p2d_max( // Maximum edge of part of division
			nmin.X + sidelen + sidelen * x0 - 1,
			nmin.Z + sidelen + sidelen * z0 - 1
		);

		// Amount of decorations
		float nval = (flags & DECO_USE_NOISE) ?
			NoisePerlin2D(&np, p2d_center.X, p2d_center.Y, mapseed) :
			fill_ratio;
		u32 deco_count = 0;
		float deco_count_f = (float)area * nval;
		if (deco_count_f >= 1.f) {
			deco_count = deco_count_f;
		} else if (deco_count_f > 0.f) {
			// For low density decorations calculate a chance for 1 decoration
			if (ps.range(1000) <= deco_count_f * 1000.f)
				deco_count = 1;
		}

		for (u32 i = 0; i < deco_count; i++) {
			s16 x = ps.range(p2d_min.X, p2d_max.X);
			s16 z = ps.range(p2d_min.Y, p2d_max.Y);

			int mapindex = carea_size * (z - nmin.Z) + (x - nmin.X);

			s16 y = -MAX_MAP_GENERATION_LIMIT;
			if (flags & DECO_LIQUID_SURFACE)
				y = mg->findLiquidSurface(v2s16(x, z), nmin.Y, nmax.Y);
			else if (mg->heightmap)
				y = mg->heightmap[mapindex];
			else
				y = mg->findGroundLevel(v2s16(x, z), nmin.Y, nmax.Y);

			if (y < nmin.Y || y > nmax.Y ||
				y < y_min  || y > y_max)
				continue;

			if (y + getHeight() >= mg->vm->m_area.MaxEdge.Y) {
				continue;
#if 0
				printf("Decoration at (%d %d %d) cut off\n", x, y, z);
				//add to queue
				MutexAutoLock cutofflock(cutoff_mutex);
				cutoffs.push_back(CutoffData(x, y, z, height));
#endif
			}

			if (mg->biomemap) {
				std::set<u8>::iterator iter;

				if (!biomes.empty()) {
					iter = biomes.find(mg->biomemap[mapindex]);
					if (iter == biomes.end())
						continue;
				}
			}

			v3s16 pos(x, y, z);
			if (generate(mg->vm, &ps, pos))
				mg->gennotify.addEvent(GENNOTIFY_DECORATION, pos, index);
		}
	}

	return 0;
}


#if 0
void Decoration::placeCutoffs(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax)
{
	PcgRandom pr(blockseed + 53);
	std::vector<CutoffData> handled_cutoffs;

	// Copy over the cutoffs we're interested in so we don't needlessly hold a lock
	{
		MutexAutoLock cutofflock(cutoff_mutex);
		for (std::list<CutoffData>::iterator i = cutoffs.begin();
			i != cutoffs.end(); ++i) {
			CutoffData cutoff = *i;
			v3s16 p    = cutoff.p;
			s16 height = cutoff.height;
			if (p.X < nmin.X || p.X > nmax.X ||
				p.Z < nmin.Z || p.Z > nmax.Z)
				continue;
			if (p.Y + height < nmin.Y || p.Y > nmax.Y)
				continue;

			handled_cutoffs.push_back(cutoff);
		}
	}

	// Generate the cutoffs
	for (size_t i = 0; i != handled_cutoffs.size(); i++) {
		v3s16 p    = handled_cutoffs[i].p;
		s16 height = handled_cutoffs[i].height;

		if (p.Y + height > nmax.Y) {
			//printf("Decoration at (%d %d %d) cut off again!\n", p.X, p.Y, p.Z);
			cuttoffs.push_back(v3s16(p.X, p.Y, p.Z));
		}

		generate(mg, &pr, nmax.Y, nmin.Y - p.Y, v3s16(p.X, nmin.Y, p.Z));
	}

	// Remove cutoffs that were handled from the cutoff list
	{
		MutexAutoLock cutofflock(cutoff_mutex);
		for (std::list<CutoffData>::iterator i = cutoffs.begin();
			i != cutoffs.end(); ++i) {

			for (size_t j = 0; j != handled_cutoffs.size(); j++) {
				CutoffData coff = *i;
				if (coff.p == handled_cutoffs[j].p)
					i = cutoffs.erase(i);
			}
		}
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////


void DecoSimple::resolveNodeNames()
{
	Decoration::resolveNodeNames();
	getIdsFromNrBacklog(&c_decos);
	getIdsFromNrBacklog(&c_spawnby);
}


bool DecoSimple::canPlaceDecoration(MMVManip *vm, v3s16 p)
{
	// Don't bother if there aren't any decorations to place
	if (c_decos.size() == 0)
		return false;

	u32 vi = vm->m_area.index(p);

	// Check if the decoration can be placed on this node
	if (!CONTAINS(c_place_on, vm->m_data[vi].getContent()))
		return false;

	// Don't continue if there are no spawnby constraints
	if (nspawnby == -1)
		return true;

	int nneighs = 0;
	v3s16 dirs[16] = {
		v3s16( 0, 0,  1),
		v3s16( 0, 0, -1),
		v3s16( 1, 0,  0),
		v3s16(-1, 0,  0),
		v3s16( 1, 0,  1),
		v3s16(-1, 0,  1),
		v3s16(-1, 0, -1),
		v3s16( 1, 0, -1),

		v3s16( 0, 1,  1),
		v3s16( 0, 1, -1),
		v3s16( 1, 1,  0),
		v3s16(-1, 1,  0),
		v3s16( 1, 1,  1),
		v3s16(-1, 1,  1),
		v3s16(-1, 1, -1),
		v3s16( 1, 1, -1)
	};

	// Check a Moore neighborhood if there are enough spawnby nodes
	for (size_t i = 0; i != ARRLEN(dirs); i++) {
		u32 index = vm->m_area.index(p + dirs[i]);
		if (!vm->m_area.contains(index))
			continue;

		if (CONTAINS(c_spawnby, vm->m_data[index].getContent()))
			nneighs++;
	}

	if (nneighs < nspawnby)
		return false;

	return true;
}


size_t DecoSimple::generate(MMVManip *vm, PcgRandom *pr, v3s16 p)
{
	if (!canPlaceDecoration(vm, p))
		return 0;

	content_t c_place = c_decos[pr->range(0, c_decos.size() - 1)];

	s16 height = (deco_height_max > 0) ?
		pr->range(deco_height, deco_height_max) : deco_height;

	bool force_placement = (flags & DECO_FORCE_PLACEMENT);

	v3s16 em = vm->m_area.getExtent();
	u32 vi = vm->m_area.index(p);
	for (int i = 0; i < height; i++) {
		vm->m_area.add_y(em, vi, 1);

		content_t c = vm->m_data[vi].getContent();
		if (c != CONTENT_AIR && c != CONTENT_IGNORE &&
				!force_placement)
			break;

		vm->m_data[vi] = MapNode(c_place);
	}

	return 1;
}


int DecoSimple::getHeight()
{
	return (deco_height_max > 0) ? deco_height_max : deco_height;
}


///////////////////////////////////////////////////////////////////////////////


DecoSchematic::DecoSchematic()
{
	schematic = NULL;
}


size_t DecoSchematic::generate(MMVManip *vm, PcgRandom *pr, v3s16 p)
{
	// Schematic could have been unloaded but not the decoration
	// In this case generate() does nothing (but doesn't *fail*)
	if (schematic == NULL)
		return 0;

	u32 vi = vm->m_area.index(p);
	content_t c = vm->m_data[vi].getContent();
	if (!CONTAINS(c_place_on, c))
		return 0;

	if (flags & DECO_PLACE_CENTER_X)
		p.X -= (schematic->size.X - 1) / 2;
	if (flags & DECO_PLACE_CENTER_Y)
		p.Y -= (schematic->size.Y - 1) / 2;
	if (flags & DECO_PLACE_CENTER_Z)
		p.Z -= (schematic->size.Z - 1) / 2;

	Rotation rot = (rotation == ROTATE_RAND) ?
		(Rotation)pr->range(ROTATE_0, ROTATE_270) : rotation;

	bool force_placement = (flags & DECO_FORCE_PLACEMENT);

	schematic->blitToVManip(vm, p, rot, force_placement);

	return 1;
}


int DecoSchematic::getHeight()
{
	return schematic->size.Y;
}
