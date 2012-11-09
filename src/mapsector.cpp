/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "jmutexautolock.h"
#ifndef SERVER
#include "client.h"
#endif
#include "exceptions.h"
#include "mapblock.h"

MapSector::MapSector(Map *parent, v2s16 pos, IGameDef *gamedef):
		differs_from_disk(false),
		m_parent(parent),
		m_pos(pos),
		m_gamedef(gamedef),
		m_block_cache(NULL)
{
}

MapSector::~MapSector()
{
	deleteBlocks();
}

void MapSector::deleteBlocks()
{
	// Clear cache
	m_block_cache = NULL;

	// Delete all
	core::map<s16, MapBlock*>::Iterator i = m_blocks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		delete i.getNode()->getValue();
	}

	// Clear container
	m_blocks.clear();
}

MapBlock * MapSector::getBlockBuffered(s16 y)
{
	MapBlock *block;

	if(m_block_cache != NULL && y == m_block_cache_y){
		return m_block_cache;
	}
	
	// If block doesn't exist, return NULL
	core::map<s16, MapBlock*>::Node *n = m_blocks.find(y);
	if(n == NULL)
	{
		block = NULL;
	}
	// If block exists, return it
	else{
		block = n->getValue();
	}
	
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
	assert(getBlockBuffered(y) == NULL);

	v3s16 blockpos_map(m_pos.X, y, m_pos.Y);
	
	MapBlock *block = new MapBlock(m_parent, blockpos_map, m_gamedef);
	
	return block;
}

MapBlock * MapSector::createBlankBlock(s16 y)
{
	MapBlock *block = createBlankBlockNoInsert(y);
	
	m_blocks.insert(y, block);

	return block;
}

void MapSector::insertBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	MapBlock *block2 = getBlockBuffered(block_y);
	if(block2 != NULL){
		throw AlreadyExistsException("Block already exists");
	}

	v2s16 p2d(block->getPos().X, block->getPos().Z);
	assert(p2d == m_pos);
	
	// Insert into container
	m_blocks.insert(block_y, block);
}

void MapSector::deleteBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	// Clear from cache
	m_block_cache = NULL;
	
	// Remove from container
	m_blocks.remove(block_y);

	// Delete
	delete block;
}

void MapSector::getBlocks(core::list<MapBlock*> &dest)
{
	core::list<MapBlock*> ref_list;

	core::map<s16, MapBlock*>::Iterator bi;

	bi = m_blocks.getIterator();
	for(; bi.atEnd() == false; bi++)
	{
		MapBlock *b = bi.getNode()->getValue();
		dest.push_back(b);
	}
}

/*
	ServerMapSector
*/

ServerMapSector::ServerMapSector(Map *parent, v2s16 pos, IGameDef *gamedef):
		MapSector(parent, pos, gamedef)
{
}

ServerMapSector::~ServerMapSector()
{
}

void ServerMapSector::serialize(std::ostream &os, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapSector format not supported");
	
	/*
		[0] u8 serialization version
		+ heightmap data
	*/
	
	// Server has both of these, no need to support not having them.
	//assert(m_objects != NULL);

	// Write version
	os.write((char*)&version, 1);
	
	/*
		Add stuff here, if needed
	*/

}

ServerMapSector* ServerMapSector::deSerialize(
		std::istream &is,
		Map *parent,
		v2s16 p2d,
		core::map<v2s16, MapSector*> & sectors,
		IGameDef *gamedef
	)
{
	/*
		[0] u8 serialization version
		+ heightmap data
	*/

	/*
		Read stuff
	*/
	
	// Read version
	u8 version = SER_FMT_VER_INVALID;
	is.read((char*)&version, 1);
	
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapSector format not supported");
	
	/*
		Add necessary reading stuff here
	*/
	
	/*
		Get or create sector
	*/

	ServerMapSector *sector = NULL;

	core::map<v2s16, MapSector*>::Node *n = sectors.find(p2d);

	if(n != NULL)
	{
		dstream<<"WARNING: deSerializing existent sectors not supported "
				"at the moment, because code hasn't been tested."
				<<std::endl;

		MapSector *sector = n->getValue();
		assert(sector->getId() == MAPSECTOR_SERVER);
		return (ServerMapSector*)sector;
	}
	else
	{
		sector = new ServerMapSector(parent, p2d, gamedef);
		sectors.insert(p2d, sector);
	}

	/*
		Set stuff in sector
	*/

	// Nothing here

	return sector;
}

#ifndef SERVER
/*
	ClientMapSector
*/

ClientMapSector::ClientMapSector(Map *parent, v2s16 pos, IGameDef *gamedef):
		MapSector(parent, pos, gamedef)
{
}

ClientMapSector::~ClientMapSector()
{
}

#endif // !SERVER

//END
