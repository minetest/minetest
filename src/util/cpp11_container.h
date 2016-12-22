/*
Minetest
Copyright (C) 2016 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#ifndef MT_CPP11CONTAINER_HEADER
#define MT_CPP11CONTAINER_HEADER

#if __cplusplus >= 201103L
#define USE_UNORDERED_CONTAINERS
#endif

#if _MSC_VER >= 1600
#define USE_UNORDERED_CONTAINERS
#endif

#ifdef USE_UNORDERED_CONTAINERS
	#include <unordered_map>
	#include <unordered_set>
	#include <functional>
	#include <irr_v3d.h>
	#define UNORDERED_MAP std::unordered_map
	#define UNORDERED_SET std::unordered_set

// Avoid some circular dependencies; declare hash function here
u64 murmur_hash_64_ua(const void *key, int len, unsigned int seed);

// A hash class is required for c++11 unordered containers

namespace std {
	template<>
	struct hash<v3s16> {
		u64 operator()(const v3s16 &pos) const
		{
			s16 data[3] = { pos.X, pos.Y, pos.Z };
			return murmur_hash_64_ua(data, sizeof(data), 0x6543210f);
		}
	};
}

#else
	#include <map>
	#include <set>
	#define UNORDERED_MAP std::map
	#define UNORDERED_SET std::set
#endif

#endif
