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

/*
 * IrrlichtMt already includes stdint.h in irrTypes.h. This works everywhere
 * we need it to (including recent MSVC), so should be fine here too.
 */
#include <cstdint>

#include <irrTypes.h>

using namespace irr;

namespace irr {

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
