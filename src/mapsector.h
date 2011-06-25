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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef MAPSECTOR_HEADER
#define MAPSECTOR_HEADER

#include <jmutex.h>
#include "common_irrlicht.h"
#include "exceptions.h"
#include <ostream>

class MapBlock;
class NodeContainer;

/*
	This is an Y-wise stack of MapBlocks.
*/

#define MAPSECTOR_SERVER 0
#define MAPSECTOR_CLIENT 1

class MapSector
{
public:
	
	MapSector(NodeContainer *parent, v2s16 pos);
	virtual ~MapSector();

	virtual u32 getId() const = 0;

	void deleteBlocks();

	v2s16 getPos()
	{
		return m_pos;
	}

	MapBlock * getBlockNoCreateNoEx(s16 y);
	MapBlock * createBlankBlockNoInsert(s16 y);
	MapBlock * createBlankBlock(s16 y);

	void insertBlock(MapBlock *block);
	
	void deleteBlock(MapBlock *block);
	
	void getBlocks(core::list<MapBlock*> &dest);
	
	// Always false at the moment, because sector contains no metadata.
	bool differs_from_disk;

protected:
	
	// The pile of MapBlocks
	core::map<s16, MapBlock*> m_blocks;

	NodeContainer *m_parent;
	// Position on parent (in MapBlock widths)
	v2s16 m_pos;
	
	// Last-used block is cached here for quicker access.
	// Be sure to set this to NULL when the cached block is deleted 
	MapBlock *m_block_cache;
	s16 m_block_cache_y;
	
	/*
		Private methods
	*/
	MapBlock *getBlockBuffered(s16 y);

};

class ServerMapSector : public MapSector
{
public:
	ServerMapSector(NodeContainer *parent, v2s16 pos);
	~ServerMapSector();
	
	u32 getId() const
	{
		return MAPSECTOR_SERVER;
	}

	/*
		These functions handle metadata.
		They do not handle blocks.
	*/

	void serialize(std::ostream &os, u8 version);
	
	static ServerMapSector* deSerialize(
			std::istream &is,
			NodeContainer *parent,
			v2s16 p2d,
			core::map<v2s16, MapSector*> & sectors
		);
		
private:
};

#ifndef SERVER
class ClientMapSector : public MapSector
{
public:
	ClientMapSector(NodeContainer *parent, v2s16 pos);
	~ClientMapSector();
	
	u32 getId() const
	{
		return MAPSECTOR_CLIENT;
	}

private:
};
#endif
	
#endif

