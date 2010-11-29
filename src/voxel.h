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
	TODO: A fast voxel manipulator class

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
	v3s16 getExtent()
	{
		return MaxEdge - MinEdge + v3s16(1,1,1);
	}
	s32 getVolume()
	{
		v3s16 e = getExtent();
		return (s32)e.X * (s32)e.Y * (s32)e.Z;
	}
	bool isInside(v3s16 p)
	{
		return(
			p.X >= MinEdge.X && p.X <= MaxEdge.X &&
			p.Y >= MinEdge.Y && p.Y <= MaxEdge.Y &&
			p.Z >= MinEdge.Z && p.Z <= MaxEdge.Z
		);
	}
	bool operator==(const VoxelArea &other)
	{
		return (MinEdge == other.MinEdge
				&& MaxEdge == other.MaxEdge);
	}
	
	/*
		Translates position from virtual coordinates to array index
	*/
	s32 index(s16 x, s16 y, s16 z)
	{
		v3s16 em = getExtent();
		v3s16 off = MinEdge;
		s32 i = (s32)(z-off.Z)*em.Y*em.X + (y-off.Y)*em.X + (x-off.X);
		//dstream<<" i("<<x<<","<<y<<","<<z<<")="<<i<<" ";
		return i;
	}
	s32 index(v3s16 p)
	{
		return index(p.X, p.Y, p.Z);
	}

	void print(std::ostream &o)
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
		return m_area.isInside(p);
	}
	// These are a bit slow and shouldn't be used internally
	MapNode getNode(v3s16 p)
	{
		if(isValidPosition(p) == false)
			emerge(VoxelArea(p));

		MapNode &n = m_data[m_area.index(p)];

		//TODO: Is this the right behaviour?
		if(n.d == MATERIAL_IGNORE)
			throw InvalidPositionException
			("Not returning MATERIAL_IGNORE in VoxelManipulator");

		return n;
	}
	void setNode(v3s16 p, MapNode & n)
	{
		if(isValidPosition(p) == false)
			emerge(VoxelArea(p));
		m_data[m_area.index(p)] = n;
	}
	
	MapNode & operator[](v3s16 p)
	{
		//dstream<<"operator[] p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;
		if(isValidPosition(p) == false)
			emerge(VoxelArea(p));
		return m_data[m_area.index(p)];
	}

	/*
		Manipulation of bigger chunks
	*/
	
	void print(std::ostream &o);
	
	void addArea(VoxelArea area);

	void interpolate(VoxelArea area);

	/*void blitFromNodeContainer
			(v3s16 p_from, v3s16 p_to, v3s16 size, NodeContainer *c);
	
	void blitToNodeContainer
			(v3s16 p_from, v3s16 p_to, v3s16 size, NodeContainer *c);*/

	/*
		Virtual functions
	*/
	
	/*
		Get the contents of the requested area from somewhere.

		If not found from source, add as MATERIAL_IGNORE.
	*/
	virtual void emerge(VoxelArea a)
	{
		//dstream<<"emerge p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;
		addArea(a);
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
		NULL if data size is 0
		Data is stored as [z*h*w + y*h + x]
		Special data values:
			MATERIAL_IGNORE: Unspecified node
	*/
	MapNode *m_data;
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

