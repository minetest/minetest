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
#include "mapnode.h"

class NoiseParams;
class Mapgen;
class ManualMapVoxelManipulator;
class PseudoRandom;

enum DecorationType {
	DECO_SIMPLE,
	DECO_SCHEMATIC,
	DECO_LSYSTEM
};

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

class Decoration {
public:
	INodeDefManager *ndef;

	int mapseed;
	std::vector<content_t> c_place_on;
	s16 sidelen;
	float fill_ratio;
	NoiseParams *np;

	std::set<u8> biomes;
	//std::list<CutoffData> cutoffs;
	//JMutex cutoff_mutex;

	Decoration();
	virtual ~Decoration();

	void placeDeco(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
	void placeCutoffs(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);

	virtual void generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p) = 0;
	virtual int getHeight() = 0;
	virtual std::string getName() = 0;
};

class DecoSimple : public Decoration {
public:
	std::vector<content_t> c_decos;
	std::vector<content_t> c_spawnby;
	s16 deco_height;
	s16 deco_height_max;
	s16 nspawnby;

	~DecoSimple() {}

	bool canPlaceDecoration(ManualMapVoxelManipulator *vm, v3s16 p);
	virtual void generate(Mapgen *mg, PseudoRandom *pr, s16 max_y, v3s16 p);
	virtual int getHeight();
	virtual std::string getName();
};

/*
class DecoLSystem : public Decoration {
public:
	virtual void generate(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
};
*/

Decoration *createDecoration(DecorationType type);

#endif
