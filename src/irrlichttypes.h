/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef IRRLICHTTYPES_HEADER
#define IRRLICHTTYPES_HEADER

#include <irrTypes.h>
#include <vector2d.h>
#include <vector3d.h>
#include <irrMap.h>
#include <irrList.h>
#include <irrArray.h>
#include <aabbox3d.h>
#include <SColor.h>
using namespace irr;
typedef core::vector3df v3f;
typedef core::vector3d<s16> v3s16;
typedef core::vector3d<s32> v3s32;

typedef core::vector2d<f32> v2f;
typedef core::vector2d<s16> v2s16;
typedef core::vector2d<s32> v2s32;
typedef core::vector2d<u32> v2u32;
typedef core::vector2d<f32> v2f32;

typedef core::aabbox3d<f32> aabb3f;

#ifdef _MSC_VER
	// Windows
	typedef unsigned long long u64;
#else
	// Posix
	#include <stdint.h>
	typedef uint64_t u64;
	//typedef unsigned long long u64;
#endif

#endif

