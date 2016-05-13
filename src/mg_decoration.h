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

#ifndef MG_DECORATION_HEADER
#define MG_DECORATION_HEADER

#include <set>
#include "objdef.h"
#include "noise.h"
#include "nodedef.h"

class Mapgen;
class MMVManip;
class PcgRandom;
class Schematic;

enum DecorationType {
	DECO_SIMPLE,
	DECO_SCHEMATIC,
	DECO_LSYSTEM
};

#define DECO_PLACE_CENTER_X  0x01
#define DECO_PLACE_CENTER_Y  0x02
#define DECO_PLACE_CENTER_Z  0x04
#define DECO_USE_NOISE       0x08
#define DECO_FORCE_PLACEMENT 0x10
#define DECO_LIQUID_SURFACE  0x20

extern FlagDesc flagdesc_deco[];


#if 0
struct CutoffData {
	VoxelArea a;
	Decoration *deco;
	//v3s16 p;
	//v3s16 size;
	//s16 height;

	CutoffData(s16 x, s16 y, s16 z, s16 h) {
		p = v3s16(x, y, z);
		height = h;
	}
};
#endif

class Decoration : public ObjDef, public NodeResolver {
public:
	Decoration();
	virtual ~Decoration();

	virtual void resolveNodeNames();

	size_t placeDeco(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
	//size_t placeCutoffs(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);

	virtual size_t generate(MMVManip *vm, PcgRandom *pr, v3s16 p) = 0;
	virtual int getHeight() = 0;

	u32 flags;
	int mapseed;
	std::vector<content_t> c_place_on;
	s16 sidelen;
	s16 y_min;
	s16 y_max;
	float fill_ratio;
	NoiseParams np;

	std::set<u8> biomes;
	//std::list<CutoffData> cutoffs;
	//Mutex cutoff_mutex;
};

class DecoSimple : public Decoration {
public:
	virtual size_t generate(MMVManip *vm, PcgRandom *pr, v3s16 p);
	bool canPlaceDecoration(MMVManip *vm, v3s16 p);
	virtual int getHeight();

	virtual void resolveNodeNames();

	std::vector<content_t> c_decos;
	std::vector<content_t> c_spawnby;
	s16 deco_height;
	s16 deco_height_max;
	s16 nspawnby;
};

class DecoSchematic : public Decoration {
public:
	DecoSchematic();

	virtual size_t generate(MMVManip *vm, PcgRandom *pr, v3s16 p);
	virtual int getHeight();

	Rotation rotation;
	Schematic *schematic;
};


/*
class DecoLSystem : public Decoration {
public:
	virtual void generate(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
};
*/

class DecorationManager : public ObjDefManager {
public:
	DecorationManager(IGameDef *gamedef);
	virtual ~DecorationManager() {}

	const char *getObjectTitle() const
	{
		return "decoration";
	}

	static Decoration *create(DecorationType type)
	{
		switch (type) {
		case DECO_SIMPLE:
			return new DecoSimple;
		case DECO_SCHEMATIC:
			return new DecoSchematic;
		//case DECO_LSYSTEM:
		//	return new DecoLSystem;
		default:
			return NULL;
		}
	}

	size_t placeAllDecos(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
};

#endif
