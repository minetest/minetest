/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "mapgen.h"
#include "voxel.h"
#include "noise.h"
#include "mapblock.h"
#include "map.h"
//#include "serverobject.h"
#include "content_sao.h"
#include "nodedef.h"
#include "content_mapnode.h" // For content_mapnode_get_new_name
#include "voxelalgorithms.h"
#include "profiler.h"
#include "main.h" // For g_profiler

namespace mapgen
{

/*
	Some helper functions for the map generator
*/

#if 1
// Returns Y one under area minimum if not found
static s16 find_ground_level(VoxelManipulator &vmanip, v2s16 p2d,
		INodeDefManager *ndef)
{
	v3s16 em = vmanip.m_area.getExtent();
	s16 y_nodes_max = vmanip.m_area.MaxEdge.Y;
	s16 y_nodes_min = vmanip.m_area.MinEdge.Y;
	u32 i = vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
	s16 y;
	for(y=y_nodes_max; y>=y_nodes_min; y--)
	{
		MapNode &n = vmanip.m_data[i];
		if(ndef->get(n).walkable)
			break;

		vmanip.m_area.add_y(em, i, -1);
	}
	if(y >= y_nodes_min)
		return y;
	else
		return y_nodes_min - 1;
}

#if 0
// Returns Y one under area minimum if not found
static s16 find_ground_level_clever(VoxelManipulator &vmanip, v2s16 p2d,
		INodeDefManager *ndef)
{
	if(!vmanip.m_area.contains(v3s16(p2d.X, vmanip.m_area.MaxEdge.Y, p2d.Y)))
		return vmanip.m_area.MinEdge.Y-1;
	v3s16 em = vmanip.m_area.getExtent();
	s16 y_nodes_max = vmanip.m_area.MaxEdge.Y;
	s16 y_nodes_min = vmanip.m_area.MinEdge.Y;
	u32 i = vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
	s16 y;
	content_t c_tree = ndef->getId("mapgen_tree");
	content_t c_leaves = ndef->getId("mapgen_leaves");
	for(y=y_nodes_max; y>=y_nodes_min; y--)
	{
		MapNode &n = vmanip.m_data[i];
		if(ndef->get(n).walkable
				&& n.getContent() != c_tree
				&& n.getContent() != c_leaves)
			break;

		vmanip.m_area.add_y(em, i, -1);
	}
	if(y >= y_nodes_min)
		return y;
	else
		return y_nodes_min - 1;
}
#endif

// Returns Y one under area minimum if not found
static s16 find_stone_level(VoxelManipulator &vmanip, v2s16 p2d,
		INodeDefManager *ndef)
{
	v3s16 em = vmanip.m_area.getExtent();
	s16 y_nodes_max = vmanip.m_area.MaxEdge.Y;
	s16 y_nodes_min = vmanip.m_area.MinEdge.Y;
	u32 i = vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
	s16 y;
	content_t c_stone = ndef->getId("mapgen_stone");
	content_t c_desert_stone = ndef->getId("mapgen_desert_stone");
	for(y=y_nodes_max; y>=y_nodes_min; y--)
	{
		MapNode &n = vmanip.m_data[i];
		content_t c = n.getContent();
		if(c != CONTENT_IGNORE && (
				c == c_stone || c == c_desert_stone))
			break;

		vmanip.m_area.add_y(em, i, -1);
	}
	if(y >= y_nodes_min)
		return y;
	else
		return y_nodes_min - 1;
}
#endif

void make_tree(ManualMapVoxelManipulator &vmanip, v3s16 p0,
		bool is_apple_tree, INodeDefManager *ndef)
{
	MapNode treenode(ndef->getId("mapgen_tree"));
	MapNode leavesnode(ndef->getId("mapgen_leaves"));
	MapNode applenode(ndef->getId("mapgen_apple"));
	
	s16 trunk_h = myrand_range(4, 5);
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
		p1.Y++;
	}

	// p1 is now the last piece of the trunk
	p1.Y -= 1;

	VoxelArea leaves_a(v3s16(-2,-1,-2), v3s16(2,2,2));
	//SharedPtr<u8> leaves_d(new u8[leaves_a.getVolume()]);
	Buffer<u8> leaves_d(leaves_a.getVolume());
	for(s32 i=0; i<leaves_a.getVolume(); i++)
		leaves_d[i] = 0;

	// Force leaves at near the end of the trunk
	{
		s16 d = 1;
		for(s16 z=-d; z<=d; z++)
		for(s16 y=-d; y<=d; y++)
		for(s16 x=-d; x<=d; x++)
		{
			leaves_d[leaves_a.index(v3s16(x,y,z))] = 1;
		}
	}

	// Add leaves randomly
	for(u32 iii=0; iii<7; iii++)
	{
		s16 d = 1;

		v3s16 p(
			myrand_range(leaves_a.MinEdge.X, leaves_a.MaxEdge.X-d),
			myrand_range(leaves_a.MinEdge.Y, leaves_a.MaxEdge.Y-d),
			myrand_range(leaves_a.MinEdge.Z, leaves_a.MaxEdge.Z-d)
		);

		for(s16 z=0; z<=d; z++)
		for(s16 y=0; y<=d; y++)
		for(s16 x=0; x<=d; x++)
		{
			leaves_d[leaves_a.index(p+v3s16(x,y,z))] = 1;
		}
	}

	// Blit leaves to vmanip
	for(s16 z=leaves_a.MinEdge.Z; z<=leaves_a.MaxEdge.Z; z++)
	for(s16 y=leaves_a.MinEdge.Y; y<=leaves_a.MaxEdge.Y; y++)
	for(s16 x=leaves_a.MinEdge.X; x<=leaves_a.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		p += p1;
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		if(vmanip.m_data[vi].getContent() != CONTENT_AIR
				&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
			continue;
		u32 i = leaves_a.index(x,y,z);
		if(leaves_d[i] == 1) {
			bool is_apple = myrand_range(0,99) < 10;
			if(is_apple_tree && is_apple) {
				vmanip.m_data[vi] = applenode;
			} else {
				vmanip.m_data[vi] = leavesnode;
			}
		}
	}
}

#if 0
static void make_jungletree(VoxelManipulator &vmanip, v3s16 p0,
		INodeDefManager *ndef)
{
	MapNode treenode(ndef->getId("mapgen_jungletree"));
	MapNode leavesnode(ndef->getId("mapgen_leaves"));

	for(s16 x=-1; x<=1; x++)
	for(s16 z=-1; z<=1; z++)
	{
		if(myrand_range(0, 2) == 0)
			continue;
		v3s16 p1 = p0 + v3s16(x,0,z);
		v3s16 p2 = p0 + v3s16(x,-1,z);
		if(vmanip.m_area.contains(p2)
				&& vmanip.m_data[vmanip.m_area.index(p2)] == CONTENT_AIR)
			vmanip.m_data[vmanip.m_area.index(p2)] = treenode;
		else if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
	}

	s16 trunk_h = myrand_range(8, 12);
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
		p1.Y++;
	}

	// p1 is now the last piece of the trunk
	p1.Y -= 1;

	VoxelArea leaves_a(v3s16(-3,-2,-3), v3s16(3,2,3));
	//SharedPtr<u8> leaves_d(new u8[leaves_a.getVolume()]);
	Buffer<u8> leaves_d(leaves_a.getVolume());
	for(s32 i=0; i<leaves_a.getVolume(); i++)
		leaves_d[i] = 0;

	// Force leaves at near the end of the trunk
	{
		s16 d = 1;
		for(s16 z=-d; z<=d; z++)
		for(s16 y=-d; y<=d; y++)
		for(s16 x=-d; x<=d; x++)
		{
			leaves_d[leaves_a.index(v3s16(x,y,z))] = 1;
		}
	}

	// Add leaves randomly
	for(u32 iii=0; iii<30; iii++)
	{
		s16 d = 1;

		v3s16 p(
			myrand_range(leaves_a.MinEdge.X, leaves_a.MaxEdge.X-d),
			myrand_range(leaves_a.MinEdge.Y, leaves_a.MaxEdge.Y-d),
			myrand_range(leaves_a.MinEdge.Z, leaves_a.MaxEdge.Z-d)
		);

		for(s16 z=0; z<=d; z++)
		for(s16 y=0; y<=d; y++)
		for(s16 x=0; x<=d; x++)
		{
			leaves_d[leaves_a.index(p+v3s16(x,y,z))] = 1;
		}
	}

	// Blit leaves to vmanip
	for(s16 z=leaves_a.MinEdge.Z; z<=leaves_a.MaxEdge.Z; z++)
	for(s16 y=leaves_a.MinEdge.Y; y<=leaves_a.MaxEdge.Y; y++)
	for(s16 x=leaves_a.MinEdge.X; x<=leaves_a.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		p += p1;
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		if(vmanip.m_data[vi].getContent() != CONTENT_AIR
				&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
			continue;
		u32 i = leaves_a.index(x,y,z);
		if(leaves_d[i] == 1)
			vmanip.m_data[vi] = leavesnode;
	}
}

static void make_papyrus(VoxelManipulator &vmanip, v3s16 p0,
		INodeDefManager *ndef)
{
	MapNode papyrusnode(ndef->getId("mapgen_papyrus"));

	s16 trunk_h = myrand_range(2, 3);
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = papyrusnode;
		p1.Y++;
	}
}

static void make_cactus(VoxelManipulator &vmanip, v3s16 p0,
		INodeDefManager *ndef)
{
	MapNode cactusnode(ndef->getId("mapgen_cactus"));

	s16 trunk_h = 3;
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = cactusnode;
		p1.Y++;
	}
}
#endif

#if 0
/*
	Dungeon making routines
*/

#define VMANIP_FLAG_DUNGEON_INSIDE VOXELFLAG_CHECKED1
#define VMANIP_FLAG_DUNGEON_PRESERVE VOXELFLAG_CHECKED2
#define VMANIP_FLAG_DUNGEON_UNTOUCHABLE (\
		VMANIP_FLAG_DUNGEON_INSIDE|VMANIP_FLAG_DUNGEON_PRESERVE)

static void make_room1(VoxelManipulator &vmanip, v3s16 roomsize, v3s16 roomplace,
		INodeDefManager *ndef)
{
	// Make +-X walls
	for(s16 z=0; z<roomsize.Z; z++)
	for(s16 y=0; y<roomsize.Y; y++)
	{
		{
			v3s16 p = roomplace + v3s16(0,y,z);
			if(vmanip.m_area.contains(p) == false)
				continue;
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip.m_data[vi] = MapNode(ndef->getId("mapgen_cobble"));
		}
		{
			v3s16 p = roomplace + v3s16(roomsize.X-1,y,z);
			if(vmanip.m_area.contains(p) == false)
				continue;
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip.m_data[vi] = MapNode(ndef->getId("mapgen_cobble"));
		}
	}
	
	// Make +-Z walls
	for(s16 x=0; x<roomsize.X; x++)
	for(s16 y=0; y<roomsize.Y; y++)
	{
		{
			v3s16 p = roomplace + v3s16(x,y,0);
			if(vmanip.m_area.contains(p) == false)
				continue;
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip.m_data[vi] = MapNode(ndef->getId("mapgen_cobble"));
		}
		{
			v3s16 p = roomplace + v3s16(x,y,roomsize.Z-1);
			if(vmanip.m_area.contains(p) == false)
				continue;
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip.m_data[vi] = MapNode(ndef->getId("mapgen_cobble"));
		}
	}
	
	// Make +-Y walls (floor and ceiling)
	for(s16 z=0; z<roomsize.Z; z++)
	for(s16 x=0; x<roomsize.X; x++)
	{
		{
			v3s16 p = roomplace + v3s16(x,0,z);
			if(vmanip.m_area.contains(p) == false)
				continue;
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip.m_data[vi] = MapNode(ndef->getId("mapgen_cobble"));
		}
		{
			v3s16 p = roomplace + v3s16(x,roomsize.Y-1,z);
			if(vmanip.m_area.contains(p) == false)
				continue;
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_UNTOUCHABLE)
				continue;
			vmanip.m_data[vi] = MapNode(ndef->getId("mapgen_cobble"));
		}
	}
	
	// Fill with air
	for(s16 z=1; z<roomsize.Z-1; z++)
	for(s16 y=1; y<roomsize.Y-1; y++)
	for(s16 x=1; x<roomsize.X-1; x++)
	{
		v3s16 p = roomplace + v3s16(x,y,z);
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		vmanip.m_flags[vi] |= VMANIP_FLAG_DUNGEON_UNTOUCHABLE;
		vmanip.m_data[vi] = MapNode(CONTENT_AIR);
	}
}

static void make_fill(VoxelManipulator &vmanip, v3s16 place, v3s16 size,
		u8 avoid_flags, MapNode n, u8 or_flags)
{
	for(s16 z=0; z<size.Z; z++)
	for(s16 y=0; y<size.Y; y++)
	for(s16 x=0; x<size.X; x++)
	{
		v3s16 p = place + v3s16(x,y,z);
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		if(vmanip.m_flags[vi] & avoid_flags)
			continue;
		vmanip.m_flags[vi] |= or_flags;
		vmanip.m_data[vi] = n;
	}
}

static void make_hole1(VoxelManipulator &vmanip, v3s16 place,
		INodeDefManager *ndef)
{
	make_fill(vmanip, place, v3s16(1,2,1), 0, MapNode(CONTENT_AIR),
			VMANIP_FLAG_DUNGEON_INSIDE);
}

static void make_door1(VoxelManipulator &vmanip, v3s16 doorplace, v3s16 doordir,
		INodeDefManager *ndef)
{
	make_hole1(vmanip, doorplace, ndef);
	// Place torch (for testing)
	//vmanip.m_data[vmanip.m_area.index(doorplace)] = MapNode(ndef->getId("mapgen_torch"));
}

static v3s16 rand_ortho_dir(PseudoRandom &random)
{
	if(random.next()%2==0)
		return random.next()%2 ? v3s16(-1,0,0) : v3s16(1,0,0);
	else
		return random.next()%2 ? v3s16(0,0,-1) : v3s16(0,0,1);
}

static v3s16 turn_xz(v3s16 olddir, int t)
{
	v3s16 dir;
	if(t == 0)
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

static v3s16 random_turn(PseudoRandom &random, v3s16 olddir)
{
	int turn = random.range(0,2);
	v3s16 dir;
	if(turn == 0)
	{
		// Go straight
		dir = olddir;
	}
	else if(turn == 1)
		// Turn right
		dir = turn_xz(olddir, 0);
	else
		// Turn left
		dir = turn_xz(olddir, 1);
	return dir;
}

static void make_corridor(VoxelManipulator &vmanip, v3s16 doorplace,
		v3s16 doordir, v3s16 &result_place, v3s16 &result_dir,
		PseudoRandom &random, INodeDefManager *ndef)
{
	make_hole1(vmanip, doorplace, ndef);
	v3s16 p0 = doorplace;
	v3s16 dir = doordir;
	u32 length;
	if(random.next()%2)
		length = random.range(1,13);
	else
		length = random.range(1,6);
	length = random.range(1,13);
	u32 partlength = random.range(1,13);
	u32 partcount = 0;
	s16 make_stairs = 0;
	if(random.next()%2 == 0 && partlength >= 3)
		make_stairs = random.next()%2 ? 1 : -1;
	for(u32 i=0; i<length; i++)
	{
		v3s16 p = p0 + dir;
		if(partcount != 0)
			p.Y += make_stairs;

		/*// If already empty
		if(vmanip.getNodeNoExNoEmerge(p).getContent()
				== CONTENT_AIR
		&& vmanip.getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent()
				== CONTENT_AIR)
		{
		}*/

		if(vmanip.m_area.contains(p) == true
				&& vmanip.m_area.contains(p+v3s16(0,1,0)) == true)
		{
			if(make_stairs)
			{
				make_fill(vmanip, p+v3s16(-1,-1,-1), v3s16(3,5,3),
						VMANIP_FLAG_DUNGEON_UNTOUCHABLE, MapNode(ndef->getId("mapgen_cobble")), 0);
				make_fill(vmanip, p, v3s16(1,2,1), 0, MapNode(CONTENT_AIR),
						VMANIP_FLAG_DUNGEON_INSIDE);
				make_fill(vmanip, p-dir, v3s16(1,2,1), 0, MapNode(CONTENT_AIR),
						VMANIP_FLAG_DUNGEON_INSIDE);
			}
			else
			{
				make_fill(vmanip, p+v3s16(-1,-1,-1), v3s16(3,4,3),
						VMANIP_FLAG_DUNGEON_UNTOUCHABLE, MapNode(ndef->getId("mapgen_cobble")), 0);
				make_hole1(vmanip, p, ndef);
				/*make_fill(vmanip, p, v3s16(1,2,1), 0, MapNode(CONTENT_AIR),
						VMANIP_FLAG_DUNGEON_INSIDE);*/
			}

			p0 = p;
		}
		else
		{
			// Can't go here, turn away
			dir = turn_xz(dir, random.range(0,1));
			make_stairs = -make_stairs;
			partcount = 0;
			partlength = random.range(1,length);
			continue;
		}

		partcount++;
		if(partcount >= partlength)
		{
			partcount = 0;
			
			dir = random_turn(random, dir);
			
			partlength = random.range(1,length);

			make_stairs = 0;
			if(random.next()%2 == 0 && partlength >= 3)
				make_stairs = random.next()%2 ? 1 : -1;
		}
	}
	result_place = p0;
	result_dir = dir;
}

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
	
	bool findPlaceForDoor(v3s16 &result_place, v3s16 &result_dir)
	{
		for(u32 i=0; i<100; i++)
		{
			v3s16 p = m_pos + m_dir;
			v3s16 p1 = p + v3s16(0,1,0);
			if(vmanip.m_area.contains(p) == false
					|| vmanip.m_area.contains(p1) == false
					|| i % 4 == 0)
			{
				randomizeDir();
				continue;
			}
			if(vmanip.getNodeNoExNoEmerge(p).getContent()
					== m_ndef->getId("mapgen_cobble")
			&& vmanip.getNodeNoExNoEmerge(p1).getContent()
					== m_ndef->getId("mapgen_cobble"))
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
			if(vmanip.getNodeNoExNoEmerge(p+v3s16(0,0,0)).getContent()
					== m_ndef->getId("mapgen_cobble")
			&& vmanip.getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent()
					== CONTENT_AIR
			&& vmanip.getNodeNoExNoEmerge(p+v3s16(0,2,0)).getContent()
					== CONTENT_AIR)
				p += v3s16(0,1,0);
			// Jump one down if the actual space is there
			if(vmanip.getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent()
					== m_ndef->getId("mapgen_cobble")
			&& vmanip.getNodeNoExNoEmerge(p+v3s16(0,0,0)).getContent()
					== CONTENT_AIR
			&& vmanip.getNodeNoExNoEmerge(p+v3s16(0,-1,0)).getContent()
					== CONTENT_AIR)
				p += v3s16(0,-1,0);
			// Check if walking is now possible
			if(vmanip.getNodeNoExNoEmerge(p).getContent()
					!= CONTENT_AIR
			|| vmanip.getNodeNoExNoEmerge(p+v3s16(0,1,0)).getContent()
					!= CONTENT_AIR)
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

	bool findPlaceForRoomDoor(v3s16 roomsize, v3s16 &result_doorplace,
			v3s16 &result_doordir, v3s16 &result_roomplace)
	{
		for(s16 trycount=0; trycount<30; trycount++)
		{
			v3s16 doorplace;
			v3s16 doordir;
			bool r = findPlaceForDoor(doorplace, doordir);
			if(r == false)
				continue;
			v3s16 roomplace;
			// X east, Z north, Y up
#if 1
			if(doordir == v3s16(1,0,0)) // X+
				roomplace = doorplace +
						v3s16(0,-1,m_random.range(-roomsize.Z+2,-2));
			if(doordir == v3s16(-1,0,0)) // X-
				roomplace = doorplace +
						v3s16(-roomsize.X+1,-1,m_random.range(-roomsize.Z+2,-2));
			if(doordir == v3s16(0,0,1)) // Z+
				roomplace = doorplace +
						v3s16(m_random.range(-roomsize.X+2,-2),-1,0);
			if(doordir == v3s16(0,0,-1)) // Z-
				roomplace = doorplace +
						v3s16(m_random.range(-roomsize.X+2,-2),-1,-roomsize.Z+1);
#endif
#if 0
			if(doordir == v3s16(1,0,0)) // X+
				roomplace = doorplace + v3s16(0,-1,-roomsize.Z/2);
			if(doordir == v3s16(-1,0,0)) // X-
				roomplace = doorplace + v3s16(-roomsize.X+1,-1,-roomsize.Z/2);
			if(doordir == v3s16(0,0,1)) // Z+
				roomplace = doorplace + v3s16(-roomsize.X/2,-1,0);
			if(doordir == v3s16(0,0,-1)) // Z-
				roomplace = doorplace + v3s16(-roomsize.X/2,-1,-roomsize.Z+1);
#endif
			
			// Check fit
			bool fits = true;
			for(s16 z=1; z<roomsize.Z-1; z++)
			for(s16 y=1; y<roomsize.Y-1; y++)
			for(s16 x=1; x<roomsize.X-1; x++)
			{
				v3s16 p = roomplace + v3s16(x,y,z);
				if(vmanip.m_area.contains(p) == false)
				{
					fits = false;
					break;
				}
				if(vmanip.m_flags[vmanip.m_area.index(p)]
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
			result_doordir = doordir;
			result_roomplace = roomplace;
			return true;
		}
		return false;
	}

private:
	VoxelManipulator &vmanip;
	v3s16 m_pos;
	v3s16 m_dir;
	PseudoRandom &m_random;
	INodeDefManager *m_ndef;
};

static void make_dungeon1(VoxelManipulator &vmanip, PseudoRandom &random,
		INodeDefManager *ndef)
{
	v3s16 areasize = vmanip.m_area.getExtent();
	v3s16 roomsize;
	v3s16 roomplace;
	
	/*
		Find place for first room
	*/
	bool fits = false;
	for(u32 i=0; i<100; i++)
	{
		roomsize = v3s16(random.range(4,8),random.range(4,6),random.range(4,8));
		roomplace = vmanip.m_area.MinEdge + v3s16(
				random.range(0,areasize.X-roomsize.X-1),
				random.range(0,areasize.Y-roomsize.Y-1),
				random.range(0,areasize.Z-roomsize.Z-1));
		/*
			Check that we're not putting the room to an unknown place,
			otherwise it might end up floating in the air
		*/
		fits = true;
		for(s16 z=1; z<roomsize.Z-1; z++)
		for(s16 y=1; y<roomsize.Y-1; y++)
		for(s16 x=1; x<roomsize.X-1; x++)
		{
			v3s16 p = roomplace + v3s16(x,y,z);
			u32 vi = vmanip.m_area.index(p);
			if(vmanip.m_flags[vi] & VMANIP_FLAG_DUNGEON_INSIDE)
			{
				fits = false;
				break;
			}
			if(vmanip.m_data[vi].getContent() == CONTENT_IGNORE)
			{
				fits = false;
				break;
			}
		}
		if(fits)
			break;
	}
	// No place found
	if(fits == false)
		return;
	
	/*
		Stores the center position of the last room made, so that
		a new corridor can be started from the last room instead of
		the new room, if chosen so.
	*/
	v3s16 last_room_center = roomplace+v3s16(roomsize.X/2,1,roomsize.Z/2);
	
	u32 room_count = random.range(2,7);
	for(u32 i=0; i<room_count; i++)
	{
		// Make a room to the determined place
		make_room1(vmanip, roomsize, roomplace, ndef);
		
		v3s16 room_center = roomplace + v3s16(roomsize.X/2,1,roomsize.Z/2);

		// Place torch at room center (for testing)
		//vmanip.m_data[vmanip.m_area.index(room_center)] = MapNode(ndef->getId("mapgen_torch"));

		// Quit if last room
		if(i == room_count-1)
			break;
		
		// Determine walker start position

		bool start_in_last_room = (random.range(0,2)!=0);
		//bool start_in_last_room = true;

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
		RoomWalker walker(vmanip, walker_start_place, random, ndef);
		v3s16 doorplace;
		v3s16 doordir;
		bool r = walker.findPlaceForDoor(doorplace, doordir);
		if(r == false)
			return;
		
		if(random.range(0,1)==0)
			// Make the door
			make_door1(vmanip, doorplace, doordir, ndef);
		else
			// Don't actually make a door
			doorplace -= doordir;
		
		// Make a random corridor starting from the door
		v3s16 corridor_end;
		v3s16 corridor_end_dir;
		make_corridor(vmanip, doorplace, doordir, corridor_end,
				corridor_end_dir, random, ndef);
		
		// Find a place for a random sized room
		roomsize = v3s16(random.range(4,8),random.range(4,6),random.range(4,8));
		walker.setPos(corridor_end);
		walker.setDir(corridor_end_dir);
		r = walker.findPlaceForRoomDoor(roomsize, doorplace, doordir, roomplace);
		if(r == false)
			return;

		if(random.range(0,1)==0)
			// Make the door
			make_door1(vmanip, doorplace, doordir, ndef);
		else
			// Don't actually make a door
			roomplace -= doordir;
		
	}
}
#endif

#if 0
static void make_nc(VoxelManipulator &vmanip, PseudoRandom &random,
		INodeDefManager *ndef)
{
	v3s16 dir;
	u8 facedir_i = 0;
	s32 r = random.range(0, 3);
	if(r == 0){
		dir = v3s16( 1, 0, 0);
		facedir_i = 3;
	}
	if(r == 1){
		dir = v3s16(-1, 0, 0);
		facedir_i = 1;
	}
	if(r == 2){
		dir = v3s16( 0, 0, 1);
		facedir_i = 2;
	}
	if(r == 3){
		dir = v3s16( 0, 0,-1);
		facedir_i = 0;
	}
	v3s16 p = vmanip.m_area.MinEdge + v3s16(
			16+random.range(0,15),
			16+random.range(0,15),
			16+random.range(0,15));
	vmanip.m_data[vmanip.m_area.index(p)] = MapNode(ndef->getId("mapgen_nyancat"), facedir_i);
	u32 length = random.range(3,15);
	for(u32 j=0; j<length; j++)
	{
		p -= dir;
		vmanip.m_data[vmanip.m_area.index(p)] = MapNode(ndef->getId("mapgen_nyancat_rainbow"));
	}
}
#endif

/*
	Noise functions. Make sure seed is mangled differently in each one.
*/

#if 0
/*
	Scaling the output of the noise function affects the overdrive of the
	contour function, which affects the shape of the output considerably.
*/
#define CAVE_NOISE_SCALE 12.0
//#define CAVE_NOISE_SCALE 10.0
//#define CAVE_NOISE_SCALE 7.5
//#define CAVE_NOISE_SCALE 5.0
//#define CAVE_NOISE_SCALE 1.0

//#define CAVE_NOISE_THRESHOLD (2.5/CAVE_NOISE_SCALE)
#define CAVE_NOISE_THRESHOLD (1.5/CAVE_NOISE_SCALE)

NoiseParams get_cave_noise1_params(u64 seed)
{
	/*return NoiseParams(NOISE_PERLIN_CONTOUR, seed+52534, 5, 0.7,
			200, CAVE_NOISE_SCALE);*/
	/*return NoiseParams(NOISE_PERLIN_CONTOUR, seed+52534, 4, 0.7,
			100, CAVE_NOISE_SCALE);*/
	/*return NoiseParams(NOISE_PERLIN_CONTOUR, seed+52534, 5, 0.6,
			100, CAVE_NOISE_SCALE);*/
	/*return NoiseParams(NOISE_PERLIN_CONTOUR, seed+52534, 5, 0.3,
			100, CAVE_NOISE_SCALE);*/
	return NoiseParams(NOISE_PERLIN_CONTOUR, seed+52534, 4, 0.5,
			50, CAVE_NOISE_SCALE);
	//return NoiseParams(NOISE_CONSTANT_ONE);
}

NoiseParams get_cave_noise2_params(u64 seed)
{
	/*return NoiseParams(NOISE_PERLIN_CONTOUR_FLIP_YZ, seed+10325, 5, 0.7,
			200, CAVE_NOISE_SCALE);*/
	/*return NoiseParams(NOISE_PERLIN_CONTOUR_FLIP_YZ, seed+10325, 4, 0.7,
			100, CAVE_NOISE_SCALE);*/
	/*return NoiseParams(NOISE_PERLIN_CONTOUR_FLIP_YZ, seed+10325, 5, 0.3,
			100, CAVE_NOISE_SCALE);*/
	return NoiseParams(NOISE_PERLIN_CONTOUR_FLIP_YZ, seed+10325, 4, 0.5,
			50, CAVE_NOISE_SCALE);
	//return NoiseParams(NOISE_CONSTANT_ONE);
}

NoiseParams get_ground_noise1_params(u64 seed)
{
	return NoiseParams(NOISE_PERLIN, seed+983240, 4,
			0.55, 80.0, 40.0);
}

NoiseParams get_ground_crumbleness_params(u64 seed)
{
	return NoiseParams(NOISE_PERLIN, seed+34413, 3,
			1.3, 20.0, 1.0);
}

NoiseParams get_ground_wetness_params(u64 seed)
{
	return NoiseParams(NOISE_PERLIN, seed+32474, 4,
			1.1, 40.0, 1.0);
}

bool is_cave(u64 seed, v3s16 p)
{
	double d1 = noise3d_param(get_cave_noise1_params(seed), p.X,p.Y,p.Z);
	double d2 = noise3d_param(get_cave_noise2_params(seed), p.X,p.Y,p.Z);
	return d1*d2 > CAVE_NOISE_THRESHOLD;
}

/*
	Ground density noise shall be interpreted by using this.

	TODO: No perlin noises here, they should be outsourced
	      and buffered
		  NOTE: The speed of these actually isn't terrible
*/
bool val_is_ground(double ground_noise1_val, v3s16 p, u64 seed)
{
	//return ((double)p.Y < ground_noise1_val);

	double f = 0.55 + noise2d_perlin(
			0.5+(float)p.X/250, 0.5+(float)p.Z/250,
			seed+920381, 3, 0.45);
	if(f < 0.01)
		f = 0.01;
	else if(f >= 1.0)
		f *= 1.6;
	double h = WATER_LEVEL + 10 * noise2d_perlin(
			0.5+(float)p.X/250, 0.5+(float)p.Z/250,
			seed+84174, 4, 0.5);
	/*double f = 1;
	double h = 0;*/
	return ((double)p.Y - h < ground_noise1_val * f);
}

/*
	Queries whether a position is ground or not.
*/
bool is_ground(u64 seed, v3s16 p)
{
	double val1 = noise3d_param(get_ground_noise1_params(seed), p.X,p.Y,p.Z);
	return val_is_ground(val1, p, seed);
}
#endif

// Amount of trees per area in nodes
double tree_amount_2d(u64 seed, v2s16 p)
{
	/*double noise = noise2d_perlin(
			0.5+(float)p.X/250, 0.5+(float)p.Y/250,
			seed+2, 5, 0.66);*/
	double noise = noise2d_perlin(
			0.5+(float)p.X/125, 0.5+(float)p.Y/125,
			seed+2, 4, 0.66);
	double zeroval = -0.39;
	if(noise < zeroval)
		return 0;
	else
		return 0.04 * (noise-zeroval) / (1.0-zeroval);
}

#if 0
double surface_humidity_2d(u64 seed, v2s16 p)
{
	double noise = noise2d_perlin(
			0.5+(float)p.X/500, 0.5+(float)p.Y/500,
			seed+72384, 4, 0.66);
	noise = (noise + 1.0)/2.0;
	if(noise < 0.0)
		noise = 0.0;
	if(noise > 1.0)
		noise = 1.0;
	return noise;
}

/*
	Incrementally find ground level from 3d noise
*/
s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision)
{
	// Start a bit fuzzy to make averaging lower precision values
	// more useful
	s16 level = myrand_range(-precision/2, precision/2);
	s16 dec[] = {31000, 100, 20, 4, 1, 0};
	s16 i;
	for(i = 1; dec[i] != 0 && precision <= dec[i]; i++)
	{
		// First find non-ground by going upwards
		// Don't stop in caves.
		{
			s16 max = level+dec[i-1]*2;
			v3s16 p(p2d.X, level, p2d.Y);
			for(; p.Y < max; p.Y += dec[i])
			{
				if(!is_ground(seed, p))
				{
					level = p.Y;
					break;
				}
			}
		}
		// Then find ground by going downwards from there.
		// Go in caves, too, when precision is 1.
		{
			s16 min = level-dec[i-1]*2;
			v3s16 p(p2d.X, level, p2d.Y);
			for(; p.Y>min; p.Y-=dec[i])
			{
				bool ground = is_ground(seed, p);
				/*if(dec[i] == 1 && is_cave(seed, p))
					ground = false;*/
				if(ground)
				{
					level = p.Y;
					break;
				}
			}
		}
	}
	
	// This is more like the actual ground level
	level += dec[i-1]/2;

	return level;
}

double get_sector_average_ground_level(u64 seed, v2s16 sectorpos, double p=4);

double get_sector_average_ground_level(u64 seed, v2s16 sectorpos, double p)
{
	v2s16 node_min = sectorpos*MAP_BLOCKSIZE;
	v2s16 node_max = (sectorpos+v2s16(1,1))*MAP_BLOCKSIZE-v2s16(1,1);
	double a = 0;
	a += find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y), p);
	a += find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_max.Y), p);
	a += find_ground_level_from_noise(seed,
			v2s16(node_max.X, node_max.Y), p);
	a += find_ground_level_from_noise(seed,
			v2s16(node_max.X, node_min.Y), p);
	a += find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_min.Y+MAP_BLOCKSIZE/2), p);
	a /= 5;
	return a;
}

double get_sector_maximum_ground_level(u64 seed, v2s16 sectorpos, double p=4);

double get_sector_maximum_ground_level(u64 seed, v2s16 sectorpos, double p)
{
	v2s16 node_min = sectorpos*MAP_BLOCKSIZE;
	v2s16 node_max = (sectorpos+v2s16(1,1))*MAP_BLOCKSIZE-v2s16(1,1);
	double a = -31000;
	// Corners
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y), p));
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_max.Y), p));
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_max.X, node_max.Y), p));
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y), p));
	// Center
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_min.Y+MAP_BLOCKSIZE/2), p));
	// Side middle points
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_min.Y), p));
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_max.Y), p));
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y+MAP_BLOCKSIZE/2), p));
	a = MYMAX(a, find_ground_level_from_noise(seed,
			v2s16(node_max.X, node_min.Y+MAP_BLOCKSIZE/2), p));
	return a;
}

double get_sector_minimum_ground_level(u64 seed, v2s16 sectorpos, double p=4);

double get_sector_minimum_ground_level(u64 seed, v2s16 sectorpos, double p)
{
	v2s16 node_min = sectorpos*MAP_BLOCKSIZE;
	v2s16 node_max = (sectorpos+v2s16(1,1))*MAP_BLOCKSIZE-v2s16(1,1);
	double a = 31000;
	// Corners
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y), p));
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_max.Y), p));
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_max.X, node_max.Y), p));
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y), p));
	// Center
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_min.Y+MAP_BLOCKSIZE/2), p));
	// Side middle points
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_min.Y), p));
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X+MAP_BLOCKSIZE/2, node_max.Y), p));
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_min.X, node_min.Y+MAP_BLOCKSIZE/2), p));
	a = MYMIN(a, find_ground_level_from_noise(seed,
			v2s16(node_max.X, node_min.Y+MAP_BLOCKSIZE/2), p));
	return a;
}
#endif

// Required by mapgen.h
bool block_is_underground(u64 seed, v3s16 blockpos)
{
	/*s16 minimum_groundlevel = (s16)get_sector_minimum_ground_level(
			seed, v2s16(blockpos.X, blockpos.Z));*/
	// Nah, this is just a heuristic, just return something
	s16 minimum_groundlevel = WATER_LEVEL;
	
	if(blockpos.Y*MAP_BLOCKSIZE + MAP_BLOCKSIZE <= minimum_groundlevel)
		return true;
	else
		return false;
}

#define AVERAGE_MUD_AMOUNT 4

double base_rock_level_2d(u64 seed, v2s16 p)
{
	// The base ground level
	double base = (double)WATER_LEVEL - (double)AVERAGE_MUD_AMOUNT
			+ 20. * noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			seed+82341, 5, 0.6);

	/*// A bit hillier one
	double base2 = WATER_LEVEL - 4.0 + 40. * noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			seed+93413, 6, 0.69);
	if(base2 > base)
		base = base2;*/
#if 1
	// Higher ground level
	double higher = (double)WATER_LEVEL + 20. + 16. * noise2d_perlin(
			0.5+(float)p.X/500., 0.5+(float)p.Y/500.,
			seed+85039, 5, 0.6);
	//higher = 30; // For debugging

	// Limit higher to at least base
	if(higher < base)
		higher = base;

	// Steepness factor of cliffs
	double b = 0.85 + 0.5 * noise2d_perlin(
			0.5+(float)p.X/125., 0.5+(float)p.Y/125.,
			seed-932, 5, 0.7);
	b = rangelim(b, 0.0, 1000.0);
	b = pow(b, 7);
	b *= 5;
	b = rangelim(b, 0.5, 1000.0);
	// Values 1.5...100 give quite horrible looking slopes
	if(b > 1.5 && b < 100.0){
		if(b < 10.0)
			b = 1.5;
		else
			b = 100.0;
	}
	//dstream<<"b="<<b<<std::endl;
	//double b = 20;
	//b = 0.25;

	// Offset to more low
	double a_off = -0.20;
	// High/low selector
	/*double a = 0.5 + b * (a_off + noise2d_perlin(
			0.5+(float)p.X/500., 0.5+(float)p.Y/500.,
			seed+4213, 6, 0.7));*/
	double a = (double)0.5 + b * (a_off + noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			seed+4213, 5, 0.69));
	// Limit
	a = rangelim(a, 0.0, 1.0);

	//dstream<<"a="<<a<<std::endl;

	double h = base*(1.0-a) + higher*a;
#else
	double h = base;
#endif
	return h;
}

s16 find_ground_level_from_noise(u64 seed, v2s16 p2d, s16 precision)
{
	return base_rock_level_2d(seed, p2d) + AVERAGE_MUD_AMOUNT;
}

double get_mud_add_amount(u64 seed, v2s16 p)
{
	return ((float)AVERAGE_MUD_AMOUNT + 2.0 * noise2d_perlin(
			0.5+(float)p.X/200, 0.5+(float)p.Y/200,
			seed+91013, 3, 0.55));
}

bool get_have_beach(u64 seed, v2s16 p2d)
{
	// Determine whether to have sand here
	double sandnoise = noise2d_perlin(
			0.2+(float)p2d.X/250, 0.7+(float)p2d.Y/250,
			seed+59420, 3, 0.50);

	return (sandnoise > 0.15);
}

enum BiomeType
{
	BT_NORMAL,
	BT_DESERT
};

BiomeType get_biome(u64 seed, v2s16 p2d)
{
	// Just do something very simple as for now
	double d = noise2d_perlin(
			0.6+(float)p2d.X/250, 0.2+(float)p2d.Y/250,
			seed+9130, 3, 0.50);
	if(d > 0.45) 
		return BT_DESERT;
	if(d > 0.35 && (noise2d( p2d.X, p2d.Y, int(seed) ) + 1.0) > ( 0.45 - d ) * 20.0  ) 
		return BT_DESERT;
	return BT_NORMAL;
};

u32 get_blockseed(u64 seed, v3s16 p)
{
	s32 x=p.X, y=p.Y, z=p.Z;
	return (u32)(seed%0x100000000ULL) + z*38134234 + y*42123 + x*23;
}

#define VMANIP_FLAG_CAVE VOXELFLAG_CHECKED1

void make_block(BlockMakeData *data)
{
	if(data->no_op)
	{
		//dstream<<"makeBlock: no-op"<<std::endl;
		return;
	}

	assert(data->vmanip);
	assert(data->nodedef);
	assert(data->blockpos_requested.X >= data->blockpos_min.X &&
			data->blockpos_requested.Y >= data->blockpos_min.Y &&
			data->blockpos_requested.Z >= data->blockpos_min.Z);
	assert(data->blockpos_requested.X <= data->blockpos_max.X &&
			data->blockpos_requested.Y <= data->blockpos_max.Y &&
			data->blockpos_requested.Z <= data->blockpos_max.Z);

	INodeDefManager *ndef = data->nodedef;

	// Hack: use minimum block coordinates for old code that assumes
	// a single block
	v3s16 blockpos = data->blockpos_requested;
	
	/*dstream<<"makeBlock(): ("<<blockpos.X<<","<<blockpos.Y<<","
			<<blockpos.Z<<")"<<std::endl;*/

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	v3s16 blockpos_full_min = blockpos_min - v3s16(1,1,1);
	v3s16 blockpos_full_max = blockpos_max + v3s16(1,1,1);
	
	ManualMapVoxelManipulator &vmanip = *(data->vmanip);
	// Area of central chunk
	v3s16 node_min = blockpos_min*MAP_BLOCKSIZE;
	v3s16 node_max = (blockpos_max+v3s16(1,1,1))*MAP_BLOCKSIZE-v3s16(1,1,1);
	// Full allocated area
	v3s16 full_node_min = (blockpos_min-1)*MAP_BLOCKSIZE;
	v3s16 full_node_max = (blockpos_max+2)*MAP_BLOCKSIZE-v3s16(1,1,1);

	v3s16 central_area_size = node_max - node_min + v3s16(1,1,1);

	const s16 max_spread_amount = MAP_BLOCKSIZE;

	int volume_blocks = (blockpos_max.X - blockpos_min.X + 1)
			* (blockpos_max.Y - blockpos_min.Y + 1)
			* (blockpos_max.Z - blockpos_max.Z + 1);
	
	int volume_nodes = volume_blocks *
			MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
	
	// Generated surface area
	//double gen_area_nodes = MAP_BLOCKSIZE*MAP_BLOCKSIZE * rel_volume;

	// Horribly wrong heuristic, but better than nothing
	bool block_is_underground = (WATER_LEVEL > node_max.Y);

	/*
		Create a block-specific seed
	*/
	u32 blockseed = get_blockseed(data->seed, full_node_min);
	
	/*
		Cache some ground type values for speed
	*/

// Creates variables c_name=id and n_name=node
#define CONTENT_VARIABLE(ndef, name)\
	content_t c_##name = ndef->getId("mapgen_" #name);\
	MapNode n_##name(c_##name);
// Default to something else if was CONTENT_IGNORE
#define CONTENT_VARIABLE_FALLBACK(name, dname)\
	if(c_##name == CONTENT_IGNORE){\
		c_##name = c_##dname;\
		n_##name = n_##dname;\
	}

	CONTENT_VARIABLE(ndef, stone);
	CONTENT_VARIABLE(ndef, air);
	CONTENT_VARIABLE(ndef, water_source);
	CONTENT_VARIABLE(ndef, dirt);
	CONTENT_VARIABLE(ndef, sand);
	CONTENT_VARIABLE(ndef, gravel);
	CONTENT_VARIABLE(ndef, clay);
	CONTENT_VARIABLE(ndef, lava_source);
	CONTENT_VARIABLE(ndef, cobble);
	CONTENT_VARIABLE(ndef, mossycobble);
	CONTENT_VARIABLE(ndef, dirt_with_grass);
	CONTENT_VARIABLE(ndef, junglegrass);
	CONTENT_VARIABLE(ndef, stone_with_coal);
	CONTENT_VARIABLE(ndef, stone_with_iron);
	CONTENT_VARIABLE(ndef, mese);
	CONTENT_VARIABLE(ndef, desert_sand);
	CONTENT_VARIABLE_FALLBACK(desert_sand, sand);
	CONTENT_VARIABLE(ndef, desert_stone);
	CONTENT_VARIABLE_FALLBACK(desert_stone, stone);

	// Maximum height of the stone surface and obstacles.
	// This is used to guide the cave generation
	s16 stone_surface_max_y = 0;

	/*
		Generate general ground level to full area
	*/
	{
#if 1
	TimeTaker timer1("Generating ground level");
	
	for(s16 x=node_min.X; x<=node_max.X; x++)
	for(s16 z=node_min.Z; z<=node_max.Z; z++)
	{
		// Node position
		v2s16 p2d = v2s16(x,z);
		
		/*
			Skip of already generated
		*/
		/*{
			v3s16 p(p2d.X, node_min.Y, p2d.Y);
			if(vmanip.m_data[vmanip.m_area.index(p)].d != CONTENT_AIR)
				continue;
		}*/

		// Ground height at this point
		float surface_y_f = 0.0;

		// Use perlin noise for ground height
		surface_y_f = base_rock_level_2d(data->seed, p2d);
		
		/*// Experimental stuff
		{
			float a = highlands_level_2d(data->seed, p2d);
			if(a > surface_y_f)
				surface_y_f = a;
		}*/

		// Convert to integer
		s16 surface_y = (s16)surface_y_f;
		
		// Log it
		if(surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;

		BiomeType bt = get_biome(data->seed, p2d);
		/*
			Fill ground with stone
		*/
		{
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_min.Y, p2d.Y));
			for(s16 y=node_min.Y; y<=node_max.Y; y++)
			{
				if(vmanip.m_data[i].getContent() == CONTENT_IGNORE){
					if(y <= surface_y){
						if(y > WATER_LEVEL && bt == BT_DESERT)
							vmanip.m_data[i] = n_desert_stone;
						else
							vmanip.m_data[i] = n_stone;
					} else if(y <= WATER_LEVEL){
						vmanip.m_data[i] = MapNode(c_water_source);
					} else {
						vmanip.m_data[i] = MapNode(c_air);
					}
				}
				vmanip.m_area.add_y(em, i, 1);
			}
		}
	}
#endif
	
	}//timer1
	
	// Limit dirt flow area by 1 because mud is flown into neighbors.
	assert(central_area_size.X == central_area_size.Z);
	s16 mudflow_minpos = 0-max_spread_amount+1;
	s16 mudflow_maxpos = central_area_size.X+max_spread_amount-2;

	/*
		Loop this part, it will make stuff look older and newer nicely
	*/

	const u32 age_loops = 2;
	for(u32 i_age=0; i_age<age_loops; i_age++)
	{ // Aging loop
	/******************************
		BEGINNING OF AGING LOOP
	******************************/

#if 1
	{
	// 24ms @cs=8
	//TimeTaker timer1("caves");

	/*
		Make caves (this code is relatively horrible)
	*/
	double cave_amount = 6.0 + 6.0 * noise2d_perlin(
			0.5+(double)node_min.X/250, 0.5+(double)node_min.Y/250,
			data->seed+34329, 3, 0.50);
	cave_amount = MYMAX(0.0, cave_amount);
	u32 caves_count = cave_amount * volume_nodes / 50000;
	u32 bruises_count = 1;
	PseudoRandom ps(blockseed+21343);
	PseudoRandom ps2(blockseed+1032);
	if(ps.range(1, 6) == 1)
		bruises_count = ps.range(0, ps.range(0, 2));
	if(get_biome(data->seed, v2s16(node_min.X, node_min.Y)) == BT_DESERT){
		caves_count /= 3;
		bruises_count /= 3;
	}
	for(u32 jj=0; jj<caves_count+bruises_count; jj++)
	{
		bool large_cave = (jj >= caves_count);
		s16 min_tunnel_diameter = 2;
		s16 max_tunnel_diameter = ps.range(2,6);
		int dswitchint = ps.range(1,14);
		u16 tunnel_routepoints = 0;
		int part_max_length_rs = 0;
		if(large_cave){
			part_max_length_rs = ps.range(2,4);
			tunnel_routepoints = ps.range(5, ps.range(15,30));
			min_tunnel_diameter = 5;
			max_tunnel_diameter = ps.range(7, ps.range(8,24));
		} else {
			part_max_length_rs = ps.range(2,9);
			tunnel_routepoints = ps.range(10, ps.range(15,30));
		}
		bool large_cave_is_flat = (ps.range(0,1) == 0);
		
		v3f main_direction(0,0,0);

		// Allowed route area size in nodes
		v3s16 ar = central_area_size;

		// Area starting point in nodes
		v3s16 of = node_min;

		// Allow a bit more
		//(this should be more than the maximum radius of the tunnel)
		//s16 insure = 5; // Didn't work with max_d = 20
		s16 insure = 10;
		s16 more = max_spread_amount - max_tunnel_diameter/2 - insure;
		ar += v3s16(1,0,1) * more * 2;
		of -= v3s16(1,0,1) * more;
		
		s16 route_y_min = 0;
		// Allow half a diameter + 7 over stone surface
		s16 route_y_max = -of.Y + stone_surface_max_y + max_tunnel_diameter/2 + 7;

		/*// If caves, don't go through surface too often
		if(large_cave == false)
			route_y_max -= ps.range(0, max_tunnel_diameter*2);*/

		// Limit maximum to area
		route_y_max = rangelim(route_y_max, 0, ar.Y-1);

		if(large_cave)
		{
			/*// Minimum is at y=0
			route_y_min = -of.Y - 0;*/
			// Minimum is at y=max_tunnel_diameter/4
			//route_y_min = -of.Y + max_tunnel_diameter/4;
			//s16 min = -of.Y + max_tunnel_diameter/4;
			//s16 min = -of.Y + 0;
			s16 min = 0;
			if(node_min.Y < WATER_LEVEL && node_max.Y > WATER_LEVEL)
			{
				min = WATER_LEVEL - max_tunnel_diameter/3 - of.Y;
				route_y_max = WATER_LEVEL + max_tunnel_diameter/3 - of.Y;
			}
			route_y_min = ps.range(min, min + max_tunnel_diameter);
			route_y_min = rangelim(route_y_min, 0, route_y_max);
		}

		/*dstream<<"route_y_min = "<<route_y_min
				<<", route_y_max = "<<route_y_max<<std::endl;*/

		s16 route_start_y_min = route_y_min;
		s16 route_start_y_max = route_y_max;

		// Start every 4th cave from surface when applicable
		/*bool coming_from_surface = false;
		if(node_min.Y <= 0 && node_max.Y >= 0){
			coming_from_surface = (jj % 4 == 0 && large_cave == false);
			if(coming_from_surface)
				route_start_y_min = -of.Y + stone_surface_max_y + 10;
		}*/
		
		route_start_y_min = rangelim(route_start_y_min, 0, ar.Y-1);
		route_start_y_max = rangelim(route_start_y_max, route_start_y_min, ar.Y-1);

		// Randomize starting position
		v3f orp(
			(float)(ps.next()%ar.X)+0.5,
			(float)(ps.range(route_start_y_min, route_start_y_max))+0.5,
			(float)(ps.next()%ar.Z)+0.5
		);

		v3s16 startp(orp.X, orp.Y, orp.Z);
		startp += of;

		MapNode airnode(CONTENT_AIR);
		MapNode waternode(c_water_source);
		MapNode lavanode(c_lava_source);
		
		/*
			Generate some tunnel starting from orp
		*/
		
		for(u16 j=0; j<tunnel_routepoints; j++)
		{
			if(j%dswitchint==0 && large_cave == false)
			{
				main_direction = v3f(
					((float)(ps.next()%20)-(float)10)/10,
					((float)(ps.next()%20)-(float)10)/30,
					((float)(ps.next()%20)-(float)10)/10
				);
				main_direction *= (float)ps.range(0, 10)/10;
			}
			
			// Randomize size
			s16 min_d = min_tunnel_diameter;
			s16 max_d = max_tunnel_diameter;
			s16 rs = ps.range(min_d, max_d);
			
			// Every second section is rough
			bool randomize_xz = (ps2.range(1,2) == 1);

			v3s16 maxlen;
			if(large_cave)
			{
				maxlen = v3s16(
					rs*part_max_length_rs,
					rs*part_max_length_rs/2,
					rs*part_max_length_rs
				);
			}
			else
			{
				maxlen = v3s16(
					rs*part_max_length_rs,
					ps.range(1, rs*part_max_length_rs),
					rs*part_max_length_rs
				);
			}

			v3f vec;
			
			vec = v3f(
				(float)(ps.next()%(maxlen.X*1))-(float)maxlen.X/2,
				(float)(ps.next()%(maxlen.Y*1))-(float)maxlen.Y/2,
				(float)(ps.next()%(maxlen.Z*1))-(float)maxlen.Z/2
			);
		
			// Jump downward sometimes
			if(!large_cave && ps.range(0,12) == 0)
			{
				vec = v3f(
					(float)(ps.next()%(maxlen.X*1))-(float)maxlen.X/2,
					(float)(ps.next()%(maxlen.Y*2))-(float)maxlen.Y*2/2,
					(float)(ps.next()%(maxlen.Z*1))-(float)maxlen.Z/2
				);
			}
			
			/*if(large_cave){
				v3f p = orp + vec;
				s16 h = find_ground_level_clever(vmanip,
						v2s16(p.X, p.Z), ndef);
				route_y_min = h - rs/3;
				route_y_max = h + rs;
			}*/

			vec += main_direction;

			v3f rp = orp + vec;
			if(rp.X < 0)
				rp.X = 0;
			else if(rp.X >= ar.X)
				rp.X = ar.X-1;
			if(rp.Y < route_y_min)
				rp.Y = route_y_min;
			else if(rp.Y >= route_y_max)
				rp.Y = route_y_max-1;
			if(rp.Z < 0)
				rp.Z = 0;
			else if(rp.Z >= ar.Z)
				rp.Z = ar.Z-1;
			vec = rp - orp;

			for(float f=0; f<1.0; f+=1.0/vec.getLength())
			{
				v3f fp = orp + vec * f;
				fp.X += 0.1*ps.range(-10,10);
				fp.Z += 0.1*ps.range(-10,10);
				v3s16 cp(fp.X, fp.Y, fp.Z);

				s16 d0 = -rs/2;
				s16 d1 = d0 + rs;
				if(randomize_xz){
					d0 += ps.range(-1,1);
					d1 += ps.range(-1,1);
				}
				for(s16 z0=d0; z0<=d1; z0++)
				{
					s16 si = rs/2 - MYMAX(0, abs(z0)-rs/7-1);
					for(s16 x0=-si-ps.range(0,1); x0<=si-1+ps.range(0,1); x0++)
					{
						s16 maxabsxz = MYMAX(abs(x0), abs(z0));
						s16 si2 = rs/2 - MYMAX(0, maxabsxz-rs/7-1);
						for(s16 y0=-si2; y0<=si2; y0++)
						{
							/*// Make better floors in small caves
							if(y0 <= -rs/2 && rs<=7)
								continue;*/
							if(large_cave_is_flat){
								// Make large caves not so tall
								if(rs > 7 && abs(y0) >= rs/3)
									continue;
							}

							s16 z = cp.Z + z0;
							s16 y = cp.Y + y0;
							s16 x = cp.X + x0;
							v3s16 p(x,y,z);
							p += of;
							
							if(vmanip.m_area.contains(p) == false)
								continue;
							
							u32 i = vmanip.m_area.index(p);
							
							if(large_cave)
							{
								if(full_node_min.Y < WATER_LEVEL &&
									full_node_max.Y > WATER_LEVEL){
									if(p.Y <= WATER_LEVEL)
										vmanip.m_data[i] = waternode;
									else
										vmanip.m_data[i] = airnode;
								} else if(full_node_max.Y < WATER_LEVEL){
									if(p.Y < startp.Y - 2)
										vmanip.m_data[i] = lavanode;
									else
										vmanip.m_data[i] = airnode;
								} else {
									vmanip.m_data[i] = airnode;
								}
							} else {
								// Don't replace air or water or lava or ignore
								if(vmanip.m_data[i].getContent() == CONTENT_IGNORE ||
								vmanip.m_data[i].getContent() == CONTENT_AIR ||
								vmanip.m_data[i].getContent() == c_water_source ||
								vmanip.m_data[i].getContent() == c_lava_source)
									continue;
								
								vmanip.m_data[i] = airnode;

								// Set tunnel flag
								vmanip.m_flags[i] |= VMANIP_FLAG_CAVE;
							}
						}
					}
				}
			}

			orp = rp;
		}
	
	}

	}//timer1
#endif
	
#if 1
	{
	// 15ms @cs=8
	TimeTaker timer1("add mud");

	/*
		Add mud to the central chunk
	*/
	
	for(s16 x=node_min.X; x<=node_max.X; x++)
	for(s16 z=node_min.Z; z<=node_max.Z; z++)
	{
		// Node position in 2d
		v2s16 p2d = v2s16(x,z);
		
		// Randomize mud amount
		s16 mud_add_amount = get_mud_add_amount(data->seed, p2d) / 2.0 + 0.5;

		// Find ground level
		s16 surface_y = find_stone_level(vmanip, p2d, ndef);
		// Handle area not found
		if(surface_y == vmanip.m_area.MinEdge.Y - 1)
			continue;

		MapNode addnode(c_dirt);
		BiomeType bt = get_biome(data->seed, p2d);

		if(bt == BT_DESERT)
			addnode = MapNode(c_desert_sand);

		if(bt == BT_DESERT && surface_y + mud_add_amount <= WATER_LEVEL+1){
			addnode = MapNode(c_sand);
		} else if(mud_add_amount <= 0){
			mud_add_amount = 1 - mud_add_amount;
			addnode = MapNode(c_gravel);
		} else if(bt == BT_NORMAL && get_have_beach(data->seed, p2d) &&
				surface_y + mud_add_amount <= WATER_LEVEL+2){
			addnode = MapNode(c_sand);
		}
		
		if(bt == BT_DESERT){
			if(surface_y > 20){
				mud_add_amount = MYMAX(0, mud_add_amount - (surface_y - 20)/5);
			}
		}

		/*
			If topmost node is grass, change it to mud.
			It might be if it was flown to there from a neighboring
			chunk and then converted.
		*/
		{
			u32 i = vmanip.m_area.index(v3s16(p2d.X, surface_y, p2d.Y));
			MapNode *n = &vmanip.m_data[i];
			if(n->getContent() == c_dirt_with_grass)
				*n = MapNode(c_dirt);
		}

		/*
			Add mud on ground
		*/
		{
			s16 mudcount = 0;
			v3s16 em = vmanip.m_area.getExtent();
			s16 y_start = surface_y+1;
			u32 i = vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			for(s16 y=y_start; y<=node_max.Y; y++)
			{
				if(mudcount >= mud_add_amount)
					break;
					
				MapNode &n = vmanip.m_data[i];
				n = addnode;
				mudcount++;

				vmanip.m_area.add_y(em, i, 1);
			}
		}

	}

	}//timer1
#endif

	/*
		Add blobs of dirt and gravel underground
	*/
	if(get_biome(data->seed, v2s16(node_min.X, node_min.Y)) == BT_NORMAL)
	{
	PseudoRandom pr(blockseed+983);
	for(int i=0; i<volume_nodes/10/10/10; i++)
	{
		bool only_fill_cave = (myrand_range(0,1) != 0);
		v3s16 size(
			pr.range(1, 8),
			pr.range(1, 8),
			pr.range(1, 8)
		);
		v3s16 p0(
			pr.range(node_min.X, node_max.X)-size.X/2,
			pr.range(node_min.Y, node_max.Y)-size.Y/2,
			pr.range(node_min.Z, node_max.Z)-size.Z/2
		);
		MapNode n1;
		if(p0.Y > -32 && pr.range(0,1) == 0)
			n1 = MapNode(c_dirt);
		else
			n1 = MapNode(c_gravel);
		for(int x1=0; x1<size.X; x1++)
		for(int y1=0; y1<size.Y; y1++)
		for(int z1=0; z1<size.Z; z1++)
		{
			v3s16 p = p0 + v3s16(x1,y1,z1);
			u32 i = vmanip.m_area.index(p);
			if(!vmanip.m_area.contains(i))
				continue;
			// Cancel if not stone and not cave air
			if(vmanip.m_data[i].getContent() != c_stone &&
					!(vmanip.m_flags[i] & VMANIP_FLAG_CAVE))
				continue;
			if(only_fill_cave && !(vmanip.m_flags[i] & VMANIP_FLAG_CAVE))
				continue;
			vmanip.m_data[i] = n1;
		}
	}
	}

#if 1
	{
	// 340ms @cs=8
	TimeTaker timer1("flow mud");

	/*
		Flow mud away from steep edges
	*/
	
	// Iterate a few times
	for(s16 k=0; k<3; k++)
	{

	for(s16 x=mudflow_minpos; x<=mudflow_maxpos; x++)
	for(s16 z=mudflow_minpos; z<=mudflow_maxpos; z++)
	{
		// Invert coordinates every 2nd iteration
		if(k%2 == 0)
		{
			x = mudflow_maxpos - (x-mudflow_minpos);
			z = mudflow_maxpos - (z-mudflow_minpos);
		}

		// Node position in 2d
		v2s16 p2d = v2s16(node_min.X, node_min.Z) + v2s16(x,z);
		
		v3s16 em = vmanip.m_area.getExtent();
		u32 i = vmanip.m_area.index(v3s16(p2d.X, node_max.Y, p2d.Y));
		s16 y=node_max.Y;

		while(y >= node_min.Y)
		{

		for(;; y--)
		{
			MapNode *n = NULL;
			// Find mud
			for(; y>=node_min.Y; y--)
			{
				n = &vmanip.m_data[i];
				//if(content_walkable(n->d))
				//	break;
				if(n->getContent() == c_dirt ||
						n->getContent() == c_dirt_with_grass ||
						n->getContent() == c_gravel)
					break;
					
				vmanip.m_area.add_y(em, i, -1);
			}

			// Stop if out of area
			//if(vmanip.m_area.contains(i) == false)
			if(y < node_min.Y)
				break;

			/*// If not mud, do nothing to it
			MapNode *n = &vmanip.m_data[i];
			if(n->d != CONTENT_MUD && n->d != CONTENT_GRASS)
				continue;*/

			if(n->getContent() == c_dirt ||
					n->getContent() == c_dirt_with_grass)
			{
				// Make it exactly mud
				n->setContent(c_dirt);
				
				/*
					Don't flow it if the stuff under it is not mud
				*/
				{
					u32 i2 = i;
					vmanip.m_area.add_y(em, i2, -1);
					// Cancel if out of area
					if(vmanip.m_area.contains(i2) == false)
						continue;
					MapNode *n2 = &vmanip.m_data[i2];
					if(n2->getContent() != c_dirt &&
							n2->getContent() != c_dirt_with_grass)
						continue;
				}
			}

			/*s16 recurse_count = 0;
	mudflow_recurse:*/

			v3s16 dirs4[4] = {
				v3s16(0,0,1), // back
				v3s16(1,0,0), // right
				v3s16(0,0,-1), // front
				v3s16(-1,0,0), // left
			};

			// Theck that upper is air or doesn't exist.
			// Cancel dropping if upper keeps it in place
			u32 i3 = i;
			vmanip.m_area.add_y(em, i3, 1);
			if(vmanip.m_area.contains(i3) == true
					&& ndef->get(vmanip.m_data[i3]).walkable)
			{
				continue;
			}

			// Drop mud on side
			
			for(u32 di=0; di<4; di++)
			{
				v3s16 dirp = dirs4[di];
				u32 i2 = i;
				// Move to side
				vmanip.m_area.add_p(em, i2, dirp);
				// Fail if out of area
				if(vmanip.m_area.contains(i2) == false)
					continue;
				// Check that side is air
				MapNode *n2 = &vmanip.m_data[i2];
				if(ndef->get(*n2).walkable)
					continue;
				// Check that under side is air
				vmanip.m_area.add_y(em, i2, -1);
				if(vmanip.m_area.contains(i2) == false)
					continue;
				n2 = &vmanip.m_data[i2];
				if(ndef->get(*n2).walkable)
					continue;
				/*// Check that under that is air (need a drop of 2)
				vmanip.m_area.add_y(em, i2, -1);
				if(vmanip.m_area.contains(i2) == false)
					continue;
				n2 = &vmanip.m_data[i2];
				if(content_walkable(n2->d))
					continue;*/
				// Loop further down until not air
				bool dropped_to_unknown = false;
				do{
					vmanip.m_area.add_y(em, i2, -1);
					n2 = &vmanip.m_data[i2];
					// if out of known area
					if(vmanip.m_area.contains(i2) == false
							|| n2->getContent() == CONTENT_IGNORE){
						dropped_to_unknown = true;
						break;
					}
				}while(ndef->get(*n2).walkable == false);
				// Loop one up so that we're in air
				vmanip.m_area.add_y(em, i2, 1);
				n2 = &vmanip.m_data[i2];
				
				bool old_is_water = (n->getContent() == c_water_source);
				// Move mud to new place
				if(!dropped_to_unknown) {
					*n2 = *n;
					// Set old place to be air (or water)
					if(old_is_water)
						*n = MapNode(c_water_source);
					else
						*n = MapNode(CONTENT_AIR);
				}

				// Done
				break;
			}
		}
		}
	}
	
	}

	}//timer1
#endif

	} // Aging loop
	/***********************
		END OF AGING LOOP
	************************/

	/*
		Add top and bottom side of water to transforming_liquid queue
	*/

	for(s16 x=full_node_min.X; x<=full_node_max.X; x++)
	for(s16 z=full_node_min.Z; z<=full_node_max.Z; z++)
	{
		// Node position
		v2s16 p2d(x,z);
		{
			bool water_found = false;
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, full_node_max.Y, p2d.Y));
			for(s16 y=full_node_max.Y; y>=full_node_min.Y; y--)
			{
				if(y == full_node_max.Y){
					water_found = 
						(vmanip.m_data[i].getContent() == c_water_source ||
						vmanip.m_data[i].getContent() == c_lava_source);
				}
				else if(water_found == false)
				{
					if(vmanip.m_data[i].getContent() == c_water_source ||
							vmanip.m_data[i].getContent() == c_lava_source)
					{
						v3s16 p = v3s16(p2d.X, y, p2d.Y);
						data->transforming_liquid.push_back(p);
						water_found = true;
					}
				}
				else
				{
					// This can be done because water_found can only
					// turn to true and end up here after going through
					// a single block.
					if(vmanip.m_data[i+1].getContent() != c_water_source ||
							vmanip.m_data[i+1].getContent() != c_lava_source)
					{
						v3s16 p = v3s16(p2d.X, y+1, p2d.Y);
						data->transforming_liquid.push_back(p);
						water_found = false;
					}
				}

				vmanip.m_area.add_y(em, i, -1);
			}
		}
	}

	/*
		Grow grass
	*/

	for(s16 x=full_node_min.X; x<=full_node_max.X; x++)
	for(s16 z=full_node_min.Z; z<=full_node_max.Z; z++)
	{
		// Node position in 2d
		v2s16 p2d = v2s16(x,z);
		
		/*
			Find the lowest surface to which enough light ends up
			to make grass grow.

			Basically just wait until not air and not leaves.
		*/
		s16 surface_y = 0;
		{
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_max.Y, p2d.Y));
			s16 y;
			// Go to ground level
			for(y=node_max.Y; y>=full_node_min.Y; y--)
			{
				MapNode &n = vmanip.m_data[i];
				if(ndef->get(n).param_type != CPT_LIGHT
						|| ndef->get(n).liquid_type != LIQUID_NONE)
					break;
				vmanip.m_area.add_y(em, i, -1);
			}
			if(y >= full_node_min.Y)
				surface_y = y;
			else
				surface_y = full_node_min.Y;
		}
		
		u32 i = vmanip.m_area.index(p2d.X, surface_y, p2d.Y);
		MapNode *n = &vmanip.m_data[i];
		if(n->getContent() == c_dirt){
			// Well yeah, this can't be overground...
			if(surface_y < WATER_LEVEL - 20)
				continue;
			n->setContent(c_dirt_with_grass);
		}
	}

	/*
		Generate some trees
	*/
	assert(central_area_size.X == central_area_size.Z);
	{
		// Divide area into parts
		s16 div = 8;
		s16 sidelen = central_area_size.X / div;
		double area = sidelen * sidelen;
		for(s16 x0=0; x0<div; x0++)
		for(s16 z0=0; z0<div; z0++)
		{
			// Center position of part of division
			v2s16 p2d_center(
				node_min.X + sidelen/2 + sidelen*x0,
				node_min.Z + sidelen/2 + sidelen*z0
			);
			// Minimum edge of part of division
			v2s16 p2d_min(
				node_min.X + sidelen*x0,
				node_min.Z + sidelen*z0
			);
			// Maximum edge of part of division
			v2s16 p2d_max(
				node_min.X + sidelen + sidelen*x0 - 1,
				node_min.Z + sidelen + sidelen*z0 - 1
			);
			// Amount of trees
			u32 tree_count = area * tree_amount_2d(data->seed, p2d_center);
			// Put trees in random places on part of division
			for(u32 i=0; i<tree_count; i++)
			{
				s16 x = myrand_range(p2d_min.X, p2d_max.X);
				s16 z = myrand_range(p2d_min.Y, p2d_max.Y);
				s16 y = find_ground_level(vmanip, v2s16(x,z), ndef);
				// Don't make a tree under water level
				if(y < WATER_LEVEL)
					continue;
				// Don't make a tree so high that it doesn't fit
				if(y > node_max.Y - 6)
					continue;
				v3s16 p(x,y,z);
				/*
					Trees grow only on mud and grass
				*/
				{
					u32 i = vmanip.m_area.index(v3s16(p));
					MapNode *n = &vmanip.m_data[i];
					if(n->getContent() != c_dirt
							&& n->getContent() != c_dirt_with_grass)
						continue;
				}
				p.Y++;
				// Make a tree
				make_tree(vmanip, p, false, ndef);
			}
		}
	}

#if 0
	/*
		Make base ground level
	*/

	for(s16 x=node_min.X; x<=node_max.X; x++)
	for(s16 z=node_min.Z; z<=node_max.Z; z++)
	{
		// Node position
		v2s16 p2d(x,z);
		{
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_min.Y, p2d.Y));
			for(s16 y=node_min.Y; y<=node_max.Y; y++)
			{
				// Only modify places that have no content
				if(vmanip.m_data[i].getContent() == CONTENT_IGNORE)
				{
					// First priority: make air and water.
					// This avoids caves inside water.
					if(all_is_ground_except_caves == false
							&& val_is_ground(noisebuf_ground.get(x,y,z),
							v3s16(x,y,z), data->seed) == false)
					{
						if(y <= WATER_LEVEL)
							vmanip.m_data[i] = n_water_source;
						else
							vmanip.m_data[i] = n_air;
					}
					else if(noisebuf_cave.get(x,y,z) > CAVE_NOISE_THRESHOLD)
						vmanip.m_data[i] = n_air;
					else
						vmanip.m_data[i] = n_stone;
				}
			
				vmanip->m_area.add_y(em, i, 1);
			}
		}
	}

	/*
		Add mud and sand and others underground (in place of stone)
	*/

	for(s16 x=node_min.X; x<=node_max.X; x++)
	for(s16 z=node_min.Z; z<=node_max.Z; z++)
	{
		// Node position
		v2s16 p2d(x,z);
		{
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_max.Y, p2d.Y));
			for(s16 y=node_max.Y; y>=node_min.Y; y--)
			{
				if(vmanip.m_data[i].getContent() == c_stone)
				{
					if(noisebuf_ground_crumbleness.get(x,y,z) > 1.3)
					{
						if(noisebuf_ground_wetness.get(x,y,z) > 0.0)
							vmanip.m_data[i] = n_dirt;
						else
							vmanip.m_data[i] = n_sand;
					}
					else if(noisebuf_ground_crumbleness.get(x,y,z) > 0.7)
					{
						if(noisebuf_ground_wetness.get(x,y,z) < -0.6)
							vmanip.m_data[i] = n_gravel;
					}
					else if(noisebuf_ground_crumbleness.get(x,y,z) <
							-3.0 + MYMIN(0.1 * sqrt((float)MYMAX(0, -y)), 1.5))
					{
						vmanip.m_data[i] = n_lava_source;
						for(s16 x1=-1; x1<=1; x1++)
						for(s16 y1=-1; y1<=1; y1++)
						for(s16 z1=-1; z1<=1; z1++)
							data->transforming_liquid.push_back(
									v3s16(p2d.X+x1, y+y1, p2d.Y+z1));
					}
				}

				vmanip->m_area.add_y(em, i, -1);
			}
		}
	}

	/*
		Add dungeons
	*/
	
	//if(node_min.Y < approx_groundlevel)
	//if(myrand() % 3 == 0)
	//if(myrand() % 3 == 0 && node_min.Y < approx_groundlevel)
	//if(myrand() % 100 == 0 && node_min.Y < approx_groundlevel)
	//float dungeon_rarity = g_settings.getFloat("dungeon_rarity");
	float dungeon_rarity = 0.02;
	if(((noise3d(blockpos.X,blockpos.Y,blockpos.Z,data->seed)+1.0)/2.0)
			< dungeon_rarity
			&& node_min.Y < approx_groundlevel)
	{
		// Dungeon generator doesn't modify places which have this set
		vmanip->clearFlag(VMANIP_FLAG_DUNGEON_INSIDE
				| VMANIP_FLAG_DUNGEON_PRESERVE);
		
		// Set all air and water to be untouchable to make dungeons open
		// to caves and open air
		for(s16 x=full_node_min.X; x<=full_node_max.X; x++)
		for(s16 z=full_node_min.Z; z<=full_node_max.Z; z++)
		{
			// Node position
			v2s16 p2d(x,z);
			{
				// Use fast index incrementing
				v3s16 em = vmanip.m_area.getExtent();
				u32 i = vmanip.m_area.index(v3s16(p2d.X, full_node_max.Y, p2d.Y));
				for(s16 y=full_node_max.Y; y>=full_node_min.Y; y--)
				{
					if(vmanip.m_data[i].getContent() == CONTENT_AIR)
						vmanip.m_flags[i] |= VMANIP_FLAG_DUNGEON_PRESERVE;
					else if(vmanip.m_data[i].getContent() == c_water_source)
						vmanip.m_flags[i] |= VMANIP_FLAG_DUNGEON_PRESERVE;
					vmanip->m_area.add_y(em, i, -1);
				}
			}
		}
		
		PseudoRandom random(blockseed+2);

		// Add it
		make_dungeon1(vmanip, random, ndef);
		
		// Convert some cobble to mossy cobble
		for(s16 x=full_node_min.X; x<=full_node_max.X; x++)
		for(s16 z=full_node_min.Z; z<=full_node_max.Z; z++)
		{
			// Node position
			v2s16 p2d(x,z);
			{
				// Use fast index incrementing
				v3s16 em = vmanip.m_area.getExtent();
				u32 i = vmanip.m_area.index(v3s16(p2d.X, full_node_max.Y, p2d.Y));
				for(s16 y=full_node_max.Y; y>=full_node_min.Y; y--)
				{
					// (noisebuf not used because it doesn't contain the
					//  full area)
					double wetness = noise3d_param(
							get_ground_wetness_params(data->seed), x,y,z);
					double d = noise3d_perlin((float)x/2.5,
							(float)y/2.5,(float)z/2.5,
							blockseed, 2, 1.4);
					if(vmanip.m_data[i].getContent() == c_cobble)
					{
						if(d < wetness/3.0)
						{
							vmanip.m_data[i].setContent(c_mossycobble);
						}
					}
					/*else if(vmanip.m_flags[i] & VMANIP_FLAG_DUNGEON_INSIDE)
					{
						if(wetness > 1.2)
							vmanip.m_data[i].setContent(c_dirt);
					}*/
					vmanip->m_area.add_y(em, i, -1);
				}
			}
		}
	}

	/*
		Add NC
	*/
	{
		PseudoRandom ncrandom(blockseed+9324342);
		if(ncrandom.range(0, 1000) == 0 && blockpos.Y <= -3)
		{
			make_nc(vmanip, ncrandom, ndef);
		}
	}
	
	/*
		Add top and bottom side of water to transforming_liquid queue
	*/

	for(s16 x=node_min.X; x<=node_max.X; x++)
	for(s16 z=node_min.Z; z<=node_max.Z; z++)
	{
		// Node position
		v2s16 p2d(x,z);
		{
			bool water_found = false;
			// Use fast index incrementing
			v3s16 em = vmanip.m_area.getExtent();
			u32 i = vmanip.m_area.index(v3s16(p2d.X, node_max.Y, p2d.Y));
			for(s16 y=node_max.Y; y>=node_min.Y; y--)
			{
				if(water_found == false)
				{
					if(vmanip.m_data[i].getContent() == c_water_source)
					{
						v3s16 p = v3s16(p2d.X, y, p2d.Y);
						data->transforming_liquid.push_back(p);
						water_found = true;
					}
				}
				else
				{
					// This can be done because water_found can only
					// turn to true and end up here after going through
					// a single block.
					if(vmanip.m_data[i+1].getContent() != c_water_source)
					{
						v3s16 p = v3s16(p2d.X, y+1, p2d.Y);
						data->transforming_liquid.push_back(p);
						water_found = false;
					}
				}

				vmanip->m_area.add_y(em, i, -1);
			}
		}
	}

	/*
		If close to ground level
	*/

	//if(abs(approx_ground_depth) < 30)
	if(minimum_ground_depth < 5 && maximum_ground_depth > -5)
	{
		/*
			Add grass and mud
		*/

		for(s16 x=node_min.X; x<=node_max.X; x++)
		for(s16 z=node_min.Z; z<=node_max.Z; z++)
		{
			// Node position
			v2s16 p2d(x,z);
			{
				bool possibly_have_sand = get_have_beach(data->seed, p2d);
				bool have_sand = false;
				u32 current_depth = 0;
				bool air_detected = false;
				bool water_detected = false;
				bool have_clay = false;

				// Use fast index incrementing
				s16 start_y = node_max.Y+2;
				v3s16 em = vmanip.m_area.getExtent();
				u32 i = vmanip.m_area.index(v3s16(p2d.X, start_y, p2d.Y));
				for(s16 y=start_y; y>=node_min.Y-3; y--)
				{
					if(vmanip.m_data[i].getContent() == c_water_source)
						water_detected = true;
					if(vmanip.m_data[i].getContent() == CONTENT_AIR)
						air_detected = true;

					if((vmanip.m_data[i].getContent() == c_stone
							|| vmanip.m_data[i].getContent() == c_dirt_with_grass
							|| vmanip.m_data[i].getContent() == c_dirt
							|| vmanip.m_data[i].getContent() == c_sand
							|| vmanip.m_data[i].getContent() == c_gravel
							) && (air_detected || water_detected))
					{
						if(current_depth == 0 && y <= WATER_LEVEL+2
								&& possibly_have_sand)
							have_sand = true;
						
						if(current_depth < 4)
						{
							if(have_sand)
							{
								vmanip.m_data[i] = MapNode(c_sand);
							}
							#if 1
							else if(current_depth==0 && !water_detected
									&& y >= WATER_LEVEL && air_detected)
								vmanip.m_data[i] = MapNode(c_dirt_with_grass);
							#endif
							else
								vmanip.m_data[i] = MapNode(c_dirt);
						}
						else
						{
							if(vmanip.m_data[i].getContent() == c_dirt
								|| vmanip.m_data[i].getContent() == c_dirt_with_grass)
								vmanip.m_data[i] = MapNode(c_stone);
						}

						current_depth++;

						if(current_depth >= 8)
							break;
					}
					else if(current_depth != 0)
						break;

					vmanip->m_area.add_y(em, i, -1);
				}
			}
		}

		/*
			Calculate some stuff
		*/
		
		float surface_humidity = surface_humidity_2d(data->seed, p2d_center);
		bool is_jungle = surface_humidity > 0.75;
		// Amount of trees
		u32 tree_count = gen_area_nodes * tree_amount_2d(data->seed, p2d_center);
		if(is_jungle)
			tree_count *= 5;

		/*
			Add trees
		*/
		PseudoRandom treerandom(blockseed);
		// Put trees in random places on part of division
		for(u32 i=0; i<tree_count; i++)
		{
			s16 x = treerandom.range(node_min.X, node_max.X);
			s16 z = treerandom.range(node_min.Z, node_max.Z);
			//s16 y = find_ground_level(vmanip, v2s16(x,z));
			s16 y = find_ground_level_from_noise(data->seed, v2s16(x,z), 4);
			// Don't make a tree under water level
			if(y < WATER_LEVEL)
				continue;
			// Make sure tree fits (only trees whose starting point is
			// at this block are added)
			if(y < node_min.Y || y > node_max.Y)
				continue;
			/*
				Find exact ground level
			*/
			v3s16 p(x,y+6,z);
			bool found = false;
			for(; p.Y >= y-6; p.Y--)
			{
				u32 i = vmanip->m_area.index(p);
				MapNode *n = &vmanip->m_data[i];
				if(n->getContent() != CONTENT_AIR && n->getContent() != c_water_source && n->getContent() != CONTENT_IGNORE)
				{
					found = true;
					break;
				}
			}
			// If not found, handle next one
			if(found == false)
				continue;

			{
				u32 i = vmanip->m_area.index(p);
				MapNode *n = &vmanip->m_data[i];

				if(n->getContent() != c_dirt && n->getContent() != c_dirt_with_grass && n->getContent() != c_sand)
						continue;

				// Papyrus grows only on mud and in water
				if(n->getContent() == c_dirt && y <= WATER_LEVEL)
				{
					p.Y++;
					make_papyrus(vmanip, p, ndef);
				}
				// Trees grow only on mud and grass, on land
				else if((n->getContent() == c_dirt || n->getContent() == c_dirt_with_grass) && y > WATER_LEVEL + 2)
				{
					p.Y++;
					//if(surface_humidity_2d(data->seed, v2s16(x, y)) < 0.5)
					if(is_jungle == false)
					{
						bool is_apple_tree;
						if(myrand_range(0,4) != 0)
							is_apple_tree = false;
						else
							is_apple_tree = noise2d_perlin(
									0.5+(float)p.X/100, 0.5+(float)p.Z/100,
									data->seed+342902, 3, 0.45) > 0.2;
						make_tree(vmanip, p, is_apple_tree, ndef);
					}
					else
						make_jungletree(vmanip, p, ndef);
				}
				// Cactii grow only on sand, on land
				else if(n->getContent() == c_sand && y > WATER_LEVEL + 2)
				{
					p.Y++;
					make_cactus(vmanip, p, ndef);
				}
			}
		}

		/*
			Add jungle grass
		*/
		if(is_jungle)
		{
			PseudoRandom grassrandom(blockseed);
			for(u32 i=0; i<surface_humidity*5*tree_count; i++)
			{
				s16 x = grassrandom.range(node_min.X, node_max.X);
				s16 z = grassrandom.range(node_min.Z, node_max.Z);
				s16 y = find_ground_level_from_noise(data->seed, v2s16(x,z), 4);
				if(y < WATER_LEVEL)
					continue;
				if(y < node_min.Y || y > node_max.Y)
					continue;
				/*
					Find exact ground level
				*/
				v3s16 p(x,y+6,z);
				bool found = false;
				for(; p.Y >= y-6; p.Y--)
				{
					u32 i = vmanip->m_area.index(p);
					MapNode *n = &vmanip->m_data[i];
					if(data->nodedef->get(*n).is_ground_content)
					{
						found = true;
						break;
					}
				}
				// If not found, handle next one
				if(found == false)
					continue;
				p.Y++;
				if(vmanip.m_area.contains(p) == false)
					continue;
				if(vmanip.m_data[vmanip.m_area.index(p)].getContent() != CONTENT_AIR)
					continue;
				/*p.Y--;
				if(vmanip.m_area.contains(p))
					vmanip.m_data[vmanip.m_area.index(p)] = c_dirt;
				p.Y++;*/
				if(vmanip.m_area.contains(p))
					vmanip.m_data[vmanip.m_area.index(p)] = c_junglegrass;
			}
		}

#if 0
		/*
			Add some kind of random stones
		*/
		
		u32 random_stone_count = gen_area_nodes *
				randomstone_amount_2d(data->seed, p2d_center);
		// Put in random places on part of division
		for(u32 i=0; i<random_stone_count; i++)
		{
			s16 x = myrand_range(node_min.X, node_max.X);
			s16 z = myrand_range(node_min.Z, node_max.Z);
			s16 y = find_ground_level_from_noise(data->seed, v2s16(x,z), 1);
			// Don't add under water level
			/*if(y < WATER_LEVEL)
				continue;*/
			// Don't add if doesn't belong to this block
			if(y < node_min.Y || y > node_max.Y)
				continue;
			v3s16 p(x,y,z);
			// Filter placement
			/*{
				u32 i = vmanip->m_area.index(v3s16(p));
				MapNode *n = &vmanip->m_data[i];
				if(n->getContent() != c_dirt && n->getContent() != c_dirt_with_grass)
					continue;
			}*/
			// Will be placed one higher
			p.Y++;
			// Add it
			make_randomstone(vmanip, p);
		}
#endif

#if 0
		/*
			Add larger stones
		*/
		
		u32 large_stone_count = gen_area_nodes *
				largestone_amount_2d(data->seed, p2d_center);
		//u32 large_stone_count = 1;
		// Put in random places on part of division
		for(u32 i=0; i<large_stone_count; i++)
		{
			s16 x = myrand_range(node_min.X, node_max.X);
			s16 z = myrand_range(node_min.Z, node_max.Z);
			s16 y = find_ground_level_from_noise(data->seed, v2s16(x,z), 1);
			// Don't add under water level
			/*if(y < WATER_LEVEL)
				continue;*/
			// Don't add if doesn't belong to this block
			if(y < node_min.Y || y > node_max.Y)
				continue;
			v3s16 p(x,y,z);
			// Filter placement
			/*{
				u32 i = vmanip->m_area.index(v3s16(p));
				MapNode *n = &vmanip->m_data[i];
				if(n->getContent() != c_dirt && n->getContent() != c_dirt_with_grass)
					continue;
			}*/
			// Will be placed one lower
			p.Y--;
			// Add it
			make_largestone(vmanip, p);
		}
#endif
	}

	/*
		Add minerals
	*/

	{
		PseudoRandom mineralrandom(blockseed);

		/*
			Add meseblocks
		*/
		for(s16 i=0; i<approx_ground_depth/4; i++)
		{
			if(mineralrandom.next()%50 == 0)
			{
				s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
				s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
				s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
				for(u16 i=0; i<27; i++)
				{
					v3s16 p = v3s16(x,y,z) + g_27dirs[i];
					u32 vi = vmanip.m_area.index(p);
					if(vmanip.m_data[vi].getContent() == c_stone)
						if(mineralrandom.next()%8 == 0)
							vmanip.m_data[vi] = MapNode(c_mese);
				}
					
			}
		}
		/*
			Add others
		*/
		{
			u16 a = mineralrandom.range(0,15);
			a = a*a*a;
			u16 amount = 20 * a/1000;
			for(s16 i=0; i<amount; i++)
			{
				s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
				s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
				s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);

				u8 base_content = c_stone;
				MapNode new_content(CONTENT_IGNORE);
				u32 sparseness = 6;

				if(noisebuf_ground_crumbleness.get(x,y+5,z) < -0.1)
				{
					new_content = MapNode(c_stone_with_coal);
				}
				else
				{
					if(noisebuf_ground_wetness.get(x,y+5,z) > 0.0)
						new_content = MapNode(c_stone_with_iron);
					/*if(noisebuf_ground_wetness.get(x,y,z) > 0.0)
						vmanip.m_data[i] = MapNode(c_dirt);
					else
						vmanip.m_data[i] = MapNode(c_sand);*/
				}
				/*else if(noisebuf_ground_crumbleness.get(x,y,z) > 0.1)
				{
				}*/

				if(new_content.getContent() != CONTENT_IGNORE)
				{
					for(u16 i=0; i<27; i++)
					{
						v3s16 p = v3s16(x,y,z) + g_27dirs[i];
						u32 vi = vmanip.m_area.index(p);
						if(vmanip.m_data[vi].getContent() == base_content)
						{
							if(mineralrandom.next()%sparseness == 0)
								vmanip.m_data[vi] = new_content;
						}
					}
				}
			}
		}
		/*
			Add coal
		*/
		//for(s16 i=0; i < MYMAX(0, 50 - abs(node_min.Y+8 - (-30))); i++)
		//for(s16 i=0; i<50; i++)
		u16 coal_amount = 30;
		u16 coal_rareness = 60 / coal_amount;
		if(coal_rareness == 0)
			coal_rareness = 1;
		if(mineralrandom.next()%coal_rareness == 0)
		{
			u16 a = mineralrandom.next() % 16;
			u16 amount = coal_amount * a*a*a / 1000;
			for(s16 i=0; i<amount; i++)
			{
				s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
				s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
				s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
				for(u16 i=0; i<27; i++)
				{
					v3s16 p = v3s16(x,y,z) + g_27dirs[i];
					u32 vi = vmanip.m_area.index(p);
					if(vmanip.m_data[vi].getContent() == c_stone)
						if(mineralrandom.next()%8 == 0)
							vmanip.m_data[vi] = MapNode(c_stone_with_coal);
				}
			}
		}
		/*
			Add iron
		*/
		u16 iron_amount = 8;
		u16 iron_rareness = 60 / iron_amount;
		if(iron_rareness == 0)
			iron_rareness = 1;
		if(mineralrandom.next()%iron_rareness == 0)
		{
			u16 a = mineralrandom.next() % 16;
			u16 amount = iron_amount * a*a*a / 1000;
			for(s16 i=0; i<amount; i++)
			{
				s16 x = mineralrandom.range(node_min.X+1, node_max.X-1);
				s16 y = mineralrandom.range(node_min.Y+1, node_max.Y-1);
				s16 z = mineralrandom.range(node_min.Z+1, node_max.Z-1);
				for(u16 i=0; i<27; i++)
				{
					v3s16 p = v3s16(x,y,z) + g_27dirs[i];
					u32 vi = vmanip.m_area.index(p);
					if(vmanip.m_data[vi].getContent() == c_stone)
						if(mineralrandom.next()%8 == 0)
							vmanip.m_data[vi] = MapNode(c_stone_with_iron);
				}
			}
		}
	}
#endif

	/*
		Calculate lighting
	*/
	{
	ScopeProfiler sp(g_profiler, "EmergeThread: mapgen lighting update",
			SPT_AVG);
	//VoxelArea a(node_min, node_max);
	VoxelArea a(node_min-v3s16(1,0,1)*MAP_BLOCKSIZE,
			node_max+v3s16(1,0,1)*MAP_BLOCKSIZE);
	/*VoxelArea a(node_min-v3s16(1,0,1)*MAP_BLOCKSIZE/2,
			node_max+v3s16(1,0,1)*MAP_BLOCKSIZE/2);*/
	enum LightBank banks[2] = {LIGHTBANK_DAY, LIGHTBANK_NIGHT};
	for(int i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];

		core::map<v3s16, bool> light_sources;
		core::map<v3s16, u8> unlight_from;

		voxalgo::clearLightAndCollectSources(vmanip, a, bank, ndef,
				light_sources, unlight_from);
		
		bool inexistent_top_provides_sunlight = !block_is_underground;
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				vmanip, a, inexistent_top_provides_sunlight,
				light_sources, ndef);
		// TODO: Do stuff according to bottom_sunlight_valid

		vmanip.unspreadLight(bank, unlight_from, light_sources, ndef);

		vmanip.spreadLight(bank, light_sources, ndef);
	}
	}
}

BlockMakeData::BlockMakeData():
	no_op(false),
	vmanip(NULL),
	seed(0),
	nodedef(NULL)
{}

BlockMakeData::~BlockMakeData()
{
	delete vmanip;
}

}; // namespace mapgen


