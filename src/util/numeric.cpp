/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "numeric.h"

#include "../log.h"
#include "../constants.h" // BS, MAP_BLOCKSIZE
#include <iostream>

// Calculate the borders of a "d-radius" cube
void getFacePositions(core::list<v3s16> &list, u16 d)
{
	if(d == 0)
	{
		list.push_back(v3s16(0,0,0));
		return;
	}
	if(d == 1)
	{
		/*
			This is an optimized sequence of coordinates.
		*/
		list.push_back(v3s16( 0, 1, 0)); // top
		list.push_back(v3s16( 0, 0, 1)); // back
		list.push_back(v3s16(-1, 0, 0)); // left
		list.push_back(v3s16( 1, 0, 0)); // right
		list.push_back(v3s16( 0, 0,-1)); // front
		list.push_back(v3s16( 0,-1, 0)); // bottom
		// 6
		list.push_back(v3s16(-1, 0, 1)); // back left
		list.push_back(v3s16( 1, 0, 1)); // back right
		list.push_back(v3s16(-1, 0,-1)); // front left
		list.push_back(v3s16( 1, 0,-1)); // front right
		list.push_back(v3s16(-1,-1, 0)); // bottom left
		list.push_back(v3s16( 1,-1, 0)); // bottom right
		list.push_back(v3s16( 0,-1, 1)); // bottom back
		list.push_back(v3s16( 0,-1,-1)); // bottom front
		list.push_back(v3s16(-1, 1, 0)); // top left
		list.push_back(v3s16( 1, 1, 0)); // top right
		list.push_back(v3s16( 0, 1, 1)); // top back
		list.push_back(v3s16( 0, 1,-1)); // top front
		// 18
		list.push_back(v3s16(-1, 1, 1)); // top back-left
		list.push_back(v3s16( 1, 1, 1)); // top back-right
		list.push_back(v3s16(-1, 1,-1)); // top front-left
		list.push_back(v3s16( 1, 1,-1)); // top front-right
		list.push_back(v3s16(-1,-1, 1)); // bottom back-left
		list.push_back(v3s16( 1,-1, 1)); // bottom back-right
		list.push_back(v3s16(-1,-1,-1)); // bottom front-left
		list.push_back(v3s16( 1,-1,-1)); // bottom front-right
		// 26
		return;
	}

	// Take blocks in all sides, starting from y=0 and going +-y
	for(s16 y=0; y<=d-1; y++)
	{
		// Left and right side, including borders
		for(s16 z=-d; z<=d; z++)
		{
			list.push_back(v3s16(d,y,z));
			list.push_back(v3s16(-d,y,z));
			if(y != 0)
			{
				list.push_back(v3s16(d,-y,z));
				list.push_back(v3s16(-d,-y,z));
			}
		}
		// Back and front side, excluding borders
		for(s16 x=-d+1; x<=d-1; x++)
		{
			list.push_back(v3s16(x,y,d));
			list.push_back(v3s16(x,y,-d));
			if(y != 0)
			{
				list.push_back(v3s16(x,-y,d));
				list.push_back(v3s16(x,-y,-d));
			}
		}
	}

	// Take the bottom and top face with borders
	// -d<x<d, y=+-d, -d<z<d
	for(s16 x=-d; x<=d; x++)
	for(s16 z=-d; z<=d; z++)
	{
		list.push_back(v3s16(x,-d,z));
		list.push_back(v3s16(x,d,z));
	}
}

/*
    myrand
*/

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

int myrand_range(int min, int max)
{
	if(max-min > MYRAND_MAX)
	{
		errorstream<<"WARNING: myrand_range: max-min > MYRAND_MAX"<<std::endl;
        max = min + MYRAND_MAX;
	}
	if(min > max)
	{
		errorstream<<"WARNING: myrand_range: min > max"<<std::endl;
		return max;
	}
	return (myrand()%(max-min+1))+min;
}

/*
	blockpos: position of block in block coordinates
	camera_pos: position of camera in nodes
	camera_dir: an unit vector pointing to camera direction
	range: viewing range
*/
bool isBlockInSight(v3s16 blockpos_b, v3f camera_pos, v3f camera_dir,
		f32 camera_fov, f32 range, f32 *distance_ptr)
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

	if(distance_ptr)
		*distance_ptr = d;
	
	// If block is very close, it is always in sight
	if(d < 1.44*1.44*MAP_BLOCKSIZE*BS/2)
		return true;

	// If block is far away, it's not in sight
	if(d > range)
		return false;

	// Maximum radius of a block
	f32 block_max_radius = 0.5*1.44*1.44*MAP_BLOCKSIZE*BS;
	
	// If block is (nearly) touching the camera, don't
	// bother validating further (that is, render it anyway)
	if(d < block_max_radius)
		return true;
	
	// Cosine of the angle between the camera direction
	// and the block direction (camera_dir is an unit vector)
	f32 cosangle = dforward / d;
	
	// Compensate for the size of the block
	// (as the block has to be shown even if it's a bit off FOV)
	// This is an estimate, plus an arbitary factor
	cosangle += block_max_radius / d * 0.5;

	// If block is not in the field of view, skip it
	if(cosangle < cos(camera_fov / 2))
		return false;

	return true;
}

