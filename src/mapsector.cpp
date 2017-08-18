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

#include "mapsector.h"
#include "exceptions.h"
#include "mapblock.h"
#include "serialization.h"

MapSector::MapSector(Map *parent, v2s16 pos, IGameDef *gamedef):
		m_parent(parent),
		m_pos(pos),
		m_gamedef(gamedef)
{
}

MapSector::~MapSector()
{
	deleteBlocks();
}

void MapSector::deleteBlocks()
{
	// Clear cache
	m_block_cache = nullptr;

	// Delete all
	for (auto &block : m_blocks) {
		delete block.second;
	}

	// Clear container
	m_blocks.clear();
}

MapBlock * MapSector::getBlockBuffered(s16 y)
{
	MapBlock *block;

	if (m_block_cache && y == m_block_cache_y) {
		return m_block_cache;
	}

	// If block doesn't exist, return NULL
	std::unordered_map<s16, MapBlock*>::const_iterator n = m_blocks.find(y);
	block = (n != m_blocks.end() ? n->second : nullptr);

	// Cache the last result
	m_block_cache_y = y;
	m_block_cache = block;

	return block;
}

MapBlock * MapSector::getBlockNoCreateNoEx(s16 y)
{
	return getBlockBuffered(y);
}

MapBlock * MapSector::createBlankBlockNoInsert(s16 y)
{
	assert(getBlockBuffered(y) == NULL);	// Pre-condition

	v3s16 blockpos_map(m_pos.X, y, m_pos.Y);

	MapBlock *block = new MapBlock(m_parent, blockpos_map, m_gamedef);

	return block;
}

MapBlock * MapSector::createBlankBlock(s16 y)
{
	MapBlock *block = createBlankBlockNoInsert(y);

	m_blocks[y] = block;

	return block;
}

void MapSector::insertBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	MapBlock *block2 = getBlockBuffered(block_y);
	if (block2) {
		throw AlreadyExistsException("Block already exists");
	}

	v2s16 p2d(block->getPos().X, block->getPos().Z);
	assert(p2d == m_pos);

	// Insert into container
	m_blocks[block_y] = block;
}

void MapSector::deleteBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	// Clear from cache
	m_block_cache = nullptr;

	// Remove from container
	m_blocks.erase(block_y);

	// Delete
	delete block;
}

void MapSector::getBlocks(MapBlockVect &dest)
{
	for (auto &block : m_blocks) {
		dest.push_back(block.second);
	}
}
