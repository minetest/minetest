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
	m_data(NULL)
{
}

VoxelManipulator::~VoxelManipulator()
{
	if(m_data)
		delete[] m_data;
}

void VoxelManipulator::addArea(VoxelArea area)
{
	if(area.getExtent() == v3s16(0,0,0))
		return;
	
	// Calculate new area
	VoxelArea new_area;
	if(m_area.getExtent() == v3s16(0,0,0))
	{
		new_area = area;
	}
	else
	{
		new_area = m_area;
		new_area.addArea(area);
	}

	if(new_area == m_area)
		return;

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
	MapNode *new_data;
	new_data = new MapNode[new_size];
	for(s32 i=0; i<new_size; i++)
	{
		new_data[i].d = MATERIAL_IGNORE;
	}
	
	// Copy old data
	
	for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
	for(s32 y=m_area.MinEdge.Y; y<=m_area.MaxEdge.Y; y++)
	for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
	{
		new_data[new_area.index(z,y,x)] = m_data[m_area.index(x,y,z)];
	}

	// Replace member
	m_area = new_area;
	MapNode *old_data = m_data;
	m_data = new_data;
	delete[] old_data;
}

void VoxelManipulator::print(std::ostream &o)
{
	v3s16 em = m_area.getExtent();
	v3s16 of = m_area.MinEdge;
	o<<"size: "<<em.X<<"x"<<em.Y<<"x"<<em.Z
	 <<" offset: ("<<of.X<<","<<of.Y<<","<<of.Z<<")"<<std::endl;
	
	for(s32 y=m_area.MinEdge.Y; y<=m_area.MaxEdge.Y; y++)
	{
		if(em.X >= 3 && em.Y >= 3)
		{
			if(y==m_area.MinEdge.Y+0) o<<"y x-> ";
			if(y==m_area.MinEdge.Y+1) o<<"|     ";
			if(y==m_area.MinEdge.Y+2) o<<"V     ";
		}

		for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
		{
			for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
			{
				u8 m = m_data[m_area.index(x,y,z)].d;
				char c = 'X';
				if(m == MATERIAL_IGNORE)
					c = 'I';
				else if(m <= 9)
					c = m + '0';
				o<<c;
			}
			o<<' ';
		}
		o<<std::endl;
	}
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

			MapNode &n = m_data[m_area.index(p2)];
			if(n.d == MATERIAL_IGNORE)
				continue;

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

#if 0
void VoxelManipulator::blitFromNodeContainer
		(v3s16 p_from, v3s16 p_to, v3s16 size, NodeContainer *c)
{
	VoxelArea a_to(p_to, p_to+size-v3s16(1,1,1));
	addArea(a_to);
	for(s16 z=0; z<size.Z; z++)
	for(s16 y=0; y<size.Y; y++)
	for(s16 x=0; x<size.X; x++)
	{
		v3s16 p(x,y,z);
		try{
			MapNode n = c->getNode(p_from + p);
			m_data[m_area.index(p_to + p)] = n;
		}
		catch(InvalidPositionException &e)
		{
		}
		
		/*v3s16 p(x,y,z);
		MapNode n(MATERIAL_IGNORE);
		try{
			n = c->getNode(p_from + p);
		}
		catch(InvalidPositionException &e)
		{
		}
		m_data[m_area.index(p_to + p)] = n;*/
	}
}

void VoxelManipulator::blitToNodeContainer
		(v3s16 p_from, v3s16 p_to, v3s16 size, NodeContainer *c)
{
	for(s16 z=0; z<size.Z; z++)
	for(s16 y=0; y<size.Y; y++)
	for(s16 x=0; x<size.X; x++)
	{
		v3s16 p(x,y,z);
		try{
			MapNode &n = m_data[m_area.index(p_from + p)];
			if(n.d == MATERIAL_IGNORE)
				continue;
			c->setNode(p_to + p, n);
		}
		catch(InvalidPositionException &e)
		{
		}
	}
}
#endif

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
		try{
			MapNode n = m_map->getNode(a.MinEdge + p);
			m_data[m_area.index(a.MinEdge + p)] = n;
		}
		catch(InvalidPositionException &e)
		{
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

		MapNode &n = m_data[m_area.index(p)];
		if(n.d == MATERIAL_IGNORE)
			continue;
			
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
