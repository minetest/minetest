/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef DUNGEONGEN_HEADER
#define DUNGEONGEN_HEADER

#include "voxel.h"
#include "noise.h"

#define VMANIP_FLAG_DUNGEON_INSIDE VOXELFLAG_CHECKED1
#define VMANIP_FLAG_DUNGEON_PRESERVE VOXELFLAG_CHECKED2
#define VMANIP_FLAG_DUNGEON_UNTOUCHABLE (\
		VMANIP_FLAG_DUNGEON_INSIDE|VMANIP_FLAG_DUNGEON_PRESERVE)

class ManualMapVoxelManipulator;
class INodeDefManager;
class Mapgen;

v3s16 rand_ortho_dir(PseudoRandom &random, bool diagonal_dirs);
v3s16 turn_xz(v3s16 olddir, int t);
v3s16 random_turn(PseudoRandom &random, v3s16 olddir);
int dir_to_facedir(v3s16 d);


struct DungeonParams {
	content_t c_water;
	content_t c_cobble;
	content_t c_moss;
	content_t c_stair;

	int notifytype;
	bool diagonal_dirs;
	float mossratio;
	v3s16 holesize;
	v3s16 roomsize;

	NoiseParams np_rarity;
	NoiseParams np_wetness;
	NoiseParams np_density;
};

class DungeonGen {
public:
	ManualMapVoxelManipulator *vm;
	Mapgen *mg;
	u32 blockseed;
	PseudoRandom random;
	v3s16 csize;

	content_t c_torch;
	DungeonParams dp;
	
	//RoomWalker
	v3s16 m_pos;
	v3s16 m_dir;

	DungeonGen(Mapgen *mg, DungeonParams *dparams);
	void generate(u32 bseed, v3s16 full_node_min, v3s16 full_node_max);
	
	void makeDungeon(v3s16 start_padding);
	void makeRoom(v3s16 roomsize, v3s16 roomplace);
	void makeCorridor(v3s16 doorplace, v3s16 doordir,
					  v3s16 &result_place, v3s16 &result_dir);
	void makeDoor(v3s16 doorplace, v3s16 doordir);
	void makeFill(v3s16 place, v3s16 size, u8 avoid_flags, MapNode n, u8 or_flags);
	void makeHole(v3s16 place);

	bool findPlaceForDoor(v3s16 &result_place, v3s16 &result_dir);
	bool findPlaceForRoomDoor(v3s16 roomsize, v3s16 &result_doorplace,
			v3s16 &result_doordir, v3s16 &result_roomplace);
			
	void randomizeDir()
	{
		m_dir = rand_ortho_dir(random, dp.diagonal_dirs);
	}
};

extern NoiseParams nparams_dungeon_rarity;
extern NoiseParams nparams_dungeon_wetness;
extern NoiseParams nparams_dungeon_density;

#endif
