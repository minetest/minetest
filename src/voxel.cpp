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

// For TimeTaker
#include "main.h"
#include "utility.h"

/*
	Debug stuff
*/
u32 addarea_time = 0;
u32 emerge_time = 0;
u32 emerge_load_time = 0;
u32 clearflag_time = 0;
//u32 getwaterpressure_time = 0;
//u32 spreadwaterpressure_time = 0;
u32 updateareawaterpressure_time = 0;
u32 flowwater_pre_time = 0;


VoxelManipulator::VoxelManipulator():
	m_data(NULL),
	m_flags(NULL)
{
	m_disable_water_climb = false;
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

void VoxelManipulator::print(std::ostream &o, VoxelPrintMode mode)
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
					u8 pr = m_data[m_area.index(x,y,z)].pressure;
					if(mode == VOXELPRINT_MATERIAL)
					{
						if(m <= 9)
							c = m + '0';
					}
					else if(mode == VOXELPRINT_WATERPRESSURE)
					{
						if(m == CONTENT_WATER)
						{
							c = 'w';
							if(pr <= 9)
								c = pr + '0';
						}
						else if(liquid_replaces_content(m))
						{
							c = ' ';
						}
						else
						{
							c = '#';
						}
					}
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
	
	TimeTaker timer("addArea", g_device, &addarea_time);

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

	//dstream<<"addArea done"<<std::endl;
}

void VoxelManipulator::copyFrom(MapNode *src, VoxelArea src_area,
		v3s16 from_pos, v3s16 to_pos, v3s16 size)
{
	for(s16 z=0; z<size.Z; z++)
	for(s16 y=0; y<size.Y; y++)
	{
		s32 i_src = src_area.index(from_pos.X, from_pos.Y+y, from_pos.Z+z);
		s32 i_local = m_area.index(to_pos.X, to_pos.Y+y, to_pos.Z+z);
		memcpy(&m_data[i_local], &src[i_src], size.X*sizeof(MapNode));
		memset(&m_flags[i_local], 0, size.X);
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
		u8 m = CONTENT_IGNORE;

		for(s16 i=0; i<8; i++)
		//for(s16 i=0; i<26; i++)
		{
			v3s16 p2 = p + dirs[i];

			u8 f = m_flags[m_area.index(p2)];
			assert(!(f & VOXELFLAG_NOT_LOADED));
			if(f & VOXELFLAG_INEXISTENT)
				continue;

			MapNode &n = m_data[m_area.index(p2)];

			airness += (n.d == CONTENT_AIR) ? 1 : -1;
			total++;

			if(m == CONTENT_IGNORE && n.d != CONTENT_AIR)
				m = n.d;
		}

		// 1 if air, 0 if not
		buf[area.index(p)] = airness > -total/2 ? CONTENT_AIR : m;
		//buf[area.index(p)] = airness > -total ? CONTENT_AIR : m;
		//buf[area.index(p)] = airness >= -7 ? CONTENT_AIR : m;
	}

	for(s32 z=area.MinEdge.Z; z<=area.MaxEdge.Z; z++)
	for(s32 y=area.MinEdge.Y; y<=area.MaxEdge.Y; y++)
	for(s32 x=area.MinEdge.X; x<=area.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		m_data[m_area.index(p)].d = buf[area.index(p)];
	}
}


void VoxelManipulator::clearFlag(u8 flags)
{
	// 0-1ms on moderate area
	TimeTaker timer("clearFlag", g_device, &clearflag_time);

	v3s16 s = m_area.getExtent();

	/*dstream<<"clearFlag clearing area of size "
			<<""<<s.X<<"x"<<s.Y<<"x"<<s.Z<<""
			<<std::endl;*/

	//s32 count = 0;

	/*for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
	for(s32 y=m_area.MinEdge.Y; y<=m_area.MaxEdge.Y; y++)
	for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
	{
		u8 f = m_flags[m_area.index(x,y,z)];
		m_flags[m_area.index(x,y,z)] &= ~flags;
		if(m_flags[m_area.index(x,y,z)] != f)
			count++;
	}*/

	s32 volume = m_area.getVolume();
	for(s32 i=0; i<volume; i++)
	{
		m_flags[i] &= ~flags;
	}

	/*s32 volume = m_area.getVolume();
	for(s32 i=0; i<volume; i++)
	{
		u8 f = m_flags[i];
		m_flags[i] &= ~flags;
		if(m_flags[i] != f)
			count++;
	}

	dstream<<"clearFlag changed "<<count<<" flags out of "
			<<volume<<" nodes"<<std::endl;*/
}

int VoxelManipulator::getWaterPressure(v3s16 p, s16 &highest_y, int recur_count)
{
	m_flags[m_area.index(p)] |= VOXELFLAG_CHECKED2;

	if(p.Y > highest_y)
		highest_y = p.Y;
	
	/*if(recur_count > 1000)
		throw ProcessingLimitException
				("getWaterPressure recur_count limit reached");*/
	
	if(recur_count > 10000)
		return -1;
	
	recur_count++;

	v3s16 dirs[6] = {
		v3s16(0,1,0), // top
		v3s16(0,0,1), // back
		v3s16(0,0,-1), // front
		v3s16(1,0,0), // right
		v3s16(-1,0,0), // left
		v3s16(0,-1,0), // bottom
	};

	// Load neighboring nodes
	emerge(VoxelArea(p - v3s16(1,1,1), p + v3s16(1,1,1)), 1);

	s32 i;
	for(i=0; i<6; i++)
	{
		v3s16 p2 = p + dirs[i];
		u8 f = m_flags[m_area.index(p2)];
		// Ignore inexistent or checked nodes
		if(f & (VOXELFLAG_INEXISTENT | VOXELFLAG_CHECKED2))
			continue;
		MapNode &n = m_data[m_area.index(p2)];
		// Ignore non-liquid nodes
		if(content_liquid(n.d) == false)
			continue;

		int pr;

		// If at ocean surface
		if(n.pressure == 1 && n.d == CONTENT_OCEAN)
		//if(n.pressure == 1) // Causes glitches but is fast
		{
			pr = 1;
		}
		// Otherwise recurse more
		else
		{
			pr = getWaterPressure(p2, highest_y, recur_count);
			if(pr == -1)
				continue;
		}

		// If block is at top, pressure here is one higher
		if(i == 0)
		{
			if(pr < 255)
				pr++;
		}
		// If block is at bottom, pressure here is one lower
		else if(i == 5)
		{
			if(pr > 1)
				pr--;
		}
		
		// Node is on the pressure route
		m_flags[m_area.index(p)] |= VOXELFLAG_CHECKED4;

		// Got pressure
		return pr;
	}
	
	// Nothing useful found
	return -1;
}

void VoxelManipulator::spreadWaterPressure(v3s16 p, int pr,
		VoxelArea request_area,
		core::map<v3s16, u8> &active_nodes,
		int recur_count)
{
	//if(recur_count > 10000)
		/*throw ProcessingLimitException
				("spreadWaterPressure recur_count limit reached");*/
	if(recur_count > 10)
		return;
	recur_count++;
	
	/*dstream<<"spreadWaterPressure: p=("
			<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<", oldpr="<<(int)m_data[m_area.index(p)].pressure
			<<", pr="<<pr
			<<", recur_count="<<recur_count
			<<", request_area=";
	request_area.print(dstream);
	dstream<<std::endl;*/

	m_flags[m_area.index(p)] |= VOXELFLAG_CHECKED3;
	m_data[m_area.index(p)].pressure = pr;

	v3s16 dirs[6] = {
		v3s16(0,1,0), // top
		v3s16(-1,0,0), // left
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,0,1), // back
		v3s16(0,-1,0), // bottom
	};

	// Load neighboring nodes
	emerge(VoxelArea(p - v3s16(1,1,1), p + v3s16(1,1,1)), 2);

	s32 i;
	for(i=0; i<6; i++)
	{
		v3s16 p2 = p + dirs[i];
		
		u8 f = m_flags[m_area.index(p2)];

		// Ignore inexistent and checked nodes
		if(f & (VOXELFLAG_INEXISTENT | VOXELFLAG_CHECKED3))
			continue;

		MapNode &n = m_data[m_area.index(p2)];
		
		/*
			If material is air:
				add to active_nodes if there is flow-causing pressure.
			NOTE: Do not remove anything from there. We cannot know
			      here if some other neighbor of it causes flow.
		*/
		if(liquid_replaces_content(n.d))
		{
			bool pressure_causes_flow = false;
			// If empty block is at top
			if(i == 0)
			{
				if(m_disable_water_climb)
					continue;
				
				//if(pr >= PRESERVE_WATER_VOLUME ? 3 : 2)
				if(pr >= 3)
					pressure_causes_flow = true;
			}
			// If block is at bottom
			else if(i == 5)
			{
				pressure_causes_flow = true;
			}
			// If block is at side
			else
			{
				//if(pr >= PRESERVE_WATER_VOLUME ? 2 : 1)
				if(pr >= 2)
					pressure_causes_flow = true;
			}
			
			if(pressure_causes_flow)
			{
				active_nodes[p2] = 1;
			}

			continue;
		}

		// Ignore non-liquid nodes
		if(content_liquid(n.d) == false)
			continue;

		int pr2 = pr;
		// If block is at top, pressure there is lower
		if(i == 0)
		{
			if(pr2 > 0)
				pr2--;
		}
		// If block is at bottom, pressure there is higher
		else if(i == 5)
		{
			if(pr2 < 255)
				pr2++;
		}

		/*if(m_disable_water_climb)
		{
			if(pr2 > 3)
				pr2 = 3;
		}*/
		
		// Ignore if correct pressure is already set and is not on
		// request_area.
		// Thus, request_area can be used for updating as much
		// pressure info in some area as possible to possibly
		// make some calls to getWaterPressure unnecessary.
		if(n.pressure == pr2 && request_area.contains(p2) == false)
			continue;

		spreadWaterPressure(p2, pr2, request_area, active_nodes, recur_count);
	}
}

void VoxelManipulator::updateAreaWaterPressure(VoxelArea a,
		core::map<v3s16, u8> &active_nodes,
		bool checked3_is_clear)
{
	TimeTaker timer("updateAreaWaterPressure", g_device,
			&updateareawaterpressure_time);

	emerge(a, 3);
	
	bool checked2_clear = false;
	
	if(checked3_is_clear == false)
	{
		//clearFlag(VOXELFLAG_CHECKED3);

		clearFlag(VOXELFLAG_CHECKED3 | VOXELFLAG_CHECKED2);
		checked2_clear = true;
	}
	

	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);

		u8 f = m_flags[m_area.index(p)];
		// Ignore inexistent or checked nodes
		if(f & (VOXELFLAG_INEXISTENT | VOXELFLAG_CHECKED3))
			continue;
		MapNode &n = m_data[m_area.index(p)];
		// Ignore non-liquid nodes
		if(content_liquid(n.d) == false)
			continue;
		
		if(checked2_clear == false)
		{
			clearFlag(VOXELFLAG_CHECKED2);
			checked2_clear = true;
		}

		checked2_clear = false;

		s16 highest_y = -32768;
		int recur_count = 0;
		int pr = -1;

		try
		{
			// 0-1ms @ recur_count <= 100
			//TimeTaker timer("getWaterPressure", g_device);
			pr = getWaterPressure(p, highest_y, recur_count);
		}
		catch(ProcessingLimitException &e)
		{
			//dstream<<"getWaterPressure ProcessingLimitException"<<std::endl;
		}

		if(pr == -1)
		{
			assert(highest_y != -32768);

			pr = highest_y - p.Y + 1;
			if(pr > 255)
				pr = 255;

			/*dstream<<"WARNING: Pressure at ("
					<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<" = "<<pr
					//<<" and highest_y == -32768"
					<<std::endl;
			assert(highest_y != -32768);
			continue;*/
		}
		
		try
		{
			// 0ms
			//TimeTaker timer("spreadWaterPressure", g_device);
			spreadWaterPressure(p, pr, a, active_nodes, 0);
		}
		catch(ProcessingLimitException &e)
		{
			//dstream<<"getWaterPressure ProcessingLimitException"<<std::endl;
		}
	}
}

bool VoxelManipulator::flowWater(v3s16 removed_pos,
		core::map<v3s16, u8> &active_nodes,
		int recursion_depth, bool debugprint,
		u32 stoptime)
{
	v3s16 dirs[6] = {
		v3s16(0,1,0), // top
		v3s16(0,0,-1), // front
		v3s16(0,0,1), // back
		v3s16(-1,0,0), // left
		v3s16(1,0,0), // right
		v3s16(0,-1,0), // bottom
	};

	recursion_depth++;

	v3s16 p;
	bool from_ocean = false;
	
	// Randomize horizontal order
	static s32 cs = 0;
	if(cs < 3)
		cs++;
	else
		cs = 0;
	s16 s1 = (cs & 1) ? 1 : -1;
	s16 s2 = (cs & 2) ? 1 : -1;
	//dstream<<"s1="<<s1<<", s2="<<s2<<std::endl;

	{
	TimeTaker timer1("flowWater pre", g_device, &flowwater_pre_time);
	
	// Load neighboring nodes
	emerge(VoxelArea(removed_pos - v3s16(1,1,1), removed_pos + v3s16(1,1,1)), 4);
	
	// Ignore incorrect removed_pos
	{
		u8 f = m_flags[m_area.index(removed_pos)];
		// Ignore inexistent or checked node
		if(f & (VOXELFLAG_INEXISTENT | VOXELFLAG_CHECKED))
			return false;
		MapNode &n = m_data[m_area.index(removed_pos)];
		// Ignore nodes to which the water can't go
		if(liquid_replaces_content(n.d) == false)
			return false;
	}
	
	s32 i;
	for(i=0; i<6; i++)
	{
		// Don't raise water from bottom
		if(m_disable_water_climb && i == 5)
			continue;

		p = removed_pos + v3s16(s1*dirs[i].X, dirs[i].Y, s2*dirs[i].Z);

		u8 f = m_flags[m_area.index(p)];
		// Inexistent or checked nodes can't move
		if(f & (VOXELFLAG_INEXISTENT | VOXELFLAG_CHECKED))
			continue;
		MapNode &n = m_data[m_area.index(p)];
		// Only liquid nodes can move
		if(content_liquid(n.d) == false)
			continue;
		// If block is at top, select it always
		if(i == 0)
		{
			break;
		}
		// If block is at bottom, select it if it has enough pressure
		if(i == 5)
		{
			//if(n.pressure >= PRESERVE_WATER_VOLUME ? 3 : 2)
			if(n.pressure >= 3)
				break;
			continue;
		}
		// Else block is at some side. Select it if it has enough pressure
		//if(n.pressure >= PRESERVE_WATER_VOLUME ? 2 : 1)
		if(n.pressure >= 2)
		{
			break;
		}
	}

	// If there is nothing to move, return
	if(i==6)
		return false;

	/*
		Move water and bubble
	*/

	u8 m = m_data[m_area.index(p)].d;
	u8 f = m_flags[m_area.index(p)];

	if(m == CONTENT_OCEAN)
		from_ocean = true;

	// Move air bubble if not taking water from ocean
	if(from_ocean == false)
	{
		m_data[m_area.index(p)].d = m_data[m_area.index(removed_pos)].d;
		m_flags[m_area.index(p)] = m_flags[m_area.index(removed_pos)];
	}
	
	/*
		This has to be done to copy the brightness of a light source
		correctly. Otherwise unspreadLight will fuck up when water
		has replaced a light source.
	*/
	u8 light = m_data[m_area.index(removed_pos)].getLightBanksWithSource();

	m_data[m_area.index(removed_pos)].d = m;
	m_flags[m_area.index(removed_pos)] = f;

	m_data[m_area.index(removed_pos)].setLightBanks(light);
	
	/*// NOTE: HACK: This has to be set to LIGHT_MAX so that
	// unspreadLight will clear all light that came from this node.
	// Otherwise there will be weird bugs
	m_data[m_area.index(removed_pos)].setLight(LIGHT_MAX);*/

	// Mark removed_pos checked
	m_flags[m_area.index(removed_pos)] |= VOXELFLAG_CHECKED;

	// If block was dropped from surface, increase pressure
	if(i == 0 && m_data[m_area.index(removed_pos)].pressure == 1)
	{
		m_data[m_area.index(removed_pos)].pressure = 2;
	}
	
	/*
	NOTE: This does not work as-is
	if(m == CONTENT_OCEAN)
	{
		// If block was raised to surface, increase pressure of
		// source node
		if(i == 5 && m_data[m_area.index(p)].pressure == 1)
		{
			m_data[m_area.index(p)].pressure = 2;
		}
	}*/
	
	/*if(debugprint)
	{
		dstream<<"VoxelManipulator::flowWater(): Moved bubble:"<<std::endl;
		print(dstream, VOXELPRINT_WATERPRESSURE);
	}*/

	// Update pressure
	VoxelArea a;
	a.addPoint(p - v3s16(1,1,1));
	a.addPoint(p + v3s16(1,1,1));
	a.addPoint(removed_pos - v3s16(1,1,1));
	a.addPoint(removed_pos + v3s16(1,1,1));
	updateAreaWaterPressure(a, active_nodes);
	
	/*if(debugprint)
	{
		dstream<<"VoxelManipulator::flowWater(): Pressure updated:"<<std::endl;
		print(dstream, VOXELPRINT_WATERPRESSURE);
		//std::cin.get();
	}*/

	if(debugprint)
	{
		dstream<<"VoxelManipulator::flowWater(): step done:"<<std::endl;
		print(dstream, VOXELPRINT_WATERPRESSURE);
		//std::cin.get();
	}
	
	}//timer1
	
	//if(PRESERVE_WATER_VOLUME)
	if(from_ocean == false)
	{
		// Flow water to the newly created empty position
		/*flowWater(p, active_nodes, recursion_depth,
				debugprint, counter, counterlimit);*/
		flowWater(p, active_nodes, recursion_depth,
				debugprint, stoptime);
	}
	
	if(stoptime != 0 && g_device != NULL)
	{
		u32 timenow = g_device->getTimer()->getRealTime();
		if(timenow >= stoptime ||
				(stoptime < 0x80000000 && timenow > 0x80000000))
		{
			dstream<<"flowWater: stoptime reached"<<std::endl;
			throw ProcessingLimitException("flowWater stoptime reached");
		}
	}
	
find_again:
	
	// Try flowing water to empty positions around removed_pos.
	// They are checked in reverse order compared to the previous loop.
	for(s32 i=5; i>=0; i--)
	{
		// Don't try to flow to top
		if(m_disable_water_climb && i == 0)
			continue;

		//v3s16 p = removed_pos + dirs[i];
		p = removed_pos + v3s16(s1*dirs[i].X, dirs[i].Y, s2*dirs[i].Z);

		u8 f = m_flags[m_area.index(p)];
		// Water can't move to inexistent nodes
		if(f & VOXELFLAG_INEXISTENT)
			continue;
		MapNode &n = m_data[m_area.index(p)];
		// Water can only move to air
		if(liquid_replaces_content(n.d) == false)
			continue;
			
		// Flow water to node
		bool moved =
		flowWater(p, active_nodes, recursion_depth,
				debugprint, stoptime);
		/*flowWater(p, active_nodes, recursion_depth,
				debugprint, counter, counterlimit);*/
		
		if(moved)
		{
			// Search again from all neighbors
			goto find_again;
		}
	}

	return true;
}

void VoxelManipulator::flowWater(
		core::map<v3s16, u8> &active_nodes,
		int recursion_depth, bool debugprint,
		u32 timelimit)
{
	addarea_time = 0;
	emerge_time = 0;
	emerge_load_time = 0;
	clearflag_time = 0;
	updateareawaterpressure_time = 0;
	flowwater_pre_time = 0;

	if(active_nodes.size() == 0)
	{
		dstream<<"flowWater: no active nodes"<<std::endl;
		return;
	}

	//TimeTaker timer1("flowWater (active_nodes)", g_device);

	//dstream<<"active_nodes.size() = "<<active_nodes.size()<<std::endl;


	u32 stoptime = 0;
	if(g_device != NULL)
	{
		stoptime = g_device->getTimer()->getRealTime() + timelimit;
	}

	// Count of handled active nodes
	u32 handled_count = 0;

	try
	{

	/*
		Take random one at first

		This is randomized only at the first time so that all
		subsequent nodes will be taken at roughly the same position
	*/
	s32 k = 0;
	if(active_nodes.size() != 0)
		k = (s32)rand() % (s32)active_nodes.size();

	// Flow water to active nodes
	for(;;)
	//for(s32 h=0; h<1; h++)
	{
		if(active_nodes.size() == 0)
			break;

		handled_count++;
		
		// Clear check flags
		clearFlag(VOXELFLAG_CHECKED);
		
		//dstream<<"Selecting a new active_node"<<std::endl;

#if 0
		// Take first one
		core::map<v3s16, u8>::Node
				*n = active_nodes.getIterator().getNode();
#endif

#if 1
		
		core::map<v3s16, u8>::Iterator
				i = active_nodes.getIterator().getNode();
		for(s32 j=0; j<k; j++)
		{
			i++;
		}
		core::map<v3s16, u8>::Node *n = i.getNode();

		// Decrement index if less than 0.
		// This keeps us in existing indices always.
		if(k > 0)
			k--;
#endif

		v3s16 p = n->getKey();
		active_nodes.remove(p);
		flowWater(p, active_nodes, recursion_depth,
				debugprint, stoptime);
	}

	}
	catch(ProcessingLimitException &e)
	{
		//dstream<<"getWaterPressure ProcessingLimitException"<<std::endl;
	}
	
	/*v3s16 e = m_area.getExtent();
	s32 v = m_area.getVolume();
	dstream<<"flowWater (active): "
			<<"area ended up as "
			<<e.X<<"x"<<e.Y<<"x"<<e.Z<<" = "<<v
			<<", handled a_node count: "<<handled_count
			<<", active_nodes.size() = "<<active_nodes.size()
			<<std::endl;
	dstream<<"addarea_time: "<<addarea_time
			<<", emerge_time: "<<emerge_time
			<<", emerge_load_time: "<<emerge_load_time
			<<", clearflag_time: "<<clearflag_time
			<<", flowwater_pre_time: "<<flowwater_pre_time
			<<", updateareawaterpressure_time: "<<updateareawaterpressure_time
			<<std::endl;*/
}


//END
