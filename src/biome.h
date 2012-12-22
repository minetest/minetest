/*
Minetest-c55
Copyright (C) 2010-2011 kwolekr, Ryan Kwolek <kwolekr2@cs.scranton.edu>

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

#ifndef BIOME_HEADER
#define BIOME_HEADER

#include <string>
#include "nodedef.h"
#include "gamedef.h"
#include "mapnode.h"
#include "noise.h"
#include "mapgen.h"


enum BiomeTerrainType
{
	BIOME_TERRAIN_NORMAL,
	BIOME_TERRAIN_LIQUID,
	BIOME_TERRAIN_NETHER,
	BIOME_TERRAIN_AETHER,
	BIOME_TERRAIN_FLAT
};

class Biome {
public:
	MapNode n_top;
	MapNode n_filler;
	s16 ntopnodes;
	s8 groupid;
	s8 flags;
	s16 height_min;
	s16 height_max;
	float heat_min;
	float heat_max;
	float humidity_min;
	float humidity_max;
	std::string name;
	NoiseParams *np;

	virtual void genColumn(MapgenV7 *mg, int x, int z, int y1, int y2);
	virtual int getSurfaceHeight(float noise_terrain);
};

class BiomeLiquid : public Biome {
	virtual void genColumn(MapgenV7 *mg, int x, int z, int y1, int y2);
};

class BiomeHell : public Biome {
	virtual void genColumn(MapgenV7 *mg, int x, int z, int y1, int y2);
	virtual int getSurfaceHeight(float noise_terrain);
};

class BiomeAether : public Biome {
	virtual void genColumn(MapgenV7 *mg, int x, int z, int y1, int y2);
	virtual int getSurfaceHeight(float noise_terrain);
};

class BiomeSuperflat : public Biome {
	virtual void genColumn(MapgenV7 *mg, int x, int z, int y1, int y2);
	virtual int getSurfaceHeight(float noise_terrain);
};

class BiomeDefManager {
public:
	std::vector<float> bgroup_freqs;
	std::vector<std::vector<Biome *> *> bgroups;
	Biome *biome_default;
	IGameDef *m_gamedef;
	INodeDefManager *ndef;

	BiomeDefManager(IGameDef *gamedef);
	~BiomeDefManager();

	Biome *createBiome(BiomeTerrainType btt);
	Biome *getBiome(float bgfreq, float heat, float humidity);

	void addBiomeGroup(float freq);
	void addBiome(Biome *b);
	void addDefaultBiomes();
};

#endif
