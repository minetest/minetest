/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "map.h"
#include "mapsector.h"

class DummyMap : public Map
{
public:
	DummyMap(IGameDef *gamedef, v3s16 bpmin, v3s16 bpmax): Map(gamedef)
	{
		for (s16 z = bpmin.Z; z <= bpmax.Z; z++)
		for (s16 x = bpmin.X; x <= bpmax.X; x++) {
			v2s16 p2d(x, z);
			MapSector *sector = new MapSector(this, p2d, gamedef);
			m_sectors[p2d] = sector;
			for (s16 y = bpmin.Y; y <= bpmax.Y; y++)
				sector->createBlankBlock(y);
		}
	}

	~DummyMap() = default;

	bool maySaveBlocks() override { return false; }
};
