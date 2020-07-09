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

#include "mapblock.h"
#include "mapsector.h"
#include "mockgame.h"
#include "nodedef.h"

MapSector *MockMap::emergeSector(v2s16 p)
{
	MapSector *sector = getSectorNoGenerateNoLock(p);

	if (sector)
		return sector;

	sector = new MapSector(this, p, m_gamedef);
	m_sectors[p] = sector;
	return sector;
}

MapBlock *MockMap::emergeBlock(v3s16 p, bool create_blank)
{
	MapBlock *block = getBlockNoCreateNoEx(p);

	if(block != nullptr)
		return block;

	// Get the MapSector
	MapSector *sector = emergeSector(v2s16(p.X, p.Z));

	// Create the block and fill with air
	block = sector->createBlankBlock(p.Y);
	block->reallocate();
	MapNode *n = block->getData();
        for(u32 i = 0; i < block->nodecount; i++, n++)
		n->setContent(CONTENT_AIR);
	return block;
}

void MockMap::setMockNode(v3s16 p, content_t c)
{
	emergeBlock(getNodeBlockPos(p));
	MapNode n(c);
	setNode(p, n);
}
