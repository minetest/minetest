/*
Minetest
Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "face_position_cache.h"
#include "threading/mutex_auto_lock.h"


std::map<u16, std::vector<v3s16> > FacePositionCache::cache;
Mutex FacePositionCache::cache_mutex;

// Calculate the borders of a "d-radius" cube
const std::vector<v3s16> &FacePositionCache::getFacePositions(u16 d)
{
	MutexAutoLock lock(cache_mutex);
	std::map<u16, std::vector<v3s16> >::iterator it = cache.find(d);
	if (it != cache.end())
		return it->second;

	return generateFacePosition(d);
}

const std::vector<v3s16> &FacePositionCache::generateFacePosition(u16 d)
{
	cache[d] = std::vector<v3s16>();
	std::vector<v3s16> &c = cache[d];
	if (d == 0) {
		c.push_back(v3s16(0,0,0));
		return c;
	}
	if (d == 1) {
		// This is an optimized sequence of coordinates.
		c.push_back(v3s16( 0, 1, 0)); // top
		c.push_back(v3s16( 0, 0, 1)); // back
		c.push_back(v3s16(-1, 0, 0)); // left
		c.push_back(v3s16( 1, 0, 0)); // right
		c.push_back(v3s16( 0, 0,-1)); // front
		c.push_back(v3s16( 0,-1, 0)); // bottom
		// 6
		c.push_back(v3s16(-1, 0, 1)); // back left
		c.push_back(v3s16( 1, 0, 1)); // back right
		c.push_back(v3s16(-1, 0,-1)); // front left
		c.push_back(v3s16( 1, 0,-1)); // front right
		c.push_back(v3s16(-1,-1, 0)); // bottom left
		c.push_back(v3s16( 1,-1, 0)); // bottom right
		c.push_back(v3s16( 0,-1, 1)); // bottom back
		c.push_back(v3s16( 0,-1,-1)); // bottom front
		c.push_back(v3s16(-1, 1, 0)); // top left
		c.push_back(v3s16( 1, 1, 0)); // top right
		c.push_back(v3s16( 0, 1, 1)); // top back
		c.push_back(v3s16( 0, 1,-1)); // top front
		// 18
		c.push_back(v3s16(-1, 1, 1)); // top back-left
		c.push_back(v3s16( 1, 1, 1)); // top back-right
		c.push_back(v3s16(-1, 1,-1)); // top front-left
		c.push_back(v3s16( 1, 1,-1)); // top front-right
		c.push_back(v3s16(-1,-1, 1)); // bottom back-left
		c.push_back(v3s16( 1,-1, 1)); // bottom back-right
		c.push_back(v3s16(-1,-1,-1)); // bottom front-left
		c.push_back(v3s16( 1,-1,-1)); // bottom front-right
		// 26
		return c;
	}

	// Take blocks in all sides, starting from y=0 and going +-y
	for (s16 y = 0; y <= d - 1; y++) {
		// Left and right side, including borders
		for (s16 z =- d; z <= d; z++) {
			c.push_back(v3s16( d, y, z));
			c.push_back(v3s16(-d, y, z));
			if (y != 0) {
				c.push_back(v3s16( d, -y, z));
				c.push_back(v3s16(-d, -y, z));
			}
		}
		// Back and front side, excluding borders
		for (s16 x = -d + 1; x <= d - 1; x++) {
			c.push_back(v3s16(x, y, d));
			c.push_back(v3s16(x, y, -d));
			if (y != 0) {
				c.push_back(v3s16(x, -y, d));
				c.push_back(v3s16(x, -y, -d));
			}
		}
	}

	// Take the bottom and top face with borders
	// -d < x < d, y = +-d, -d < z < d
	for (s16 x = -d; x <= d; x++)
	for (s16 z = -d; z <= d; z++) {
		c.push_back(v3s16(x, -d, z));
		c.push_back(v3s16(x, d, z));
	}
	return c;
}

