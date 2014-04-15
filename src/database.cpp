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

static inline s16 unsigned_to_signed(u16 i, u16 max_positive)
{
	if (i < max_positive) {
		return i;
	} else {
		return i - (max_positive * 2);
	}
}


s64 Database::getBlockAsInteger(const v3s16 pos) const
{
	return (u64) pos.Z * 0x1000000 +
		(u64) pos.Y * 0x1000 +
		(u64) pos.X;
}

v3s16 Database::getIntegerAsBlock(const s64 i) const
{
        v3s16 pos;
        pos.Z = unsigned_to_signed((i >> 24) & 0xFFF, 0x1000 / 2);
        pos.Y = unsigned_to_signed((i >> 12) & 0xFFF, 0x1000 / 2);
        pos.X = unsigned_to_signed((i      ) & 0xFFF, 0x1000 / 2);
        return pos;
}

