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

#pragma once

/* Ensure that <stdint.h> is included before <irrTypes.h>, unless building on
 * MSVC, to address an irrlicht issue: https://sourceforge.net/p/irrlicht/bugs/433/
 *
 * TODO: Decide whether or not we support non-compliant C++ compilers like old
 *       versions of MSCV.  If we do not then <stdint.h> can always be included
 *       regardless of the compiler.
 */
#ifndef _MSC_VER
#	include <cstdint>
#endif

#include <irrTypes.h>

using namespace irr;

namespace irr {

// Irrlicht 1.8+ defines 64bit unsigned symbol in irrTypes.h
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8)
#ifdef _MSC_VER
	// Windows
	typedef long long s64;
	typedef unsigned long long u64;
#else
	// Posix
	typedef int64_t s64;
	typedef uint64_t u64;
#endif
#endif

#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >= 9)
namespace core {
	template <typename T>
	inline T roundingError();

	template <>
	inline s16 roundingError()
	{
		return 0;
	}
}
#endif

}

#define S8_MIN  (-0x7F - 1)
#define S16_MIN (-0x7FFF - 1)
#define S32_MIN (-0x7FFFFFFF - 1)
#define S64_MIN (-0x7FFFFFFFFFFFFFFF - 1)

#define S8_MAX  0x7F
#define S16_MAX 0x7FFF
#define S32_MAX 0x7FFFFFFF
#define S64_MAX 0x7FFFFFFFFFFFFFFF

#define U8_MAX  0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF
