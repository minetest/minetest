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

#include "voxel.h"
#include "map.h"

VoxelManipulator::VoxelManipulator():
	m_data(NULL),
	m_flags(NULL)
{
}

VoxelManipulator::~VoxelManipulator()
{
	clear();
	if(m_data)
		delete[] m_data;
	if(m_flags)
		delete[] m_flags;
}

void VoxelManipulator::clear()
{
	// Reset area to volume=0
	m_area = VoxelArea();
	if(m_data)
		delete[] m_data;
	m_data = NULL;
	if(m_flags)
		delete[] m_flags;
	m_flags = NULL;
}

void VoxelManipulator::print(std::ostream &o)
{
	v3s16 em = m_area.getExtent();
	v3s16 of = m_area.MinEdge;
	o<<"size: "<<em.X<<"x"<<em.Y<<"x"<<em.Z
	 <<" offset: ("<<of.X<<","<<of.Y<<","<<of.Z<<")"<<std::endl;
	
	for(s32 y=m_area.MaxEdge.Y; y>=m_area.MinEdge.Y; y--)
	{
		if(em.X >= 3 && em.Y >= 3)
		{
			if     (y==m_area.MinEdge.Y+2) o<<"^     ";
			else if(y==m_area.MinEdge.Y+1) o<<"|     ";
			else if(y==m_area.MinEdge.Y+0) o<<"y x-> ";
			else                           o<<"      ";
		}

		for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
		{
			for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
			{
				u8 f = m_flags[m_area.index(x,y,z)];
				char c;
				if(f & VOXELFLAG_NOT_LOADED)
					c = 'N';
				else if(f & VOXELFLAG_INEXISTENT)
					c = 'I';
				else
				{
					c = 'X';
					u8 m = m_data[m_area.index(x,y,z)].d;
					if(m <= 9)
						c = m + '0';
				}
				o<<c;
			}
			o<<' ';
		}
		o<<std::endl;
	}
}

void VoxelManipulator::addArea(VoxelArea area)
{
	// Cancel if requested area has zero volume
	if(area.getExtent() == v3s16(0,0,0))
		return;
	
	// Cancel if m_area already contains the requested area
	if(m_area.contains(area))
		return;
	
	// Calculate new area
	VoxelArea new_area;
	// New area is the requested area if m_area has zero volume
	if(m_area.getExtent() == v3s16(0,0,0))
	{
		new_area = area;
	}
	// Else add requested area to m_area
	else
	{
		new_area = m_area;
		new_area.addArea(area);
	}

	s32 new_size = new_area.getVolume();

	/*dstream<<"adding area ";
	area.print(dstream);
	dstream<<", old area ";
	m_area.print(dstream);
	dstream<<", new area ";
	new_area.print(dstream);
	dstream<<", new_size="<<new_size;
	dstream<<std::endl;*/

	// Allocate and clear new data
	MapNode *new_data = new MapNode[new_size];
	u8 *new_flags = new u8[new_size];
	for(s32 i=0; i<new_size; i++)
	{
		new_flags[i] = VOXELFLAG_NOT_LOADED;
	}
	
	// Copy old data
	
	for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
	for(s32 y=m_area.MinEdge.Y; y<=m_area.MaxEdge.Y; y++)
	for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
	{
		// If loaded, copy data and flags
		if((m_flags[m_area.index(x,y,z)] & VOXELFLAG_NOT_LOADED) == false)
		{
			new_data[new_area.index(x,y,z)] = m_data[m_area.index(x,y,z)];
			new_flags[new_area.index(x,y,z)] = m_flags[m_area.index(x,y,z)];
		}
	}

	// Replace area, data and flags
	
	m_area = new_area;
	
	MapNode *old_data = m_data;
	u8 *old_flags = m_flags;

	/*dstream<<"old_data="<<(int)old_data<<", new_data="<<(int)new_data
	<<", old_flags="<<(int)m_flags<<", new_flags="<<(int)new_flags<<std::endl;*/

	m_data = new_data;
	m_flags = new_flags;
	
	if(old_data)
		delete[] old_data;
	if(old_flags)
		delete[] old_flags;
}

void VoxelManipulator::interpolate(VoxelArea area)
{
	VoxelArea emerge_area = area;
	emerge_area.MinEdge -= v3s16(1,1,1);
	emerge_area.MaxEdge += v3s16(1,1,1);
	emerge(emerge_area);

	SharedBuffer<u8> buf(area.getVolume());

	for(s32 z=area.MinEdge.Z; z<=area.MaxEdge.Z; z++)
	for(s32 y=area.MinEdge.Y; y<=area.MaxEdge.Y; y++)
	for(s32 x=area.MinEdge.X; x<=area.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);

		v3s16 dirs[] = {
			v3s16(1,1,0),
			v3s16(1,0,1),
			v3s16(1,-1,0),
			v3s16(1,0,-1),
			v3s16(-1,1,0),
			v3s16(-1,0,1),
			v3s16(-1,-1,0),
			v3s16(-1,0,-1),
		};
		//const v3s16 *dirs = g_26dirs;
		
		s16 total = 0;
		s16 airness = 0;
		u8 m = MATERIAL_IGNORE;

		for(s16 i=0; i<8; i++)
		//for(s16 i=0; i<26; i++)
		{
			v3s16 p2 = p + dirs[i];

			u8 f = m_flags[m_area.index(p2)];
			assert(!(f & VOXELFLAG_NOT_LOADED));
			if(f & VOXELFLAG_INEXISTENT)
				continue;

			MapNode &n = m_data[m_area.index(p2)];

			airness += (n.d == MATERIAL_AIR) ? 1 : -1;
			total++;

			if(m == MATERIAL_IGNORE && n.d != MATERIAL_AIR)
				m = n.d;
		}

		// 1 if air, 0 if not
		buf[area.index(p)] = airness > -total/2 ? MATERIAL_AIR : m;
		//buf[area.index(p)] = airness > -total ? MATERIAL_AIR : m;
		//buf[area.index(p)] = airness >= -7 ? MATERIAL_AIR : m;
	}

	for(s32 z=area.MinEdge.Z; z<=area.MaxEdge.Z; z++)
	for(s32 y=area.MinEdge.Y; y<=area.MaxEdge.Y; y++)
	for(s32 x=area.MinEdge.X; x<=area.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		m_data[m_area.index(p)].d = buf[area.index(p)];
	}
}

void VoxelManipulator::flowWater(v3s16 removed_pos)
{
	v3s16 dirs[6] = {
		v3s16(0,1,0), // top
		v3s16(-1,0,0), // left
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,0,1), // back
		v3s16(0,-1,0), // bottom
	};

	v3s16 p;

	// Load neighboring nodes
	// TODO: A bigger area would be better
	emerge(VoxelArea(removed_pos - v3s16(1,1,1), removed_pos + v3s16(1,1,1)));

	s32 i;
	for(i=0; i<6; i++)
	{
		p = removed_pos + dirs[i];
		u8 f = m_flags[m_area.index(p)];
		// Inexistent or checked nodes can't move
		if(f & (VOXELFLAG_INEXISTENT | VOXELFLAG_CHECKED))
			continue;
		MapNode &n = m_data[m_area.index(p)];
		// Only liquid nodes can move
		if(material_liquid(n.d) == false)
			continue;
		// If block is at top, select it always
		if(i == 0)
		{
			break;
		}
		// If block is at bottom, select it if it has enough pressure
		if(i == 5)
		{
			if(n.pressure >= 3)
				break;
			continue;
		}
		// Else block is at some side. Select it if it has enough pressure
		if(n.pressure >= 2)
		{
			break;
		}
	}

	// If there is nothing to move, return
	if(i==6)
		return;
	
	// Switch nodes at p and removed_pos
	MapNode n = m_data[m_area.index(p)];
	u8 f = m_flags[m_area.index(p)];
	m_data[m_area.index(p)] = m_data[m_area.index(removed_pos)];
	m_flags[m_area.index(p)] = m_flags[m_area.index(removed_pos)];
	m_data[m_area.index(removed_pos)] = n;
	m_flags[m_area.index(removed_pos)] = f;

	// Mark p checked
	m_flags[m_area.index(p)] |= VOXELFLAG_CHECKED;
	
	// Update pressure
	//TODO

	// Flow water to the newly created empty position
	flowWater(p);
	
	// Try flowing water to empty positions around removed_pos.
	// They are checked in reverse order compared to the previous loop.
	for(i=5; i>=0; i--)
	{
		p = removed_pos + dirs[i];
		u8 f = m_flags[m_area.index(p)];
		// Water can't move to inexistent nodes
		if(f & VOXELFLAG_INEXISTENT)
			continue;
		MapNode &n = m_data[m_area.index(p)];
		// Water can only move to air
		if(n.d != MATERIAL_AIR)
			continue;
		flowWater(p);
	}
}

/*
	MapVoxelManipulator
*/

MapVoxelManipulator::MapVoxelManipulator(Map *map)
{
	m_map = map;
}

void MapVoxelManipulator::emerge(VoxelArea a)
{
	v3s16 size = a.getExtent();

	addArea(a);

	for(s16 z=0; z<size.Z; z++)
	for(s16 y=0; y<size.Y; y++)
	for(s16 x=0; x<size.X; x++)
	{
		v3s16 p(x,y,z);
		s32 i = m_area.index(a.MinEdge + p);
		// Don't touch nodes that have already been loaded
		if(!(m_flags[i] & VOXELFLAG_NOT_LOADED))
			continue;
		try{
			MapNode n = m_map->getNode(a.MinEdge + p);
			m_data[i] = n;
			m_flags[i] = 0;
		}
		catch(InvalidPositionException &e)
		{
			m_flags[i] = VOXELFLAG_INEXISTENT;
		}
	}
}

void MapVoxelManipulator::blitBack
		(core::map<v3s16, MapBlock*> & modified_blocks)
{
	/*
		Initialize block cache
	*/
	v3s16 blockpos_last;
	MapBlock *block = NULL;
	bool block_checked_in_modified = false;

	for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
	for(s32 y=m_area.MinEdge.Y; y<=m_area.MaxEdge.Y; y++)
	for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);

		u8 f = m_flags[m_area.index(p)];
		if(f & (VOXELFLAG_NOT_LOADED|VOXELFLAG_INEXISTENT))
			continue;

		MapNode &n = m_data[m_area.index(p)];
			
		v3s16 blockpos = getNodeBlockPos(p);
		
		try
		{
			// Get block
			if(block == NULL || blockpos != blockpos_last){
				block = m_map->getBlockNoCreate(blockpos);
				blockpos_last = blockpos;
				block_checked_in_modified = false;
			}
			
			// Calculate relative position in block
			v3s16 relpos = p - blockpos * MAP_BLOCKSIZE;

			// Don't continue if nothing has changed here
			if(block->getNode(relpos) == n)
				continue;

			//m_map->setNode(m_area.MinEdge + p, n);
			block->setNode(relpos, n);
			
			/*
				Make sure block is in modified_blocks
			*/
			if(block_checked_in_modified == false)
			{
				modified_blocks[blockpos] = block;
				block_checked_in_modified = true;
			}
		}
		catch(InvalidPositionException &e)
		{
		}
	}
}

//END
