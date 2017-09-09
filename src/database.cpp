/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "database.h"
#include "irrlichttypes.h"


/* The position hashing is done by the
 * straightforward function getBlockAsInteger.
 *
 * To invert it, we use unsigned_to_signed to get
 * X, Y, Z in that order, since knowing the low
 * 12 bits of the hash uniquely determines X, and
 * the we can iterate to get Y then Z.
 */

static inline s16 unsigned_to_signed(u16 i, u16 max_positive)
{
	if (i < max_positive) {
		return i;
	}

	return i - (max_positive * 2);
}

s64 MapDatabase::getBlockAsInteger(const v3s16 &pos)
{
	return (u64) pos.Z * 0x1000000 +
		(u64) pos.Y * 0x1000 +
		(u64) pos.X;
}


v3s16 MapDatabase::getIntegerAsBlock(s64 i)
{
	v3s16 pos;
	pos.X = unsigned_to_signed(i & 0xfff, 2048);
	i = (i - pos.X) / 4096;
	pos.Y = unsigned_to_signed(i & 0xfff, 2048);
	i = (i - pos.Y) / 4096;
	pos.Z = unsigned_to_signed(i & 0xfff, 2048);
	return pos;
}

