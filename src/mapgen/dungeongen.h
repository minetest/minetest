/*
Minetest
Copyright (C) 2010-2018 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2015-2018 paramat

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

#include "voxel.h"
#include "noise.h"
#include "mapgen.h"

#define VMANIP_FLAG_DUNGEON_INSIDE VOXELFLAG_CHECKED1
#define VMANIP_FLAG_DUNGEON_PRESERVE VOXELFLAG_CHECKED2
#define VMANIP_FLAG_DUNGEON_UNTOUCHABLE (\
		VMANIP_FLAG_DUNGEON_INSIDE|VMANIP_FLAG_DUNGEON_PRESERVE)

class MMVManip;
class NodeDefManager;

v3s16 rand_ortho_dir(PseudoRandom &random, bool diagonal_dirs);
v3s16 turn_xz(v3s16 olddir, int t);
v3s16 random_turn(PseudoRandom &random, v3s16 olddir);
int dir_to_facedir(v3s16 d);


struct DungeonParams {
	s32 seed;

	content_t c_wall;
	content_t c_alt_wall;
	content_t c_stair;

	bool diagonal_dirs;
	bool only_in_ground;
	v3s16 holesize;
	u16 corridor_len_min;
	u16 corridor_len_max;
	v3s16 room_size_min;
	v3s16 room_size_max;
	v3s16 room_size_large_min;
	v3s16 room_size_large_max;
	u16 rooms_min;
	u16 rooms_max;
	GenNotifyType notifytype;

	NoiseParams np_density;
	NoiseParams np_alt_wall;
};

class DungeonGen {
public:
	MMVManip *vm;
	const NodeDefManager *ndef;
	GenerateNotifier *gennotify;

	u32 blockseed;
	PseudoRandom random;
	v3s16 csize;

	content_t c_torch;
	DungeonParams dp;

	// RoomWalker
	v3s16 m_pos;
	v3s16 m_dir;

	DungeonGen(const NodeDefManager *ndef,
		GenerateNotifier *gennotify, DungeonParams *dparams);

	void generate(MMVManip *vm, u32 bseed,
		v3s16 full_node_min, v3s16 full_node_max);

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

	inline void randomizeDir()
	{
		m_dir = rand_ortho_dir(random, dp.diagonal_dirs);
	}
};

extern NoiseParams nparams_dungeon_density;
extern NoiseParams nparams_dungeon_alt_wall;
