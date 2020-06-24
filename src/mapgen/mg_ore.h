/*
Minetest
Copyright (C) 2015-2020 paramat
Copyright (C) 2014-2016 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#pragma once

#include <unordered_set>
#include "objdef.h"
#include "noise.h"
#include "nodedef.h"

typedef u16 biome_t;  // copy from mg_biome.h to avoid an unnecessary include

class Noise;
class Mapgen;
class MMVManip;

/////////////////// Ore generation flags

#define OREFLAG_ABSHEIGHT     0x01 // Non-functional but kept to not break flags
#define OREFLAG_PUFF_CLIFFS   0x02
#define OREFLAG_PUFF_ADDITIVE 0x04
#define OREFLAG_USE_NOISE     0x08
#define OREFLAG_USE_NOISE2    0x10

enum OreType {
	ORE_SCATTER,
	ORE_SHEET,
	ORE_PUFF,
	ORE_BLOB,
	ORE_VEIN,
	ORE_STRATUM,
};

extern FlagDesc flagdesc_ore[];

class Ore : public ObjDef, public NodeResolver {
public:
	static const bool NEEDS_NOISE = false;

	content_t c_ore;                  // the node to place
	std::vector<content_t> c_wherein; // the nodes to be placed in
	u32 clust_scarcity; // ore cluster has a 1-in-clust_scarcity chance of appearing at a node
	s16 clust_num_ores; // how many ore nodes are in a chunk
	s16 clust_size;     // how large (in nodes) a chunk of ore is
	s16 y_min;
	s16 y_max;
	u8 ore_param2;		// to set node-specific attributes
	u32 flags = 0;          // attributes for this ore
	float nthresh;      // threshold for noise at which an ore is placed
	NoiseParams np;     // noise for distribution of clusters (NULL for uniform scattering)
	Noise *noise = nullptr;
	std::unordered_set<biome_t> biomes;

	Ore() = default;;
	virtual ~Ore();

	virtual void resolveNodeNames();

	size_t placeOre(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);
	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap) = 0;

protected:
	void cloneTo(Ore *def) const;
};

class OreScatter : public Ore {
public:
	static const bool NEEDS_NOISE = false;

	ObjDef *clone() const;

	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap);
};

class OreSheet : public Ore {
public:
	static const bool NEEDS_NOISE = true;

	ObjDef *clone() const;

	u16 column_height_min;
	u16 column_height_max;
	float column_midpoint_factor;

	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap);
};

class OrePuff : public Ore {
public:
	static const bool NEEDS_NOISE = true;

	ObjDef *clone() const;

	NoiseParams np_puff_top;
	NoiseParams np_puff_bottom;
	Noise *noise_puff_top = nullptr;
	Noise *noise_puff_bottom = nullptr;

	OrePuff() = default;
	virtual ~OrePuff();

	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap);
};

class OreBlob : public Ore {
public:
	static const bool NEEDS_NOISE = true;

	ObjDef *clone() const;

	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap);
};

class OreVein : public Ore {
public:
	static const bool NEEDS_NOISE = true;

	ObjDef *clone() const;

	float random_factor;
	Noise *noise2 = nullptr;
	int sizey_prev = 0;

	OreVein() = default;
	virtual ~OreVein();

	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap);
};

class OreStratum : public Ore {
public:
	static const bool NEEDS_NOISE = false;

	ObjDef *clone() const;

	NoiseParams np_stratum_thickness;
	Noise *noise_stratum_thickness = nullptr;
	u16 stratum_thickness;

	OreStratum() = default;
	virtual ~OreStratum();

	virtual void generate(MMVManip *vm, int mapseed, u32 blockseed,
		v3s16 nmin, v3s16 nmax, biome_t *biomemap);
};

class OreManager : public ObjDefManager {
public:
	OreManager(IGameDef *gamedef);
	virtual ~OreManager() = default;

	OreManager *clone() const;

	const char *getObjectTitle() const
	{
		return "ore";
	}

	static Ore *create(OreType type)
	{
		switch (type) {
		case ORE_SCATTER:
			return new OreScatter;
		case ORE_SHEET:
			return new OreSheet;
		case ORE_PUFF:
			return new OrePuff;
		case ORE_BLOB:
			return new OreBlob;
		case ORE_VEIN:
			return new OreVein;
		case ORE_STRATUM:
			return new OreStratum;
		default:
			return nullptr;
		}
	}

	void clear();

	size_t placeAllOres(Mapgen *mg, u32 blockseed, v3s16 nmin, v3s16 nmax);

private:
	OreManager() {};
};
