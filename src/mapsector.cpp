/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mapsector.h"
#include "jmutexautolock.h"
#include "client.h"
#include "exceptions.h"

MapSector::MapSector(NodeContainer *parent, v2s16 pos):
		differs_from_disk(true),
		usage_timer(0.0),
		m_parent(parent),
		m_pos(pos),
		m_block_cache(NULL)
{
	m_mutex.Init();
	assert(m_mutex.IsInitialized());
}

MapSector::~MapSector()
{
	deleteBlocks();
}

void MapSector::deleteBlocks()
{
	JMutexAutoLock lock(m_mutex);

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
	JMutexAutoLock lock(m_mutex);
	
	return getBlockBuffered(y);
}

MapBlock * MapSector::getBlockNoCreate(s16 y)
{
	MapBlock *block = getBlockNoCreateNoEx(y);

	if(block == NULL)
		throw InvalidPositionException();
	
	return block;
}

MapBlock * MapSector::createBlankBlockNoInsert(s16 y)
{
	// There should not be a block at this position
	if(getBlockBuffered(y) != NULL)
		throw AlreadyExistsException("Block already exists");

	v3s16 blockpos_map(m_pos.X, y, m_pos.Y);
	
	MapBlock *block = new MapBlock(m_parent, blockpos_map);
	
	return block;
}

MapBlock * MapSector::createBlankBlock(s16 y)
{
	JMutexAutoLock lock(m_mutex);
	
	MapBlock *block = createBlankBlockNoInsert(y);
	
	m_blocks.insert(y, block);

	return block;
}

void MapSector::insertBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	{
		JMutexAutoLock lock(m_mutex);

		MapBlock *block2 = getBlockBuffered(block_y);
		if(block2 != NULL){
			throw AlreadyExistsException("Block already exists");
		}

		v2s16 p2d(block->getPos().X, block->getPos().Z);
		assert(p2d == m_pos);
		
		// Insert into container
		m_blocks.insert(block_y, block);
	}
}

void MapSector::removeBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	JMutexAutoLock lock(m_mutex);
	
	// Clear from cache
	m_block_cache = NULL;
	
	// Remove from container
	m_blocks.remove(block_y);
}

void MapSector::getBlocks(core::list<MapBlock*> &dest)
{
	JMutexAutoLock lock(m_mutex);

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

ServerMapSector::ServerMapSector(NodeContainer *parent, v2s16 pos):
		MapSector(parent, pos)
{
}

ServerMapSector::~ServerMapSector()
{
}

f32 ServerMapSector::getGroundHeight(v2s16 p, bool generate)
{
	return GROUNDHEIGHT_NOTFOUND_SETVALUE;
}

void ServerMapSector::setGroundHeight(v2s16 p, f32 y, bool generate)
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
		NodeContainer *parent,
		v2s16 p2d,
		core::map<v2s16, MapSector*> & sectors
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
		sector = new ServerMapSector(parent, p2d);
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

ClientMapSector::ClientMapSector(NodeContainer *parent, v2s16 pos):
		MapSector(parent, pos)
{
}

ClientMapSector::~ClientMapSector()
{
}

void ClientMapSector::deSerialize(std::istream &is)
{
	/*
		[0] u8 serialization version
		[1] s16 corners[0]
		[3] s16 corners[1]
		[5] s16 corners[2]
		[7] s16 corners[3]
		size = 9
		
		In which corners are in these positions
		v2s16(0,0),
		v2s16(1,0),
		v2s16(1,1),
		v2s16(0,1),
	*/
	
	// Read version
	u8 version = SER_FMT_VER_INVALID;
	is.read((char*)&version, 1);
	
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapSector format not supported");
	
	u8 buf[2];
	
	// Dummy read corners
	is.read((char*)buf, 2);
	is.read((char*)buf, 2);
	is.read((char*)buf, 2);
	is.read((char*)buf, 2);
	
	/*
		Set stuff in sector
	*/
	
	// Nothing here

}
#endif // !SERVER

//END
