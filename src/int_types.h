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

#ifndef INT_TYPES_HEADER
#define INT_TYPES_HEADER

#if defined(_MSC_VER) && _MSC_VER >= 1310 && _MSC_VER <= 1600
	// MSVC 2003-2010
	typedef   signed __int8  s8;
	typedef unsigned __int8  u8;
	typedef   signed __int16 s16;
	typedef unsigned __int16 u16;
	typedef   signed __int32 s32;
	typedef unsigned __int32 u32;
	typedef   signed __int32 s64;
	typedef unsigned __int32 u64;
#else
	// C99 / Posix
	#include <stdint.h>
	typedef  int8_t  s8;
	typedef uint8_t  u8;
	typedef  int16_t s16;
	typedef uint16_t u16;
	typedef  int32_t s32;
	typedef uint32_t u32;
	typedef  int64_t s64;
	typedef uint64_t u64;
#endif

#ifndef UINT8_MAX
	#define UINT8_MAX  0xFF
	#define UINT16_MAX 0xFFFF
	#define UINT32_MAX 0xFFFFFFFFU
	#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL
	#define INT8_MAX  0x7F
	#define INT16_MAX 0x7FFF
	#define INT32_MAX 0x7FFFFFFF
	#define INT64_MAX 0x7FFFFFFFFFFFFFFFLL
	#define INT8_MIN  (-INT8_MAX-1)
	#define INT16_MIN (-INT16_MAX-1)
	#define INT32_MIN (-INT32_MAX-1)
	#define INT64_MIN (-INT64_MAX-1)
#endif

// This is for compatability.  This definition is the same as Irrlicht's,
// but it isn't actually guaranteed to be 32 bits.
typedef float f32;

#endif

