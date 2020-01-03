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

#include "liquidlogic.h"
#include "util/container.h"
#include "irrlichttypes_bloated.h"

class ServerEnvironment;
class IGameDef;
class INodeDefManager;
class Map;
class MapBlock;
class MapNode;


class LiquidLogicClassic: public LiquidLogic {
public:
	LiquidLogicClassic(Map *map, IGameDef *gamedef);
	void addTransforming(v3s16 p);
	void scanBlock(MapBlock *block);
	void scanVoxelManip(MMVManip *vm, v3s16 nmin, v3s16 nmax);
	void scanVoxelManip(UniqueQueue<v3s16> *liquid_queue,
		MMVManip *vm, v3s16 nmin, v3s16 nmax);
	void transform(std::map<v3s16, MapBlock*> &modified_blocks,
		ServerEnvironment *env);
	void addTransformingFromData(BlockMakeData *data);
private:
	MapBlock *lookupBlock(int x, int y, int z);
	bool isLiquidFlowableTo(int x, int y, int z);
	bool isLiquidHorizontallyFlowable(int x, int y, int z);
	bool isLiquidHorizontallyFlowable(MMVManip *vm, u32 vi, v3s16 em);
	void scanColumn(int x, int z);

private:
	UniqueQueue<v3s16> m_liquid_queue;

	u32 m_unprocessed_count = 0;
	bool m_queue_size_timer_started = false;
	u64 m_inc_trending_up_start_time = 0; // milliseconds

	v3s16 m_block_pos, m_rel_block_pos;
	MapBlock *m_lookup[3 * 3 * 3];
	u32 m_lookup_state_bitset;
};
