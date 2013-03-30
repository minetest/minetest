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

v3s16 rand_ortho_dir(PseudoRandom &random);
v3s16 turn_xz(v3s16 olddir, int t);
v3s16 random_turn(PseudoRandom &random, v3s16 olddir);
int dir_to_facedir(v3s16 d);

class DungeonGen {
public:
	u32 blockseed;
	u64 mapseed;
	ManualMapVoxelManipulator *vmanip;
	INodeDefManager *ndef;
	PseudoRandom random;
	v3s16 csize;
	s16 water_level;
	
	NoiseParams *np_rarity;
	NoiseParams *np_wetness;
	NoiseParams *np_density;
	
	content_t cid_water_source;
	content_t cid_cobble;
	content_t cid_mossycobble;
	content_t cid_torch;
	content_t cid_cobblestair;
	
	//RoomWalker
	v3s16 m_pos;
	v3s16 m_dir;

	DungeonGen(INodeDefManager *ndef, u64 seed, s16 waterlevel);
	void generate(ManualMapVoxelManipulator *vm, u32 bseed,
				  v3s16 full_node_min, v3s16 full_node_max);
	//void generate(v3s16 full_node_min, v3s16 full_node_max, u32 bseed);
	
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
		m_dir = rand_ortho_dir(random);
	}
};

class RoomWalker
{
public:

	RoomWalker(VoxelManipulator &vmanip_, v3s16 pos, PseudoRandom &random,
			INodeDefManager *ndef):
			vmanip(vmanip_),
			m_pos(pos),
			m_random(random),
			m_ndef(ndef)
	{
		randomizeDir();
	}

	void randomizeDir()
	{
		m_dir = rand_ortho_dir(m_random);
	}

	void setPos(v3s16 pos)
	{
		m_pos = pos;
	}

	void setDir(v3s16 dir)
	{
		m_dir = dir;
	}

	//bool findPlaceForDoor(v3s16 &result_place, v3s16 &result_dir);
	//bool findPlaceForRoomDoor(v3s16 roomsize, v3s16 &result_doorplace,
	//		v3s16 &result_doordir, v3s16 &result_roomplace);

private:
	VoxelManipulator &vmanip;
	v3s16 m_pos;
	v3s16 m_dir;
	PseudoRandom &m_random;
	INodeDefManager *m_ndef;
};


#endif
