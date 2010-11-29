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
#include "mapblock.h"
#include "heightmap.h"
#include "exceptions.h"

/*
	This is an Y-wise stack of MapBlocks.
*/

#define WATER_LEVEL (-5)

#define SECTOR_OBJECT_TEST 0
#define SECTOR_OBJECT_TREE_1 1
#define SECTOR_OBJECT_BUSH_1 2
#define SECTOR_OBJECT_RAVINE 3

#define MAPSECTOR_FIXEDHEIGHTMAPS_MAXCOUNT 4

#define MAPSECTOR_SERVER 0
#define MAPSECTOR_CLIENT 1

class MapSector: public NodeContainer, public Heightmappish
{
public:
	
	MapSector(NodeContainer *parent, v2s16 pos);
	virtual ~MapSector();

	virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_MAPSECTOR;
	}

	virtual u32 getId() const = 0;

	void deleteBlocks();

	v2s16 getPos()
	{
		return m_pos;
	}

	MapBlock * getBlockNoCreate(s16 y);
	MapBlock * createBlankBlockNoInsert(s16 y);
	MapBlock * createBlankBlock(s16 y);
	//MapBlock * getBlock(s16 y, bool generate=true);

	void insertBlock(MapBlock *block);
	
	// This is used to remove a dummy from the sector while generating it.
	// Block is only removed from internal container, not deleted.
	void removeBlock(MapBlock *block);
	
	/*
		This might not be a thread-safe depending on the day.
		See the implementation.
	*/
	void getBlocks(core::list<MapBlock*> &dest);
	
	/*
		If all nodes in area can be accessed, returns true and
		adds all blocks in area to blocks.

		If all nodes in area cannot be accessed, returns false.

		The implementation of this is quite slow

		if blocks==NULL; it is not accessed at all.
	*/
	bool isValidArea(v3s16 p_min_nodes, v3s16 p_max_nodes,
			core::map<s16, MapBlock*> *blocks)
	{
		core::map<s16, MapBlock*> bs;
		
		v3s16 p_min = getNodeBlockPos(p_min_nodes);
		v3s16 p_max = getNodeBlockPos(p_max_nodes);
		if(p_min.X != 0 || p_min.Z != 0
				|| p_max.X != 0 || p_max.Z != 0)
			return false;
		v3s16 y;
		for(s16 y=p_min.Y; y<=p_max.Y; y++)
		{
			try{
				MapBlock *block = getBlockNoCreate(y);
				if(block->isDummy())
					return false;
				if(blocks!=NULL)
					bs[y] = block;
			}
			catch(InvalidPositionException &e)
			{
				return false;
			}
		}

		if(blocks!=NULL)
		{
			for(core::map<s16, MapBlock*>::Iterator i=bs.getIterator();
					i.atEnd()==false; i++)
			{
				MapBlock *block = i.getNode()->getValue();
				s16 y = i.getNode()->getKey();
				blocks->insert(y, block);
			}
		}
		return true;
	}

	void getBlocksInArea(v3s16 p_min_nodes, v3s16 p_max_nodes,
			core::map<v3s16, MapBlock*> &blocks)
	{
		v3s16 p_min = getNodeBlockPos(p_min_nodes);
		v3s16 p_max = getNodeBlockPos(p_max_nodes);
		v3s16 y;
		for(s16 y=p_min.Y; y<=p_max.Y; y++)
		{
			try{
				MapBlock *block = getBlockNoCreate(y);
				blocks.insert(block->getPos(), block);
			}
			catch(InvalidPositionException &e)
			{
			}
		}
	}
	
	// virtual from NodeContainer
	bool isValidPosition(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);

		if(blockpos.X != 0 || blockpos.Z != 0)
			return false;

		MapBlock *blockref;
		try{
			blockref = getBlockNoCreate(blockpos.Y);
		}
		catch(InvalidPositionException &e)
		{
			return false;
		}

		return true;
	}

	// virtual from NodeContainer
	MapNode getNode(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		if(blockpos.X != 0 || blockpos.Z != 0)
			throw InvalidPositionException
				("MapSector only allows Y");

		MapBlock * blockref = getBlockNoCreate(blockpos.Y);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;

		return blockref->getNode(relpos);
	}
	// virtual from NodeContainer
	void setNode(v3s16 p, MapNode & n)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		if(blockpos.X != 0 || blockpos.Z != 0)
			throw InvalidPositionException
				("MapSector only allows Y");

		MapBlock * blockref = getBlockNoCreate(blockpos.Y);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		blockref->setNode(relpos, n);
	}

	virtual f32 getGroundHeight(v2s16 p, bool generate=false)
	{
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}
	virtual void setGroundHeight(v2s16 p, f32 y, bool generate=false)
	{
	}
	
	// When true, sector metadata is changed from the one on disk
	// (sector metadata = all but blocks)
	// Basically, this should be changed to true in every setter method
	bool differs_from_disk;

	// Counts seconds from last usage.
	// Sector can be deleted from memory after some time of inactivity.
	// NOTE: It has to be made very sure no other thread is accessing
	//       the sector and it doesn't remain in any cache when
	//       deleting it.
	float usage_timer;

protected:
	
	// The pile of MapBlocks
	core::map<s16, MapBlock*> m_blocks;
	//JMutex m_blocks_mutex; // For public access functions

	NodeContainer *m_parent;
	// Position on parent (in MapBlock widths)
	v2s16 m_pos;

	// Be sure to set this to NULL when the cached block is deleted 
	MapBlock *m_block_cache;
	s16 m_block_cache_y;
	
	// This is used for protecting m_blocks
	JMutex m_mutex;
	
	/*
		Private methods
	*/
	MapBlock *getBlockBuffered(s16 y);

};

class ServerMapSector : public MapSector
{
public:
	ServerMapSector(NodeContainer *parent, v2s16 pos, u16 hm_split);
	~ServerMapSector();
	
	u32 getId() const
	{
		return MAPSECTOR_SERVER;
	}

	void setHeightmap(v2s16 hm_p, FixedHeightmap *hm);
	FixedHeightmap * getHeightmap(v2s16 hm_p);
	
	void printHeightmaps()
	{
		for(s16 y=0; y<m_hm_split; y++)
		for(s16 x=0; x<m_hm_split; x++)
		{
			std::cout<<"Sector "
					<<"("<<m_pos.X<<","<<m_pos.Y<<")"
					" heightmap "
					"("<<x<<","<<y<<"):"
					<<std::endl;
			FixedHeightmap *hm = getHeightmap(v2s16(x,y));
			hm->print();
		}
	}
	
	void setObjects(core::map<v3s16, u8> *objects)
	{
		m_objects = objects;
		differs_from_disk = true;
	}

	core::map<v3s16, u8> * getObjects()
	{
		differs_from_disk = true;
		return m_objects;
	}

	f32 getGroundHeight(v2s16 p, bool generate=false);
	void setGroundHeight(v2s16 p, f32 y, bool generate=false);

	/*
		These functions handle metadata.
		They do not handle blocks.
	*/
	void serialize(std::ostream &os, u8 version);
	
	static ServerMapSector* deSerialize(
			std::istream &is,
			NodeContainer *parent,
			v2s16 p2d,
			Heightmap *master_hm,
			core::map<v2s16, MapSector*> & sectors
		);
		
private:
	// Heightmap(s) for the sector
	FixedHeightmap *m_heightmaps[MAPSECTOR_FIXEDHEIGHTMAPS_MAXCOUNT];
	// Sector is split in m_hm_split^2 heightmaps.
	// Value of 0 means there is no heightmap.
	u16 m_hm_split;
	// These are removed when they are drawn to blocks.
	// - Each is drawn when generating blocks; When the last one of
	//   the needed blocks is being generated.
	core::map<v3s16, u8> *m_objects;
};

class ClientMapSector : public MapSector
{
public:
	ClientMapSector(NodeContainer *parent, v2s16 pos);
	~ClientMapSector();
	
	u32 getId() const
	{
		return MAPSECTOR_CLIENT;
	}

	void deSerialize(std::istream &is);

	s16 getCorner(u16 i)
	{
		return m_corners[i];
	}
		
private:
	// The ground height of the corners is stored in here
	s16 m_corners[4];
};
	
#endif

