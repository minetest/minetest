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

#include "numeric.h"
#include "mathconstants.h"

#include "../log.h"
#include "../constants.h" // BS, MAP_BLOCKSIZE
#include <string.h>
#include <iostream>

// Calculate the borders of a "d-radius" cube
void getFacePositions(std::list<v3s16> &list, u16 d)
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

// 64-bit unaligned version of MurmurHash
u64 murmur_hash_64_ua(const void *key, int len, unsigned int seed)
{
	const u64 m = 0xc6a4a7935bd1e995ULL;
	const int r = 47;
	u64 h = seed ^ (len * m);

	const u64 *data = (const u64 *)key;
	const u64 *end = data + (len / 8);

	while (data != end) {
		u64 k;
		memcpy(&k, data, sizeof(u64));
		data++;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h ^= k;
		h *= m; 
	}

	const unsigned char *data2 = (const unsigned char *)data;
	switch (len & 7) {
		case 7: h ^= (u64)data2[6] << 48;
		case 6: h ^= (u64)data2[5] << 40;
		case 5: h ^= (u64)data2[4] << 32;
		case 4: h ^= (u64)data2[3] << 24;
		case 3: h ^= (u64)data2[2] << 16;
		case 2: h ^= (u64)data2[1] << 8;
		case 1: h ^= (u64)data2[0];
				h *= m;
	}
 
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	
	return h;
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

	// Total distance
	f32 d = blockpos_relative.getLength();

	if(distance_ptr)
		*distance_ptr = d;
	
	// If block is far away, it's not in sight
	if(d > range)
		return false;

	// Maximum radius of a block.  The magic number is
	// sqrt(3.0) / 2.0 in literal form.
	f32 block_max_radius = 0.866025403784 * MAP_BLOCKSIZE * BS;
	
	// If block is (nearly) touching the camera, don't
	// bother validating further (that is, render it anyway)
	if(d < block_max_radius)
		return true;

	// Adjust camera position, for purposes of computing the angle,
	// such that a block that has any portion visible with the
	// current camera position will have the center visible at the
	// adjusted postion
	f32 adjdist = block_max_radius / cos((M_PI - camera_fov) / 2);

	// Block position relative to adjusted camera
	v3f blockpos_adj = blockpos - (camera_pos - camera_dir * adjdist);

	// Distance in camera direction (+=front, -=back)
	f32 dforward = blockpos_adj.dotProduct(camera_dir);

	// Cosine of the angle between the camera direction
	// and the block direction (camera_dir is an unit vector)
	f32 cosangle = dforward / blockpos_adj.getLength();
	
	// If block is not in the field of view, skip it
	if(cosangle < cos(camera_fov / 2))
		return false;

	return true;
}

