/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef VOXEL_HEADER
#define VOXEL_HEADER

#include "common_irrlicht.h"
#include "mapblock.h"
#include <iostream>

/*
	A fast voxel manipulator class

	Not thread-safe.
*/

/*
	This class resembles aabbox3d<s16> a lot, but has inclusive
	edges for saner handling of integer sizes
*/
class VoxelArea
{
public:
	// Starts as zero sized
	VoxelArea():
		MinEdge(1,1,1),
		MaxEdge(0,0,0)
	{
	}
	VoxelArea(v3s16 min_edge, v3s16 max_edge):
		MinEdge(min_edge),
		MaxEdge(max_edge)
	{
	}
	VoxelArea(v3s16 p):
		MinEdge(p),
		MaxEdge(p)
	{
	}
	void addArea(VoxelArea &a)
	{
		if(a.MinEdge.X < MinEdge.X) MinEdge.X = a.MinEdge.X;
		if(a.MinEdge.Y < MinEdge.Y) MinEdge.Y = a.MinEdge.Y;
		if(a.MinEdge.Z < MinEdge.Z) MinEdge.Z = a.MinEdge.Z;
		if(a.MaxEdge.X > MaxEdge.X) MaxEdge.X = a.MaxEdge.X;
		if(a.MaxEdge.Y > MaxEdge.Y) MaxEdge.Y = a.MaxEdge.Y;
		if(a.MaxEdge.Z > MaxEdge.Z) MaxEdge.Z = a.MaxEdge.Z;
	}
	void addPoint(v3s16 p)
	{
		if(p.X < MinEdge.X) MinEdge.X = p.X;
		if(p.Y < MinEdge.Y) MinEdge.Y = p.Y;
		if(p.Z < MinEdge.Z) MinEdge.Z = p.Z;
		if(p.X > MaxEdge.X) MaxEdge.X = p.X;
		if(p.Y > MaxEdge.Y) MaxEdge.Y = p.Y;
		if(p.Z > MaxEdge.Z) MaxEdge.Z = p.Z;
	}
	v3s16 getExtent() const
	{
		return MaxEdge - MinEdge + v3s16(1,1,1);
	}
	s32 getVolume() const
	{
		v3s16 e = getExtent();
		return (s32)e.X * (s32)e.Y * (s32)e.Z;
	}
	bool contains(VoxelArea &a) const
	{
		return(
			a.MinEdge.X >= MinEdge.X && a.MaxEdge.X <= MaxEdge.X &&
			a.MinEdge.Y >= MinEdge.Y && a.MaxEdge.Y <= MaxEdge.Y &&
			a.MinEdge.Z >= MinEdge.Z && a.MaxEdge.Z <= MaxEdge.Z
		);
	}
	bool contains(v3s16 p) const
	{
		return(
			p.X >= MinEdge.X && p.X <= MaxEdge.X &&
			p.Y >= MinEdge.Y && p.Y <= MaxEdge.Y &&
			p.Z >= MinEdge.Z && p.Z <= MaxEdge.Z
		);
	}
	bool operator==(const VoxelArea &other) const
	{
		return (MinEdge == other.MinEdge
				&& MaxEdge == other.MaxEdge);
	}
	
	/*
		Translates position from virtual coordinates to array index
	*/
	s32 index(s16 x, s16 y, s16 z) const
	{
		v3s16 em = getExtent();
		v3s16 off = MinEdge;
		s32 i = (s32)(z-off.Z)*em.Y*em.X + (y-off.Y)*em.X + (x-off.X);
		//dstream<<" i("<<x<<","<<y<<","<<z<<")="<<i<<" ";
		return i;
	}
	s32 index(v3s16 p) const
	{
		return index(p.X, p.Y, p.Z);
	}

	void print(std::ostream &o) const
	{
		o<<"("<<MinEdge.X
		 <<","<<MinEdge.Y
		 <<","<<MinEdge.Z
		 <<")("<<MaxEdge.X
		 <<","<<MaxEdge.Y
		 <<","<<MaxEdge.Z
		 <<")";
	}

	// Edges are inclusive
	v3s16 MinEdge;
	v3s16 MaxEdge;
};

// Hasn't been copied from source (emerged)
#define VOXELFLAG_NOT_LOADED (1<<0)
// Checked as being inexistent in source
#define VOXELFLAG_INEXISTENT (1<<1)
// Algorithm-dependent
#define VOXELFLAG_CHECKED (1<<2)

class VoxelManipulator : public NodeContainer
{
public:
	VoxelManipulator();
	~VoxelManipulator();
	
	/*
		Virtuals from NodeContainer
	*/
	virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_VOXELMANIPULATOR;
	}
	bool isValidPosition(v3s16 p)
	{
		emerge(p);
		return !(m_flags[m_area.index(p)] & VOXELFLAG_INEXISTENT);
	}
	// These are a bit slow and shouldn't be used internally
	MapNode getNode(v3s16 p)
	{
		emerge(p);

		if(m_flags[m_area.index(p)] & VOXELFLAG_INEXISTENT)
		{
			dstream<<"ERROR: VoxelManipulator::getNode(): "
					<<"p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<", index="<<m_area.index(p)
					<<", flags="<<(int)m_flags[m_area.index(p)]
					<<" is inexistent"<<std::endl;
			throw InvalidPositionException
			("VoxelManipulator: getNode: inexistent");
		}

		return m_data[m_area.index(p)];
	}
	void setNode(v3s16 p, MapNode &n)
	{
		emerge(p);
		
		m_data[m_area.index(p)] = n;
		m_flags[m_area.index(p)] &= ~VOXELFLAG_INEXISTENT;
		m_flags[m_area.index(p)] &= ~VOXELFLAG_NOT_LOADED;
	}
	void setNodeNoRef(v3s16 p, MapNode n)
	{
		setNode(p, n);
	}

	/*void setExists(VoxelArea a)
	{
		emerge(a);
		for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
		for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
		for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
		{
			m_flags[m_area.index(x,y,z)] &= ~VOXELFLAG_INEXISTENT;
		}
	}*/

	/*MapNode & operator[](v3s16 p)
	{
		//dstream<<"operator[] p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;
		if(isValidPosition(p) == false)
			emerge(VoxelArea(p));
		
		return m_data[m_area.index(p)];
	}*/

	/*
		Control
	*/

	void clear();
	
	void print(std::ostream &o);
	
	void addArea(VoxelArea area);

	/*
		Algorithms
	*/

	void interpolate(VoxelArea area);

	void flowWater(v3s16 removed_pos);

	/*
		Virtual functions
	*/
	
	/*
		Get the contents of the requested area from somewhere.
		Shall touch only nodes that have VOXELFLAG_NOT_LOADED
		Shall reset VOXELFLAG_NOT_LOADED

		If not found from source, add with VOXELFLAG_INEXISTENT
	*/
	virtual void emerge(VoxelArea a)
	{
		//dstream<<"emerge p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;
		addArea(a);
		for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
		for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
		for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
		{
			s32 i = m_area.index(x,y,z);
			// Don't touch nodes that have already been loaded
			if(!(m_flags[i] & VOXELFLAG_NOT_LOADED))
				continue;
			m_flags[i] = VOXELFLAG_INEXISTENT;
		}
	}

	/*
		Member variables
	*/

	/*
		The area that is stored in m_data.
		addInternalBox should not be used if getExtent() == v3s16(0,0,0)
		MaxEdge is 1 higher than maximum allowed position
	*/
	VoxelArea m_area;
	/*
		NULL if data size is 0 (extent (0,0,0))
		Data is stored as [z*h*w + y*h + x]
	*/
	MapNode *m_data;
	/*
		Flags of all nodes
	*/
	u8 *m_flags;
private:
};

class Map;

class MapVoxelManipulator : public VoxelManipulator
{
public:
	MapVoxelManipulator(Map *map);

	virtual void emerge(VoxelArea a);

	void blitBack(core::map<v3s16, MapBlock*> & modified_blocks);

private:
	Map *m_map;
};

#endif

