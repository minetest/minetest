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

#include "dungeongen.h"
#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "mapblock.h"
#include "mapnode.h"
#include "map.h"
#include "nodedef.h"
#include "profiler.h"
#include "settings.h" // For g_settings
#include "main.h" // For g_profiler

NoiseParams nparams_dungeon_rarity = 
	{0.0, 1.0, v3f(500.0, 500.0, 500.0), 0, 2, 0.8};
NoiseParams nparams_dungeon_wetness =
	{0.0, 1.0, v3f(40.0, 40.0, 40.0), 32474, 4, 1.1};
NoiseParams nparams_dungeon_density =
	{0.0, 1.0, v3f(2.5, 2.5, 2.5), 0, 2, 1.4};


///////////////////////////////////////////////////////////////////////////////


DungeonGen::DungeonGen(INodeDefManager *ndef, u64 seed, s16 waterlevel) {
	this->ndef        = ndef;
	this->mapseed     = seed;
	this->water_level = waterlevel;
	
	np_rarity  = &nparams_dungeon_rarity;
	np_wetness = &nparams_dungeon_wetness;
	np_density = &nparams_dungeon_density;
	/*
	cid_water_source = ndef->getId("mapgen_water_source");
	cid_cobble       = ndef->getId("mapgen_cobble");
	cid_mossycobble  = ndef->getId("mapgen_mossycobble");
	cid_torch        = ndef->getId("default:torch");
	*/
}


void DungeonGen::generate(ManualMapVoxelManipulator *vm, u32 bseed,
						 v3s16 nmin, v3s16 nmax) {
	//TimeTaker t("gen dungeons");
	int approx_groundlevel = 10 + water_level;

	if ((nmin.Y + nmax.Y) / 2 >= approx_groundlevel ||
		NoisePerlin3D(np_rarity, nmin.X, nmin.Y, nmin.Z, mapseed) < 0.2)
		return;
		
	this->vmanip    = vm;
	this->blockseed = bseed;
	random.seed(bseed + 2);

	cid_water_source = ndef->getId("mapgen_water_source");
	cid_cobble       = ndef->getId("mapgen_cobble");
	cid_mossycobble  = ndef->getId("mapgen_mossycobble");
	//cid_torch        = ndef->getId("default:torch");
	cid_cobblestair  = ndef->getId("mapgen_stair_cobble");

	// Dungeon generator doesn't modify places which have this set
	vmanip->clearFlag(VMANIP_FLAG_DUNGEON_INSIDE | VMANIP_FLAG_DUNGEON_PRESERVE);

	// Set all air and water to be untouchable to make dungeons open
	// to caves and open air
	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 y = nmin.Y; y <= nmax.Y; y++) {
			u32 i = vmanip->m_area.index(nmin.X, y, z);
			for (s16 x = nmin.X; x <= nmax.X; x++) {
				content_t c = vmanip->m_data[i].getContent();
				if (c == CONTENT_AIR || c == cid_water_source)
					vmanip->m_flags[i] |= VMANIP_FLAG_DUNGEON_PRESERVE;
				i++;
			}
		}
	}
	
	// Add it
	makeDungeon(v3s16(1,1,1) * MAP_BLOCKSIZE);
	
	// Convert some cobble to mossy cobble
	for (s16 z = nmin.Z; z <= nmax.Z; z++) {
		for (s16 y = nmin.Y; y <= nmax.Y; y++) {
			u32 i = vmanip->m_area.index(nmin.X, y, z);
			for (s16 x = nmin.X; x <= nmax.X; x++) {
				if (vmanip->m_data[i].getContent() == cid_cobble) {
					float wetness = NoisePerlin3D(np_wetness, x, y, z, mapseed);
					float density = NoisePerlin3D(np_density, x, y, z, blockseed);
					if (density < wetness / 3.0)
						vmanip->m_data[i].setContent(cid_mossycobble);
				}
				i++;
			}
		}
	}
	
	//printf("== gen dungeons: %dms\n", t.stop());
}


void DungeonGen::makeDungeon(v3s16 start_padding)
{
	v3s16 areasize = vmanip->m_area.getExtent();
	v3s16 roomsize;
	v3s16 roomplace;

	/*
		Find place for first room
	*/
	bool fits = false;
	for (u32 i = 0; i < 100; i++)
	{
		bool is_large_room = ((random.next() & 3) == 1);
		roomsize = is_large_room ?
			v3s16(random.range(8, 16),random.range(8, 16),random.range(8, 16)) :
			v3s16(random.range(4,  8),random.range(4,  6),random.range(4, 8));
		
		// start_padding is used to disallow starting the generation of
		// a dungeon in a neighboring generation chunk
		roomplace = vmanip->m_area.MinEdge + start_padding + v3s16(
			random.range(0,areasize.X-roomsize.X-1-start_padding.X),
			random.range(0,areasize.Y-roomsize.Y-1-start_padding.Y),
			random.range(0,areasize.Z-roomsize.Z-1-start_padding.Z));
			
		/*
			Check that we're not putting the room to an unknown place,
			otherwise it might end up floating in the air
		*/
		fits = true;
		for (s16 z = 1; z < roomsize.Z - 1; z++)
		for (s16 y = 1; y < roomsize.Y - 1; y++)
		for (s16 x = 1; x < roomsize.X - 1; x++)
		{
			v3s16 p = roomplace + v3s16(x, y, z);
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_INSIDE)
			{
				fits = false;
				break;
			}
			if (vmanip->m_data[vi].getContent() == CONTENT_IGNORE)
			{
				fits = false;
				break;
			}
		}
		if (fits)
			break;
	}
	// No place found
	if (fits == false)
		return;

	/*
		Stores the center position of the last room made, so that
		a new corridor can be started from the last room instead of
		the new room, if chosen so.
	*/
	v3s16 last_room_center = roomplace + v3s16(roomsize.X / 2, 1, roomsize.Z / 2);

	u32 room_count = random.range(2, 16);
	for (u32 i = 0; i < room_count; i++)
	{
		// Make a room to the determined place
		makeRoom(roomsize, roomplace);

		v3s16 room_center = roomplace + v3s16(roomsize.X / 2, 1, roomsize.Z / 2);

		// Place torch at room center (for testing)
		//vmanip->m_data[vmanip->m_area.index(room_center)] = MapNode(cid_torch);

		// Quit if last room
		if (i == room_count - 1)
			break;

		// Determine walker start position

		bool start_in_last_room = (random.range(0, 2) != 0);

		v3s16 walker_start_place;

		if(start_in_last_room)
		{
			walker_start_place = last_room_center;
		}
		else
		{
			walker_start_place = room_center;
			// Store center of current room as the last one
			last_room_center = room_center;
		}

		// Create walker and find a place for a door
		v3s16 doorplace;
		v3s16 doordir;
		
		m_pos = walker_start_place;
		bool r = findPlaceForDoor(doorplace, doordir);
		if (r == false)
			return;

		if (random.range(0,1) == 0)
			// Make the door
			makeDoor(doorplace, doordir);
		else
			// Don't actually make a door
			doorplace -= doordir;

		// Make a random corridor starting from the door
		v3s16 corridor_end;
		v3s16 corridor_end_dir;
		makeCorridor(doorplace, doordir, corridor_end, corridor_end_dir);

		// Find a place for a random sized room
		roomsize = v3s16(random.range(4,8),random.range(4,6),random.range(4,8));
		m_pos = corridor_end;
		m_dir = corridor_end_dir;
		r = findPlaceForRoomDoor(roomsize, doorplace, doordir, roomplace);
		if (r == false)
			return;

		if (random.range(0,1) == 0)
			// Make the door
			makeDoor(doorplace, doordir);
		else
			// Don't actually make a door
			roomplace -= doordir;

	}
}


void DungeonGen::makeRoom(v3s16 roomsize, v3s16 roomplace)
{
	MapNode n_cobble(cid_cobble);
	MapNode n_air(CONTENT_AIR);
	
	// Make +-X walls
	for (s16 z = 0; z < roomsize.Z; z++)
	for (s16 y = 0; y < roomsize.Y; y++)
	{
		{
			v3s16 p = roomplace + v3s16(0, y, z);
			if (vmanip->m_area.contains(p) == false)
				continue;
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip->m_data[vi] = n_cobble;
		}
		{
			v3s16 p = roomplace + v3s16(roomsize.X - 1, y, z);
			if (vmanip->m_area.contains(p) == false)
				continue;
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip->m_data[vi] = n_cobble;
		}
	}

	// Make +-Z walls
	for (s16 x = 0; x < roomsize.X; x++)
	for (s16 y = 0; y < roomsize.Y; y++)
	{
		{
			v3s16 p = roomplace + v3s16(x, y, 0);
			if (vmanip->m_area.contains(p) == false)
				continue;
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip->m_data[vi] = n_cobble;
		}
		{
			v3s16 p = roomplace + v3s16(x, y, roomsize.Z - 1);
			if (vmanip->m_area.contains(p) == false)
				continue;
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip->m_data[vi] = n_cobble;
		}
	}

	// Make +-Y walls (floor and ceiling)
	for (s16 z = 0; z < roomsize.Z; z++)
	for (s16 x = 0; x < roomsize.X; x++)
	{
		{
			v3s16 p = roomplace + v3s16(x, 0, z);
			if (vmanip->m_area.contains(p) == false)
				continue;
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip->m_data[vi] = n_cobble;
		}
		{
			v3s16 p = roomplace + v3s16(x,roomsize. Y - 1, z);
			if (vmanip->m_area.contains(p) == false)
				continue;
			u32 vi = vmanip->m_area.index(p);
			if (vmanip->m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip->m_data[vi] = n_cobble;
		}
	}

	// Fill with air
	for (s16 z = 1; z < roomsize.Z - 1; z++)
	for (s16 y = 1; y < roomsize.Y - 1; y++)
	for (s16 x = 1; x < roomsize.X - 1; x++)
	{
		v3s16 p = roomplace + v3s16(x, y, z);
		if (vmanip->m_area.contains(p) == false)
			continue;
		u32 vi = vmanip->m_area.index(p);
		vmanip->m_flags[vi] |= VMANIP_FLAG_DUNGEON_UNTOUCHABLE;
		vmanip->m_data[vi]   = n_air;
	}
}


void DungeonGen::makeFill(v3s16 place, v3s16 size,
		u8 avoid_flags, MapNode n, u8 or_flags)
{
	for (s16 z = 0; z < size.Z; z++)
	for (s16 y = 0; y < size.Y; y++)
	for (s16 x = 0; x < size.X; x++)
	{
		v3s16 p = place + v3s16(x, y, z);
		if (vmanip->m_area.contains(p) == false)
			continue;
		u32 vi = vmanip->m_area.index(p);
		if (vmanip->m_flags[vi] & avoid_flags)
			continue;
		vmanip->m_flags[vi] |= or_flags;
		vmanip->m_data[vi]   = n;
	}
}


void DungeonGen::makeHole(v3s16 place)
{
	makeFill(place, v3s16(1, 2, 1), 0, MapNode(CONTENT_AIR),
			VMANIP_FLAG_DUNGEON_INSIDE);
}


void DungeonGen::makeDoor(v3s16 doorplace, v3s16 doordir)
{
	makeHole(doorplace);
	// Place torch (for testing)
	//vmanip->m_data[vmanip->m_area.index(doorplace)] = MapNode(cid_torch);
}


void DungeonGen::makeCorridor(v3s16 doorplace,
		v3s16 doordir, v3s16 &result_place, v3s16 &result_dir)
{
	makeHole(doorplace);
	v3s16 p0 = doorplace;
	v3s16 dir = doordir;
	u32 length;
	/*if (random.next() % 2)
		length = random.range(1, 13);
	else
		length = random.range(1, 6);*/
	length = random.range(1, 13);
	u32 partlength = random.range(1, 13);
	u32 partcount = 0;
	s16 make_stairs = 0;
	
	if (random.next() % 2 == 0 && partlength >= 3)
		make_stairs = random.next() % 2 ? 1 : -1;
	
	for (u32 i = 0; i < length; i++) {
		v3s16 p = p0 + dir;
		if (partcount != 0)
			p.Y += make_stairs;

		if (vmanip->m_area.contains(p) == true &&
			vmanip->m_area.contains(p + v3s16(0, 1, 0)) == true) {
			if (make_stairs) {
				makeFill(p + v3s16(-1, -1, -1), v3s16(3, 5, 3),
						VMANIP_FLAG_DUNGEON_UNTOUCHABLE, MapNode(cid_cobble), 0);
				makeHole(p);
				makeHole(p - dir);
				
				// TODO: fix stairs code so it works 100% (quite difficult)

				// exclude stairs from the bottom step
				if (((make_stairs ==  1) && i != 0) ||
					((make_stairs == -1) && i != length - 1)) {
					// rotate face 180 deg if making stairs backwards
					int facedir = dir_to_facedir(dir * make_stairs);
					
					u32 vi = vmanip->m_area.index(p.X - dir.X, p.Y - 1, p.Z - dir.Z);
					if (vmanip->m_data[vi].getContent() == cid_cobble)
						vmanip->m_data[vi] = MapNode(cid_cobblestair, 0, facedir);
					
					vi = vmanip->m_area.index(p.X, p.Y, p.Z);
					if (vmanip->m_data[vi].getContent() == cid_cobble)
						vmanip->m_data[vi] = MapNode(cid_cobblestair, 0, facedir);
				}
			} else {
				makeFill(p + v3s16(-1, -1, -1), v3s16(3, 4, 3),
						VMANIP_FLAG_DUNGEON_UNTOUCHABLE, MapNode(cid_cobble), 0);
				makeHole(p);
			}

			p0 = p;
		} else {
			// Can't go here, turn away
			dir = turn_xz(dir, random.range(0, 1));
			make_stairs = -make_stairs;
			partcount = 0;
			partlength = random.range(1, length);
			continue;
		}

		partcount++;
		if (partcount >= partlength) {
			partcount = 0;

			dir = random_turn(random, dir);

			partlength = random.range(1,length);

			make_stairs = 0;
			if (random.next() % 2 == 0 && partlength >= 3)
				make_stairs = random.next() % 2 ? 1 : -1;
		}
	}
	result_place = p0;
	result_dir = dir;
}


bool DungeonGen::findPlaceForDoor(v3s16 &result_place, v3s16 &result_dir)
{
	for (u32 i = 0; i < 100; i++)
	{
		v3s16 p = m_pos + m_dir;
		v3s16 p1 = p + v3s16(0, 1, 0);
		if (vmanip->m_area.contains(p) == false
		 || vmanip->m_area.contains(p1) == false
		 || i % 4 == 0)
		{
			randomizeDir();
			continue;
		}
		if (vmanip->getNodeNoExNoEmerge(p).getContent()  == cid_cobble
		 && vmanip->getNodeNoExNoEmerge(p1).getContent() == cid_cobble)
		{
			// Found wall, this is a good place!
			result_place = p;
			result_dir = m_dir;
			// Randomize next direction
			randomizeDir();
			return true;
		}
		/*
			Determine where to move next
		*/
		// Jump one up if the actual space is there
		if (vmanip->getNodeNoExNoEmerge(p+v3s16(0,0,0)).getContent() == cid_cobble
		 && vmanip->getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent() == CONTENT_AIR
		 && vmanip->getNodeNoExNoEmerge(p+v3s16(0,2,0)).getContent() == CONTENT_AIR)
			p += v3s16(0,1,0);
		// Jump one down if the actual space is there
		if (vmanip->getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent() == cid_cobble
		 && vmanip->getNodeNoExNoEmerge(p+v3s16(0,0,0)).getContent() == CONTENT_AIR
		 && vmanip->getNodeNoExNoEmerge(p+v3s16(0,-1,0)).getContent() == CONTENT_AIR)
			p += v3s16(0,-1,0);
		// Check if walking is now possible
		if (vmanip->getNodeNoExNoEmerge(p).getContent() != CONTENT_AIR
		 || vmanip->getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent() != CONTENT_AIR)
		{
			// Cannot continue walking here
			randomizeDir();
			continue;
		}
		// Move there
		m_pos = p;
	}
	return false;
}


bool DungeonGen::findPlaceForRoomDoor(v3s16 roomsize, v3s16 &result_doorplace,
		v3s16 &result_doordir, v3s16 &result_roomplace)
{
	for (s16 trycount = 0; trycount < 30; trycount++)
	{
		v3s16 doorplace;
		v3s16 doordir;
		bool r = findPlaceForDoor(doorplace, doordir);
		if (r == false)
			continue;
		v3s16 roomplace;
		// X east, Z north, Y up
#if 1
		if (doordir == v3s16(1, 0, 0)) // X+
			roomplace = doorplace +
					v3s16(0, -1, random.range(-roomsize.Z + 2, -2));
		if (doordir == v3s16(-1, 0, 0)) // X-
			roomplace = doorplace +
					v3s16(-roomsize.X + 1, -1, random.range(-roomsize.Z + 2, -2));
		if (doordir == v3s16(0, 0, 1)) // Z+
			roomplace = doorplace +
					v3s16(random.range(-roomsize.X + 2, -2), -1, 0);
		if (doordir == v3s16(0, 0, -1)) // Z-
			roomplace = doorplace +
					v3s16(random.range(-roomsize.X + 2, -2), -1, -roomsize.Z + 1);
#endif
#if 0
		if (doordir == v3s16(1, 0, 0)) // X+
			roomplace = doorplace + v3s16(0, -1, -roomsize.Z / 2);
		if (doordir == v3s16(-1, 0, 0)) // X-
			roomplace = doorplace + v3s16(-roomsize.X+1,-1,-roomsize.Z / 2);
		if (doordir == v3s16(0, 0, 1)) // Z+
			roomplace = doorplace + v3s16(-roomsize.X / 2, -1, 0);
		if (doordir == v3s16(0, 0, -1)) // Z-
			roomplace = doorplace + v3s16(-roomsize.X / 2, -1, -roomsize.Z + 1);
#endif

		// Check fit
		bool fits = true;
		for (s16 z = 1; z < roomsize.Z - 1; z++)
		for (s16 y = 1; y < roomsize.Y - 1; y++)
		for (s16 x = 1; x < roomsize.X - 1; x++)
		{
			v3s16 p = roomplace + v3s16(x, y, z);
			if (vmanip->m_area.contains(p) == false)
			{
				fits = false;
				break;
			}
			if (vmanip->m_flags[vmanip->m_area.index(p)]
					& VMANIP_FLAG_DUNGEON_INSIDE)
			{
				fits = false;
				break;
			}
		}
		if(fits == false)
		{
			// Find new place
			continue;
		}
		result_doorplace = doorplace;
		result_doordir   = doordir;
		result_roomplace = roomplace;
		return true;
	}
	return false;
}


v3s16 rand_ortho_dir(PseudoRandom &random)
{
	if (random.next() % 2 == 0)
		return random.next() % 2 ? v3s16(-1, 0, 0) : v3s16(1, 0, 0);
	else
		return random.next() % 2 ? v3s16(0, 0, -1) : v3s16(0, 0, 1);
}


v3s16 turn_xz(v3s16 olddir, int t)
{
	v3s16 dir;
	if (t == 0)
	{
		// Turn right
		dir.X = olddir.Z;
		dir.Z = -olddir.X;
		dir.Y = olddir.Y;
	}
	else
	{
		// Turn left
		dir.X = -olddir.Z;
		dir.Z = olddir.X;
		dir.Y = olddir.Y;
	}
	return dir;
}


v3s16 random_turn(PseudoRandom &random, v3s16 olddir)
{
	int turn = random.range(0, 2);
	v3s16 dir;
	if (turn == 0)
	{
		// Go straight
		dir = olddir;
	}
	else if (turn == 1)
		// Turn right
		dir = turn_xz(olddir, 0);
	else
		// Turn left
		dir = turn_xz(olddir, 1);
	return dir;
}


int dir_to_facedir(v3s16 d) {
	if (abs(d.X) > abs(d.Z))
		return d.X < 0 ? 3 : 1;
	else
		return d.Z < 0 ? 2 : 0;
}
