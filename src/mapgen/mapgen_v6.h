/*
Minetest
Copyright (C) 2010-2018 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2018 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2014-2018 paramat

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

#include "mapgen.h"
#include "noise.h"

#define MGV6_AVERAGE_MUD_AMOUNT 4
#define MGV6_DESERT_STONE_BASE -32
#define MGV6_ICE_BASE 0
#define MGV6_FREQ_HOT 0.4
#define MGV6_FREQ_SNOW -0.4
#define MGV6_FREQ_TAIGA 0.5
#define MGV6_FREQ_JUNGLE 0.5

//////////// Mapgen V6 flags
#define MGV6_JUNGLES    0x01
#define MGV6_BIOMEBLEND 0x02
#define MGV6_MUDFLOW    0x04
#define MGV6_SNOWBIOMES 0x08
#define MGV6_FLAT       0x10
#define MGV6_TREES      0x20


extern FlagDesc flagdesc_mapgen_v6[];


enum BiomeV6Type
{
	BT_NORMAL,
	BT_DESERT,
	BT_JUNGLE,
	BT_TUNDRA,
	BT_TAIGA,
};


struct MapgenV6Params : public MapgenParams {
	float freq_desert = 0.45f;
	float freq_beach = 0.15f;
	pos_t dungeon_ymin = -MAX_MAP_GENERATION_LIMIT;
	pos_t dungeon_ymax = MAX_MAP_GENERATION_LIMIT;

	NoiseParams np_terrain_base;
	NoiseParams np_terrain_higher;
	NoiseParams np_steepness;
	NoiseParams np_height_select;
	NoiseParams np_mud;
	NoiseParams np_beach;
	NoiseParams np_biome;
	NoiseParams np_cave;
	NoiseParams np_humidity;
	NoiseParams np_trees;
	NoiseParams np_apple_trees;

	MapgenV6Params();
	~MapgenV6Params() = default;

	void readParams(const Settings *settings);
	void writeParams(Settings *settings) const;
	void setDefaultSettings(Settings *settings);
};


class MapgenV6 : public Mapgen {
public:
	EmergeParams *m_emerge;

	int ystride;
	u32 spflags;

	v3pos_t node_min;
	v3pos_t node_max;
	v3pos_t full_node_min;
	v3pos_t full_node_max;
	v3pos_t central_area_size;

	Noise *noise_terrain_base;
	Noise *noise_terrain_higher;
	Noise *noise_steepness;
	Noise *noise_height_select;
	Noise *noise_mud;
	Noise *noise_beach;
	Noise *noise_biome;
	Noise *noise_humidity;
	NoiseParams *np_cave;
	NoiseParams *np_humidity;
	NoiseParams *np_trees;
	NoiseParams *np_apple_trees;

	NoiseParams np_dungeons;

	float freq_desert;
	float freq_beach;
	pos_t dungeon_ymin;
	pos_t dungeon_ymax;

	content_t c_stone;
	content_t c_dirt;
	content_t c_dirt_with_grass;
	content_t c_sand;
	content_t c_water_source;
	content_t c_lava_source;
	content_t c_gravel;
	content_t c_desert_stone;
	content_t c_desert_sand;
	content_t c_dirt_with_snow;
	content_t c_snow;
	content_t c_snowblock;
	content_t c_ice;

	content_t c_cobble;
	content_t c_mossycobble;
	content_t c_stair_cobble;
	content_t c_stair_desert_stone;

	MapgenV6(MapgenV6Params *params, EmergeParams *emerge);
	~MapgenV6();

	virtual MapgenType getType() const { return MAPGEN_V6; }

	void makeChunk(BlockMakeData *data);
	int getGroundLevelAtPoint(v2pos_t p);
	int getSpawnLevelAtPoint(v2pos_t p);

	float baseTerrainLevel(float terrain_base, float terrain_higher,
		float steepness, float height_select);
	virtual float baseTerrainLevelFromNoise(v2pos_t p);
	virtual float baseTerrainLevelFromMap(v2pos_t p);
	virtual float baseTerrainLevelFromMap(int index);

	pos_t find_stone_level(v2pos_t p2d);
	bool block_is_underground(u64 seed, v3pos_t blockpos);

	float getHumidity(v2pos_t p);
	float getTreeAmount(v2pos_t p);
	bool getHaveAppleTree(v2pos_t p);
	float getMudAmount(int index);
	bool getHaveBeach(int index);
	BiomeV6Type getBiome(v2pos_t p);
	BiomeV6Type getBiome(int index, v2pos_t p);

	u32 get_blockseed(u64 seed, v3pos_t p);

	virtual void calculateNoise();
	int generateGround();
	void addMud();
	void flowMud(pos_t &mudflow_minpos, pos_t &mudflow_maxpos);
	void moveMud(u32 remove_index, u32 place_index,
		u32 above_remove_index, v2pos_t pos, v3pos_t em);
	void growGrass();
	void placeTreesAndJungleGrass();
	virtual void generateCaves(int max_stone_y);
};
