/*
Minetest
Copyright (C) 2016 MillersMan <millersman@users.noreply.github.com>

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

#include "util/container.h"
#include "irrlichttypes_bloated.h"

class NodeDefManager;
class Map;
class MapBlock;

class ReflowScan {
public:
	ReflowScan(Map *map, const NodeDefManager *ndef);
	void scan(MapBlock *block, UniqueQueue<v3s16> *liquid_queue);

private:
	MapBlock *lookupBlock(int x, int y, int z);
	bool isLiquidFlowableTo(int x, int y, int z);
	bool isLiquidHorizontallyFlowable(int x, int y, int z);
	void scanColumn(int x, int z);

private:
	Map *m_map = nullptr;
	const NodeDefManager *m_ndef = nullptr;
	v3s16 m_block_pos, m_rel_block_pos;
	UniqueQueue<v3s16> *m_liquid_queue = nullptr;
	MapBlock *m_lookup[3 * 3 * 3];
	u32 m_lookup_state_bitset;
};
