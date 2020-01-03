/*
Minetest
Copyright (C) 2019 Pierre-Yves Rollo <dev@pyrollo.com>

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
#include "mapblock.h"

class ServerEnvironment;
class IGameDef;
class NodeDefManager;
class Map;
class MapBlock;
class MMVManip;

class LiquidLogic {
public:
	LiquidLogic(Map *map, IGameDef *gamedef);
	virtual ~LiquidLogic() {};
	virtual void addTransforming(v3s16 p) {};
	virtual void scanBlock(MapBlock *block) {};
	virtual void scanVoxelManip(MMVManip *vm, v3s16 nmin, v3s16 nmax) {};
	virtual void scanVoxelManip(UniqueQueue<v3s16> *liquid_queue,
		MMVManip *vm, v3s16 nmin, v3s16 nmax) {};
	virtual void transform(std::map<v3s16, MapBlock*> &modified_blocks,
		ServerEnvironment *env) {};
	virtual void addTransformingFromData(BlockMakeData *data) {};

protected:
	Map *m_map = nullptr;
	IGameDef *m_gamedef = nullptr;
	const NodeDefManager *m_ndef = nullptr;
};
