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

#define S8_MIN  INT8_MIN
#define S16_MIN INT16_MIN
#define S32_MIN INT32_MIN
#define S64_MIN INT64_MIN

#define S8_MAX  INT8_MAX
#define S16_MAX INT16_MAX
#define S32_MAX INT32_MAX
#define S64_MAX INT64_MAX

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX
