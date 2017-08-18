/*
Minetest
Copyright (C) 2015 Nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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


std::unordered_map<u16, std::vector<v3s16>> FacePositionCache::cache;
std::mutex FacePositionCache::cache_mutex;

// Calculate the borders of a "d-radius" cube
const std::vector<v3s16> &FacePositionCache::getFacePositions(u16 d)
{
	MutexAutoLock lock(cache_mutex);
	std::unordered_map<u16, std::vector<v3s16>>::const_iterator it = cache.find(d);
	if (it != cache.end())
		return it->second;

	return generateFacePosition(d);
}

const std::vector<v3s16> &FacePositionCache::generateFacePosition(u16 d)
{
	cache[d] = std::vector<v3s16>();
	std::vector<v3s16> &c = cache[d];
	if (d == 0) {
		c.emplace_back(0,0,0);
		return c;
	}
	if (d == 1) {
		// This is an optimized sequence of coordinates.
		c.emplace_back(0, 1, 0); // Top
		c.emplace_back(0, 0, 1); // Back
		c.emplace_back(-1, 0, 0); // Left
		c.emplace_back(1, 0, 0); // Right
		c.emplace_back(0, 0,-1); // Front
		c.emplace_back(0,-1, 0); // Bottom
		// 6
		c.emplace_back(-1, 0, 1); // Back left
		c.emplace_back(1, 0, 1); // Back right
		c.emplace_back(-1, 0,-1); // Front left
		c.emplace_back(1, 0,-1); // Front right
		c.emplace_back(-1,-1, 0); // Bottom left
		c.emplace_back(1,-1, 0); // Bottom right
		c.emplace_back(0,-1, 1); // Bottom back
		c.emplace_back(0,-1,-1); // Bottom front
		c.emplace_back(-1, 1, 0); // Top left
		c.emplace_back(1, 1, 0); // Top right
		c.emplace_back(0, 1, 1); // Top back
		c.emplace_back(0, 1,-1); // Top front
		// 18
		c.emplace_back(-1, 1, 1); // Top back-left
		c.emplace_back(1, 1, 1); // Top back-right
		c.emplace_back(-1, 1,-1); // Top front-left
		c.emplace_back(1, 1,-1); // Top front-right
		c.emplace_back(-1,-1, 1); // Bottom back-left
		c.emplace_back(1,-1, 1); // Bottom back-right
		c.emplace_back(-1,-1,-1); // Bottom front-left
		c.emplace_back(1,-1,-1); // Bottom front-right
		// 26
		return c;
	}

	// Take blocks in all sides, starting from y=0 and going +-y
	for (s16 y = 0; y <= d - 1; y++) {
		// Left and right side, including borders
		for (s16 z =- d; z <= d; z++) {
			c.emplace_back(d, y, z);
			c.emplace_back(-d, y, z);
			if (y != 0) {
				c.emplace_back(d, -y, z);
				c.emplace_back(-d, -y, z);
			}
		}
		// Back and front side, excluding borders
		for (s16 x = -d + 1; x <= d - 1; x++) {
			c.emplace_back(x, y, d);
			c.emplace_back(x, y, -d);
			if (y != 0) {
				c.emplace_back(x, -y, d);
				c.emplace_back(x, -y, -d);
			}
		}
	}

	// Take the bottom and top face with borders
	// -d < x < d, y = +-d, -d < z < d
	for (s16 x = -d; x <= d; x++)
	for (s16 z = -d; z <= d; z++) {
		c.emplace_back(x, -d, z);
		c.emplace_back(x, d, z);
	}
	return c;
}
