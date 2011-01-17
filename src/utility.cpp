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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "utility.h"
#include "irrlichtwrapper.h"
#include "gettime.h"
#include "mapblock.h"

TimeTaker::TimeTaker(const char *name, u32 *result)
{
	m_name = name;
	m_result = result;
	m_running = true;
	m_time1 = getTimeMs();
}

u32 TimeTaker::stop(bool quiet)
{
	if(m_running)
	{
		u32 time2 = getTimeMs();
		u32 dtime = time2 - m_time1;
		if(m_result != NULL)
		{
			(*m_result) += dtime;
		}
		else
		{
			if(quiet == false)
				std::cout<<m_name<<" took "<<dtime<<"ms"<<std::endl;
		}
		m_running = false;
		return dtime;
	}
	return 0;
}

const v3s16 g_26dirs[26] =
{
	// +right, +top, +back
	v3s16( 0, 0, 1), // back
	v3s16( 0, 1, 0), // top
	v3s16( 1, 0, 0), // right
	v3s16( 0, 0,-1), // front
	v3s16( 0,-1, 0), // bottom
	v3s16(-1, 0, 0), // left
	// 6
	v3s16(-1, 1, 0), // top left
	v3s16( 1, 1, 0), // top right
	v3s16( 0, 1, 1), // top back
	v3s16( 0, 1,-1), // top front
	v3s16(-1, 0, 1), // back left
	v3s16( 1, 0, 1), // back right
	v3s16(-1, 0,-1), // front left
	v3s16( 1, 0,-1), // front right
	v3s16(-1,-1, 0), // bottom left
	v3s16( 1,-1, 0), // bottom right
	v3s16( 0,-1, 1), // bottom back
	v3s16( 0,-1,-1), // bottom front
	// 18
	v3s16(-1, 1, 1), // top back-left
	v3s16( 1, 1, 1), // top back-right
	v3s16(-1, 1,-1), // top front-left
	v3s16( 1, 1,-1), // top front-right
	v3s16(-1,-1, 1), // bottom back-left
	v3s16( 1,-1, 1), // bottom back-right
	v3s16(-1,-1,-1), // bottom front-left
	v3s16( 1,-1,-1), // bottom front-right
	// 26
};

static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
int myrand(void)
{
   next = next * 1103515245 + 12345;
   return((unsigned)(next/65536) % 32768);
}

void mysrand(unsigned seed)
{
   next = seed;
}

// Float with distance
struct DFloat
{
	float v;
	u32 d;
};

float PointAttributeList::getInterpolatedFloat(v3s16 p)
{
	const u32 near_wanted_count = 5;
	// Last is nearest, first is farthest
	core::list<DFloat> near;

	for(core::list<PointWithAttr>::Iterator
			i = m_points.begin();
			i != m_points.end(); i++)
	{
		PointWithAttr &pwa = *i;
		u32 d = pwa.p.getDistanceFrom(p);
		
		DFloat df;
		df.v = pwa.attr.getFloat();
		df.d = d;
				
		// If near list is empty, add directly and continue
		if(near.size() == 0)
		{
			near.push_back(df);
			continue;
		}
		
		// Get distance of farthest in near list
		u32 near_d = 100000;
		if(near.size() > 0)
		{
			core::list<DFloat>::Iterator i = near.begin();
			near_d = i->d;
		}
		
		/*
			If point is closer than the farthest in the near list or
			there are not yet enough points on the list
		*/
		if(d < near_d || near.size() < near_wanted_count)
		{
			// Find the right place in the near list and put it there
			
			// Go from farthest to near in the near list
			core::list<DFloat>::Iterator i = near.begin();
			for(; i != near.end(); i++)
			{
				// Stop when i is at the first nearer node
				if(i->d < d)
					break;
			}
			// Add df to before i
			if(i == near.end())
				near.push_back(df);
			else
				near.insert_before(i, df);

			// Keep near list at right size
			if(near.size() > near_wanted_count)
			{
				core::list<DFloat>::Iterator j = near.begin();
				near.erase(j);
			}
		}
	}
	
	// Return if no values found
	if(near.size() == 0)
		return 0.0;
	
	/*
20:58:29 < tejeez> joka pisteelle a += arvo / etäisyys^6; b += 1 / etäisyys^6; ja 
lopuks sit otetaan a/b
	*/
	
	float a = 0;
	float b = 0;
	for(core::list<DFloat>::Iterator i = near.begin();
			i != near.end(); i++)
	{
		if(i->d == 0)
			return i->v;
		
		//float dd = pow((float)i->d, 6);
		float dd = pow((float)i->d, 5);
		float v = i->v;
		//dstream<<"dd="<<dd<<", v="<<v<<std::endl;
		a += v / dd;
		b += 1 / dd;
	}

	return a / b;
}

#if 0
float PointAttributeList::getInterpolatedFloat(v3s16 p)
{
	const u32 near_wanted_count = 2;
	const u32 nearest_wanted_count = 2;
	// Last is near
	core::list<DFloat> near;

	for(core::list<PointWithAttr>::Iterator
			i = m_points.begin();
			i != m_points.end(); i++)
	{
		PointWithAttr &pwa = *i;
		u32 d = pwa.p.getDistanceFrom(p);
		
		DFloat df;
		df.v = pwa.attr.getFloat();
		df.d = d;
				
		// If near list is empty, add directly and continue
		if(near.size() == 0)
		{
			near.push_back(df);
			continue;
		}
		
		// Get distance of farthest in near list
		u32 near_d = 100000;
		if(near.size() > 0)
		{
			core::list<DFloat>::Iterator i = near.begin();
			near_d = i->d;
		}
		
		/*
			If point is closer than the farthest in the near list or
			there are not yet enough points on the list
		*/
		if(d < near_d || near.size() < near_wanted_count)
		{
			// Find the right place in the near list and put it there
			
			// Go from farthest to near in the near list
			core::list<DFloat>::Iterator i = near.begin();
			for(; i != near.end(); i++)
			{
				// Stop when i is at the first nearer node
				if(i->d < d)
					break;
			}
			// Add df to before i
			if(i == near.end())
				near.push_back(df);
			else
				near.insert_before(i, df);

			// Keep near list at right size
			if(near.size() > near_wanted_count)
			{
				core::list<DFloat>::Iterator j = near.begin();
				near.erase(j);
			}
		}
	}
	
	// Return if no values found
	if(near.size() == 0)
		return 0.0;
	
	/*
		Get nearest ones
	*/

	u32 nearest_count = nearest_wanted_count;
	if(nearest_count > near.size())
		nearest_count = near.size();
	core::list<DFloat> nearest;
	{
		core::list<DFloat>::Iterator i = near.getLast();
		for(u32 j=0; j<nearest_count; j++)
		{
			nearest.push_front(*i);
			i--;
		}
	}

	/*
		TODO: Try this:
20:58:29 < tejeez> joka pisteelle a += arvo / etäisyys^6; b += 1 / etäisyys^6; ja 
lopuks sit otetaan a/b
	*/

	/*
		Get total distance to nearest points
	*/
	
	float nearest_d_sum = 0;
	for(core::list<DFloat>::Iterator i = nearest.begin();
			i != nearest.end(); i++)
	{
		nearest_d_sum += (float)i->d;
	}

	/*
		Interpolate a value between the first ones
	*/

	dstream<<"nearest.size()="<<nearest.size()<<std::endl;

	float interpolated = 0;
	
	for(core::list<DFloat>::Iterator i = nearest.begin();
			i != nearest.end(); i++)
	{
		float weight;
		if(nearest_d_sum > 0.001)
			weight = (float)i->d / nearest_d_sum;
		else
			weight = 1. / nearest.size();
		/*dstream<<"i->d="<<i->d<<" nearest_d_sum="<<nearest_d_sum
				<<" weight="<<weight<<std::endl;*/
		interpolated += weight * i->v;
	}

	return interpolated;
}
#endif

/*
	blockpos: position of block in block coordinates
	camera_pos: position of camera in nodes
	camera_dir: an unit vector pointing to camera direction
	range: viewing range
*/
bool isBlockInSight(v3s16 blockpos_b, v3f camera_pos, v3f camera_dir, f32 range)
{
	v3s16 blockpos_nodes = blockpos_b * MAP_BLOCKSIZE;
	
	// Block center position
	v3f blockpos(
			((float)blockpos_nodes.X + MAP_BLOCKSIZE/2) * BS,
			((float)blockpos_nodes.Y + MAP_BLOCKSIZE/2) * BS,
			((float)blockpos_nodes.Z + MAP_BLOCKSIZE/2) * BS
	);

	// Block position relative to camera
	v3f blockpos_relative = blockpos - camera_pos;

	// Distance in camera direction (+=front, -=back)
	f32 dforward = blockpos_relative.dotProduct(camera_dir);

	// Total distance
	f32 d = blockpos_relative.getLength();
	
	// If block is far away, it's not in sight
	if(d > range * BS)
		return false;

	// Maximum radius of a block
	f32 block_max_radius = 0.5*1.44*1.44*MAP_BLOCKSIZE*BS;
	
	// If block is (nearly) touching the camera, don't
	// bother validating further (that is, render it anyway)
	if(d > block_max_radius * 1.5)
	{
		// Cosine of the angle between the camera direction
		// and the block direction (camera_dir is an unit vector)
		f32 cosangle = dforward / d;
		
		// Compensate for the size of the block
		// (as the block has to be shown even if it's a bit off FOV)
		// This is an estimate.
		cosangle += block_max_radius / dforward;

		// If block is not in the field of view, skip it
		//if(cosangle < cos(FOV_ANGLE/2))
		if(cosangle < cos(FOV_ANGLE/2. * 4./3.))
			return false;
	}

	return true;
}

