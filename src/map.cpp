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

#include "map.h"
//#include "player.h"
#include "main.h"
#include "jmutexautolock.h"
#include "client.h"
#include "filesys.h"
#include "utility.h"
#include "voxel.h"

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#define sleep_ms(x) usleep(x*1000)
#endif

MapBlockPointerCache::MapBlockPointerCache(Map *map)
{
	m_map = map;
	m_map->m_blockcachelock.cacheCreated();

	m_from_cache_count = 0;
	m_from_map_count = 0;
}

MapBlockPointerCache::~MapBlockPointerCache()
{
	m_map->m_blockcachelock.cacheRemoved();

	/*dstream<<"MapBlockPointerCache:"
			<<" from_cache_count="<<m_from_cache_count
			<<" from_map_count="<<m_from_map_count
			<<std::endl;*/
}

MapBlock * MapBlockPointerCache::getBlockNoCreate(v3s16 p)
{
	core::map<v3s16, MapBlock*>::Node *n = NULL;
	n = m_blocks.find(p);
	if(n != NULL)
	{
		m_from_cache_count++;
		return n->getValue();
	}
	
	m_from_map_count++;
	
	// Throws InvalidPositionException if not found
	MapBlock *b = m_map->getBlockNoCreate(p);
	m_blocks[p] = b;
	return b;
}

/*
	Map
*/

Map::Map(std::ostream &dout):
	m_dout(dout),
	m_camera_position(0,0,0),
	m_camera_direction(0,0,1),
	m_sector_cache(NULL),
	m_hwrapper(this),
	drawoffset(0,0,0)
{
	m_sector_mutex.Init();
	m_camera_mutex.Init();
	assert(m_sector_mutex.IsInitialized());
	assert(m_camera_mutex.IsInitialized());
	
	// Get this so that the player can stay on it at first
	//getSector(v2s16(0,0));
}

Map::~Map()
{
	/*
		Stop updater thread
	*/
	/*updater.setRun(false);
	while(updater.IsRunning())
		sleep_s(1);*/

	/*
		Free all MapSectors.
	*/
	core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapSector *sector = i.getNode()->getValue();
		delete sector;
	}
}

/*bool Map::sectorExists(v2s16 p)
{
	JMutexAutoLock lock(m_sector_mutex);
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
	return (n != NULL);
}*/

MapSector * Map::getSectorNoGenerate(v2s16 p)
{
	JMutexAutoLock lock(m_sector_mutex);

	if(m_sector_cache != NULL && p == m_sector_cache_p){
		MapSector * sector = m_sector_cache;
		// Reset inactivity timer
		sector->usage_timer = 0.0;
		return sector;
	}
	
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
	// If sector doesn't exist, throw an exception
	if(n == NULL)
	{
		throw InvalidPositionException();
	}
	
	MapSector *sector = n->getValue();
	
	// Cache the last result
	m_sector_cache_p = p;
	m_sector_cache = sector;

	//MapSector * ref(sector);
	
	// Reset inactivity timer
	sector->usage_timer = 0.0;
	return sector;
}

MapBlock * Map::getBlockNoCreate(v3s16 p3d)
{	
	v2s16 p2d(p3d.X, p3d.Z);
	MapSector * sector = getSectorNoGenerate(p2d);

	MapBlock *block = sector->getBlockNoCreate(p3d.Y);

	return block;
}

/*MapBlock * Map::getBlock(v3s16 p3d, bool generate)
{
	dstream<<"Map::getBlock() with generate=true called"
			<<std::endl;
	v2s16 p2d(p3d.X, p3d.Z);
	//MapSector * sector = getSector(p2d, generate);
	MapSector * sector = getSectorNoGenerate(p2d);

	if(sector == NULL)
		throw InvalidPositionException();

	return sector->getBlockNoCreate(p3d.Y);
}*/

f32 Map::getGroundHeight(v2s16 p, bool generate)
{
	try{
		v2s16 sectorpos = getNodeSectorPos(p);
		MapSector * sref = getSectorNoGenerate(sectorpos);
		v2s16 relpos = p - sectorpos * MAP_BLOCKSIZE;
		f32 y = sref->getGroundHeight(relpos);
		return y;
	}
	catch(InvalidPositionException &e)
	{
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}
}

void Map::setGroundHeight(v2s16 p, f32 y, bool generate)
{
	/*m_dout<<DTIME<<"Map::setGroundHeight(("
			<<p.X<<","<<p.Y
			<<"), "<<y<<")"<<std::endl;*/
	v2s16 sectorpos = getNodeSectorPos(p);
	MapSector * sref = getSectorNoGenerate(sectorpos);
	v2s16 relpos = p - sectorpos * MAP_BLOCKSIZE;
	//sref->mutex.Lock();
	sref->setGroundHeight(relpos, y);
	//sref->mutex.Unlock();
}

bool Map::isNodeUnderground(v3s16 p)
{
	v3s16 blockpos = getNodeBlockPos(p);
	try{
		MapBlock * block = getBlockNoCreate(blockpos);
		return block->getIsUnderground();
	}
	catch(InvalidPositionException &e)
	{
		return false;
	}
}

#if 0
void Map::interpolate(v3s16 block,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	const v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};

	if(from_nodes.size() == 0)
		return;
	
	u32 blockchangecount = 0;

	core::map<v3s16, bool> lighted_nodes;
	core::map<v3s16, bool>::Iterator j;
	j = from_nodes.getIterator();

	/*
		Initialize block cache
	*/
	v3s16 blockpos_last;
	MapBlock *block = NULL;
	// Cache this a bit, too
	bool block_checked_in_modified = false;
	
	for(; j.atEnd() == false; j++)
	//for(; j != from_nodes.end(); j++)
	{
		v3s16 pos = j.getNode()->getKey();
		//v3s16 pos = *j;
		//dstream<<"pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"<<std::endl;
		v3s16 blockpos = getNodeBlockPos(pos);
		
		// Only fetch a new block if the block position has changed
		try{
			if(block == NULL || blockpos != blockpos_last){
				block = getBlockNoCreate(blockpos);
				blockpos_last = blockpos;

				block_checked_in_modified = false;
				blockchangecount++;
			}
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		if(block->isDummy())
			continue;

		// Calculate relative position in block
		v3s16 relpos = pos - blockpos_last * MAP_BLOCKSIZE;

		// Get node straight from the block
		MapNode n = block->getNode(relpos);

		u8 oldlight = n.getLight();
		u8 newlight = diminish_light(oldlight);

		// Loop through 6 neighbors
		for(u16 i=0; i<6; i++){
			// Get the position of the neighbor node
			v3s16 n2pos = pos + dirs[i];
			
			// Get the block where the node is located
			v3s16 blockpos = getNodeBlockPos(n2pos);

			try
			{
				// Only fetch a new block if the block position has changed
				try{
					if(block == NULL || blockpos != blockpos_last){
						block = getBlockNoCreate(blockpos);
						blockpos_last = blockpos;

						block_checked_in_modified = false;
						blockchangecount++;
					}
				}
				catch(InvalidPositionException &e)
				{
					continue;
				}
				
				// Calculate relative position in block
				v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
				// Get node straight from the block
				MapNode n2 = block->getNode(relpos);
				
				bool changed = false;
				/*
					If the neighbor is brighter than the current node,
					add to list (it will light up this node on its turn)
				*/
				if(n2.getLight() > undiminish_light(oldlight))
				{
					lighted_nodes.insert(n2pos, true);
					//lighted_nodes.push_back(n2pos);
					changed = true;
				}
				/*
					If the neighbor is dimmer than how much light this node
					would spread on it, add to list
				*/
				if(n2.getLight() < newlight)
				{
					if(n2.light_propagates())
					{
						n2.setLight(newlight);
						block->setNode(relpos, n2);
						lighted_nodes.insert(n2pos, true);
						//lighted_nodes.push_back(n2pos);
						changed = true;
					}
				}

				// Add to modified_blocks
				if(changed == true && block_checked_in_modified == false)
				{
					// If the block is not found in modified_blocks, add.
					if(modified_blocks.find(blockpos) == NULL)
					{
						modified_blocks.insert(blockpos, block);
					}
					block_checked_in_modified = true;
				}
			}
			catch(InvalidPositionException &e)
			{
				continue;
			}
		}
	}

	/*dstream<<"spreadLight(): Changed block "
			<<blockchangecount<<" times"
			<<" for "<<from_nodes.size()<<" nodes"
			<<std::endl;*/
	
	if(lighted_nodes.size() > 0)
		spreadLight(lighted_nodes, modified_blocks);
}
#endif

/*
	Goes recursively through the neighbours of the node.

	Alters only transparent nodes.

	If the lighting of the neighbour is lower than the lighting of
	the node was (before changing it to 0 at the step before), the
	lighting of the neighbour is set to 0 and then the same stuff
	repeats for the neighbour.

	The ending nodes of the routine are stored in light_sources.
	This is useful when a light is removed. In such case, this
	routine can be called for the light node and then again for
	light_sources to re-light the area without the removed light.

	values of from_nodes are lighting values.
*/
void Map::unspreadLight(core::map<v3s16, u8> & from_nodes,
		core::map<v3s16, bool> & light_sources,
		core::map<v3s16, MapBlock*>  & modified_blocks)
{
	v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	
	if(from_nodes.size() == 0)
		return;
	
	u32 blockchangecount = 0;

	core::map<v3s16, u8> unlighted_nodes;
	core::map<v3s16, u8>::Iterator j;
	j = from_nodes.getIterator();

	/*
		Initialize block cache
	*/
	v3s16 blockpos_last;
	MapBlock *block = NULL;
	// Cache this a bit, too
	bool block_checked_in_modified = false;
	
	for(; j.atEnd() == false; j++)
	{
		v3s16 pos = j.getNode()->getKey();
		v3s16 blockpos = getNodeBlockPos(pos);
		
		// Only fetch a new block if the block position has changed
		try{
			if(block == NULL || blockpos != blockpos_last){
				block = getBlockNoCreate(blockpos);
				blockpos_last = blockpos;

				block_checked_in_modified = false;
				blockchangecount++;
			}
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		if(block->isDummy())
			continue;

		// Calculate relative position in block
		v3s16 relpos = pos - blockpos_last * MAP_BLOCKSIZE;

		// Get node straight from the block
		MapNode n = block->getNode(relpos);
		
		u8 oldlight = j.getNode()->getValue();
		
		// Loop through 6 neighbors
		for(u16 i=0; i<6; i++)
		{
			// Get the position of the neighbor node
			v3s16 n2pos = pos + dirs[i];
			
			// Get the block where the node is located
			v3s16 blockpos = getNodeBlockPos(n2pos);

			try
			{
				// Only fetch a new block if the block position has changed
				try{
					if(block == NULL || blockpos != blockpos_last){
						block = getBlockNoCreate(blockpos);
						blockpos_last = blockpos;

						block_checked_in_modified = false;
						blockchangecount++;
					}
				}
				catch(InvalidPositionException &e)
				{
					continue;
				}
				
				// Calculate relative position in block
				v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
				// Get node straight from the block
				MapNode n2 = block->getNode(relpos);
				
				bool changed = false;

				//TODO: Optimize output by optimizing light_sources?

				/*
					If the neighbor is dimmer than what was specified
					as oldlight (the light of the previous node)
				*/
				if(n2.getLight() < oldlight)
				{
					/*
						And the neighbor is transparent and it has some light
					*/
					if(n2.light_propagates() && n2.getLight() != 0)
					{
						/*
							Set light to 0 and add to queue
						*/

						u8 current_light = n2.getLight();
						n2.setLight(0);
						block->setNode(relpos, n2);

						unlighted_nodes.insert(n2pos, current_light);
						changed = true;

						/*
							Remove from light_sources if it is there
							NOTE: This doesn't happen nearly at all
						*/
						/*if(light_sources.find(n2pos))
						{
							std::cout<<"Removed from light_sources"<<std::endl;
							light_sources.remove(n2pos);
						}*/
					}
				}
				else{
					light_sources.insert(n2pos, true);
				}

				// Add to modified_blocks
				if(changed == true && block_checked_in_modified == false)
				{
					// If the block is not found in modified_blocks, add.
					if(modified_blocks.find(blockpos) == NULL)
					{
						modified_blocks.insert(blockpos, block);
					}
					block_checked_in_modified = true;
				}
			}
			catch(InvalidPositionException &e)
			{
				continue;
			}
		}
	}

	/*dstream<<"unspreadLight(): Changed block "
			<<blockchangecount<<" times"
			<<" for "<<from_nodes.size()<<" nodes"
			<<std::endl;*/
	
	if(unlighted_nodes.size() > 0)
		unspreadLight(unlighted_nodes, light_sources, modified_blocks);
}

/*
	A single-node wrapper of the above
*/
void Map::unLightNeighbors(v3s16 pos, u8 lightwas,
		core::map<v3s16, bool> & light_sources,
		core::map<v3s16, MapBlock*>  & modified_blocks)
{
	core::map<v3s16, u8> from_nodes;
	from_nodes.insert(pos, lightwas);

	unspreadLight(from_nodes, light_sources, modified_blocks);
}

/*
	Lights neighbors of from_nodes, collects all them and then
	goes on recursively.
*/
void Map::spreadLight(core::map<v3s16, bool> & from_nodes,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	const v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};

	if(from_nodes.size() == 0)
		return;
	
	u32 blockchangecount = 0;

	core::map<v3s16, bool> lighted_nodes;
	core::map<v3s16, bool>::Iterator j;
	j = from_nodes.getIterator();

	/*
		Initialize block cache
	*/
	v3s16 blockpos_last;
	MapBlock *block = NULL;
	// Cache this a bit, too
	bool block_checked_in_modified = false;
	
	for(; j.atEnd() == false; j++)
	//for(; j != from_nodes.end(); j++)
	{
		v3s16 pos = j.getNode()->getKey();
		//v3s16 pos = *j;
		//dstream<<"pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"<<std::endl;
		v3s16 blockpos = getNodeBlockPos(pos);
		
		// Only fetch a new block if the block position has changed
		try{
			if(block == NULL || blockpos != blockpos_last){
				block = getBlockNoCreate(blockpos);
				blockpos_last = blockpos;

				block_checked_in_modified = false;
				blockchangecount++;
			}
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		if(block->isDummy())
			continue;

		// Calculate relative position in block
		v3s16 relpos = pos - blockpos_last * MAP_BLOCKSIZE;

		// Get node straight from the block
		MapNode n = block->getNode(relpos);

		u8 oldlight = n.getLight();
		u8 newlight = diminish_light(oldlight);

		// Loop through 6 neighbors
		for(u16 i=0; i<6; i++){
			// Get the position of the neighbor node
			v3s16 n2pos = pos + dirs[i];
			
			// Get the block where the node is located
			v3s16 blockpos = getNodeBlockPos(n2pos);

			try
			{
				// Only fetch a new block if the block position has changed
				try{
					if(block == NULL || blockpos != blockpos_last){
						block = getBlockNoCreate(blockpos);
						blockpos_last = blockpos;

						block_checked_in_modified = false;
						blockchangecount++;
					}
				}
				catch(InvalidPositionException &e)
				{
					continue;
				}
				
				// Calculate relative position in block
				v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
				// Get node straight from the block
				MapNode n2 = block->getNode(relpos);
				
				bool changed = false;
				/*
					If the neighbor is brighter than the current node,
					add to list (it will light up this node on its turn)
				*/
				if(n2.getLight() > undiminish_light(oldlight))
				{
					lighted_nodes.insert(n2pos, true);
					//lighted_nodes.push_back(n2pos);
					changed = true;
				}
				/*
					If the neighbor is dimmer than how much light this node
					would spread on it, add to list
				*/
				if(n2.getLight() < newlight)
				{
					if(n2.light_propagates())
					{
						n2.setLight(newlight);
						block->setNode(relpos, n2);
						lighted_nodes.insert(n2pos, true);
						//lighted_nodes.push_back(n2pos);
						changed = true;
					}
				}

				// Add to modified_blocks
				if(changed == true && block_checked_in_modified == false)
				{
					// If the block is not found in modified_blocks, add.
					if(modified_blocks.find(blockpos) == NULL)
					{
						modified_blocks.insert(blockpos, block);
					}
					block_checked_in_modified = true;
				}
			}
			catch(InvalidPositionException &e)
			{
				continue;
			}
		}
	}

	/*dstream<<"spreadLight(): Changed block "
			<<blockchangecount<<" times"
			<<" for "<<from_nodes.size()<<" nodes"
			<<std::endl;*/
	
	if(lighted_nodes.size() > 0)
		spreadLight(lighted_nodes, modified_blocks);
}

/*
	A single-node source variation of the above.
*/
void Map::lightNeighbors(v3s16 pos,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	core::map<v3s16, bool> from_nodes;
	from_nodes.insert(pos, true);
	spreadLight(from_nodes, modified_blocks);
}

v3s16 Map::getBrightestNeighbour(v3s16 p)
{
	v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	
	u8 brightest_light = 0;
	v3s16 brightest_pos(0,0,0);
	bool found_something = false;

	// Loop through 6 neighbors
	for(u16 i=0; i<6; i++){
		// Get the position of the neighbor node
		v3s16 n2pos = p + dirs[i];
		MapNode n2;
		try{
			n2 = getNode(n2pos);
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}
		if(n2.getLight() > brightest_light || found_something == false){
			brightest_light = n2.getLight();
			brightest_pos = n2pos;
			found_something = true;
		}
	}

	if(found_something == false)
		throw InvalidPositionException();
		
	return brightest_pos;
}

/*
	Propagates sunlight down from a node.
	Starting point gets sunlight.

	Returns the lowest y value of where the sunlight went.
*/
s16 Map::propagateSunlight(v3s16 start,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	s16 y = start.Y;
	for(; ; y--)
	{
		v3s16 pos(start.X, y, start.Z);
		
		v3s16 blockpos = getNodeBlockPos(pos);
		MapBlock *block;
		try{
			block = getBlockNoCreate(blockpos);
		}
		catch(InvalidPositionException &e)
		{
			break;
		}

		v3s16 relpos = pos - blockpos*MAP_BLOCKSIZE;
		MapNode n = block->getNode(relpos);

		if(n.sunlight_propagates())
		{
			n.setLight(LIGHT_SUN);
			block->setNode(relpos, n);

			modified_blocks.insert(blockpos, block);
		}
		else{
			break;
		}
	}
	return y + 1;
}

void Map::updateLighting(core::map<v3s16, MapBlock*> & a_blocks,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	/*m_dout<<DTIME<<"Map::updateLighting(): "
			<<a_blocks.getSize()<<" blocks... ";*/
	
	// For debugging
	bool debug=false;
	u32 count_was = modified_blocks.size();

	/*core::list<MapBlock *>::Iterator i = a_blocks.begin();
	for(; i != a_blocks.end(); i++)
	{
		MapBlock *block = *i;*/

	core::map<v3s16, bool> light_sources;
	
	core::map<v3s16, u8> unlight_from;
		
	core::map<v3s16, MapBlock*>::Iterator i;
	i = a_blocks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();
		
		for(;;)
		{
			// Don't bother with dummy blocks.
			if(block->isDummy())
				break;
		
			v3s16 pos = block->getPos();
			modified_blocks.insert(pos, block);

			/*
				Clear all light from block
			*/
			for(s16 z=0; z<MAP_BLOCKSIZE; z++)
			for(s16 x=0; x<MAP_BLOCKSIZE; x++)
			for(s16 y=0; y<MAP_BLOCKSIZE; y++)
			{
				
				try{
					v3s16 p(x,y,z);
					MapNode n = block->getNode(v3s16(x,y,z));
					u8 oldlight = n.getLight();
					n.setLight(0);
					block->setNode(v3s16(x,y,z), n);
					
					// Collect borders for unlighting
					if(x==0 || x == MAP_BLOCKSIZE-1
							|| y==0 || y == MAP_BLOCKSIZE-1
							|| z==0 || z == MAP_BLOCKSIZE-1)
					{
						v3s16 p_map = p + v3s16(
								MAP_BLOCKSIZE*pos.X,
								MAP_BLOCKSIZE*pos.Y,
								MAP_BLOCKSIZE*pos.Z);
						unlight_from.insert(p_map, oldlight);
					}
				}
				catch(InvalidPositionException &e)
				{
					/*
						This would happen when dealing with a
						dummy block.
					*/
					//assert(0);
					dstream<<"updateLighting(): InvalidPositionException"
							<<std::endl;
				}
			}
			
			bool bottom_valid = block->propagateSunlight(light_sources);

			// If bottom is valid, we're done.
			if(bottom_valid)
				break;
				
			/*dstream<<"Bottom for sunlight-propagated block ("
					<<pos.X<<","<<pos.Y<<","<<pos.Z<<") not valid"
					<<std::endl;*/

			// Else get the block below and loop to it

			pos.Y--;
			try{
				block = getBlockNoCreate(pos);
			}
			catch(InvalidPositionException &e)
			{
				assert(0);
			}
			
		}
	}
	
	{
		//TimeTaker timer("unspreadLight", g_device);
		unspreadLight(unlight_from, light_sources, modified_blocks);
	}
	
	if(debug)
	{
		u32 diff = modified_blocks.size() - count_was;
		count_was = modified_blocks.size();
		dstream<<"unspreadLight modified "<<diff<<std::endl;
	}

	// TODO: Spread light from propagated sunlight?
	// Yes, add it to light_sources... somehow.
	// It has to be added at somewhere above, in the loop.
	// TODO

	{
		//TimeTaker timer("spreadLight", g_device);
		spreadLight(light_sources, modified_blocks);
	}
	
	if(debug)
	{
		u32 diff = modified_blocks.size() - count_was;
		count_was = modified_blocks.size();
		dstream<<"spreadLight modified "<<diff<<std::endl;
	}

	//m_dout<<"Done ("<<getTimestamp()<<")"<<std::endl;
}

/*
	This is called after changing a node from transparent to opaque.
	The lighting value of the node should be left as-is after changing
	other values. This sets the lighting value to 0.
*/
/*void Map::nodeAddedUpdate(v3s16 p, u8 lightwas,
		core::map<v3s16, MapBlock*> &modified_blocks)*/
void Map::addNodeAndUpdate(v3s16 p, MapNode n,
		core::map<v3s16, MapBlock*> &modified_blocks)
{
	/*PrintInfo(m_dout);
	m_dout<<DTIME<<"Map::nodeAddedUpdate(): p=("
			<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/

	u8 lightwas = getNode(p).getLight();

	//core::list<v3s16> light_sources;
	core::map<v3s16, bool> light_sources;
	//MapNode n = getNode(p);

	/*
		From this node to nodes underneath:
		If lighting is sunlight (1.0), unlight neighbours and
		set lighting to 0.
		Else discontinue.
	*/

	bool node_under_sunlight = true;
	
	v3s16 toppos = p + v3s16(0,1,0);

	/*
		If there is a node at top and it doesn't have sunlight,
		there has not been any sunlight going down.

		Otherwise there probably is.
	*/
	try{
		MapNode topnode = getNode(toppos);

		if(topnode.getLight() != LIGHT_SUN)
			node_under_sunlight = false;
	}
	catch(InvalidPositionException &e)
	{
	}

	// Add the block of the added node to modified_blocks
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock * block = getBlockNoCreate(blockpos);
	assert(block != NULL);
	modified_blocks.insert(blockpos, block);
	
	if(isValidPosition(p) == false)
		throw;
		
	// Unlight neighbours of node.
	// This means setting light of all consequent dimmer nodes
	// to 0.
	// This also collects the nodes at the border which will spread
	// light again into this.
	unLightNeighbors(p, lightwas, light_sources, modified_blocks);

	n.setLight(0);
	setNode(p, n);
	
	/*
		If node is under sunlight, take all sunlighted nodes under
		it and clear light from them and from where the light has
		been spread.
	*/
	if(node_under_sunlight)
	{
		s16 y = p.Y - 1;
		for(;; y--){
			//m_dout<<DTIME<<"y="<<y<<std::endl;
			v3s16 n2pos(p.X, y, p.Z);
			
			MapNode n2;
			try{
				n2 = getNode(n2pos);
			}
			catch(InvalidPositionException &e)
			{
				break;
			}

			if(n2.getLight() == LIGHT_SUN)
			{
				//m_dout<<DTIME<<"doing"<<std::endl;
				unLightNeighbors(n2pos, n2.getLight(), light_sources, modified_blocks);
				n2.setLight(0);
				setNode(n2pos, n2);
			}
			else
				break;
		}
	}
	
	/*
		Spread light from all nodes that might be capable of doing so
		TODO: Convert to spreadLight
	*/
	spreadLight(light_sources, modified_blocks);
}

/*
*/
void Map::removeNodeAndUpdate(v3s16 p,
		core::map<v3s16, MapBlock*> &modified_blocks)
{
	/*PrintInfo(m_dout);
	m_dout<<DTIME<<"Map::removeNodeAndUpdate(): p=("
			<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
	
	bool node_under_sunlight = true;
	
	v3s16 toppos = p + v3s16(0,1,0);

	// Node will be replaced with this
	u8 replace_material = MATERIAL_AIR;
	
	// NOTE: Water is now managed elsewhere
#if 0
	{
		/*
			Find out with what material the node will be replaced.
			It will be replaced with the mostly seen buildable_to.
		*/

		v3s16 dirs[6] = {
			v3s16(0,0,1), // back
			v3s16(0,1,0), // top
			v3s16(1,0,0), // right
			v3s16(0,0,-1), // front
			v3s16(0,-1,0), // bottom
			v3s16(-1,0,0), // left
		};

		core::map<u8, u16> neighbor_rankings;
		
		for(u32 i=0; i<sizeof(dirs)/sizeof(dirs[0]); i++)
		{
			try{
				MapNode n2 = getNode(p + dirs[i]);

				if(material_buildable_to(n2.d))
				{
					if(neighbor_rankings.find(n2.d) == NULL)
						neighbor_rankings[n2.d] = 1;
					else
						neighbor_rankings[n2.d]
							= neighbor_rankings[n2.d] + 1;
				}
			}
			catch(InvalidPositionException &e)
			{
			}
		}

		u16 highest_ranking = 0;
		
		for(core::map<u8, u16>::Iterator
				i = neighbor_rankings.getIterator();
				i.atEnd() == false; i++)
		{
			u8 m = i.getNode()->getKey();
			u8 c = i.getNode()->getValue();
			if(
					c > highest_ranking ||
					// Prefer something else than air
					(c >= highest_ranking && m != MATERIAL_AIR)

			)
			{
				replace_material = m;
				highest_ranking = c;
			}
		}
	}

#endif

	/*
		If there is a node at top and it doesn't have sunlight,
		there will be no sunlight going down.
	*/
	try{
		MapNode topnode = getNode(toppos);

		if(topnode.getLight() != LIGHT_SUN)
			node_under_sunlight = false;
	}
	catch(InvalidPositionException &e)
	{
	}

	/*
		Unlight neighbors (in case the node is a light source)
	*/
	//core::list<v3s16> light_sources;
	core::map<v3s16, bool> light_sources;
	unLightNeighbors(p, getNode(p).getLight(),
			light_sources, modified_blocks);

	/*
		Remove the node
	*/
	MapNode n;
	n.d = replace_material;
	n.setLight(0);
	setNode(p, n);
	
	/*
		Recalculate lighting
	*/
	spreadLight(light_sources, modified_blocks);

	// Add the block of the removed node to modified_blocks
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock * block = getBlockNoCreate(blockpos);
	assert(block != NULL);
	modified_blocks.insert(blockpos, block);

	/*
		If the removed node was under sunlight, propagate the
		sunlight down from it and then light all neighbors
		of the propagated blocks.
	*/
	if(node_under_sunlight)
	{
		s16 ybottom = propagateSunlight(p, modified_blocks);
		/*m_dout<<DTIME<<"Node was under sunlight. "
				"Propagating sunlight";
		m_dout<<DTIME<<" -> ybottom="<<ybottom<<std::endl;*/
		s16 y = p.Y;
		for(; y >= ybottom; y--)
		{
			v3s16 p2(p.X, y, p.Z);
			/*m_dout<<DTIME<<"lighting neighbors of node ("
					<<p2.X<<","<<p2.Y<<","<<p2.Z<<")"
					<<std::endl;*/
			lightNeighbors(p2, modified_blocks);
		}
	}
	else
	{
		// Set the lighting of this node to 0
		try{
			MapNode n = getNode(p);
			n.setLight(0);
			setNode(p, n);
		}
		catch(InvalidPositionException &e)
		{
			throw;
		}
	}

	// Get the brightest neighbour node and propagate light from it
	v3s16 n2p = getBrightestNeighbour(p);
	try{
		MapNode n2 = getNode(n2p);
		lightNeighbors(n2p, modified_blocks);
	}
	catch(InvalidPositionException &e)
	{
	}
}

void Map::updateMeshes(v3s16 blockpos)
{
	assert(mapType() == MAPTYPE_CLIENT);

	try{
		v3s16 p = blockpos + v3s16(0,0,0);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh();
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(-1,0,0);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh();
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,-1,0);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh();
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,0,-1);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh();
	}
	catch(InvalidPositionException &e){}
}

/*
	Updates usage timers
*/
void Map::timerUpdate(float dtime)
{
	JMutexAutoLock lock(m_sector_mutex);

	core::map<v2s16, MapSector*>::Iterator si;

	si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();
		sector->usage_timer += dtime;
	}
}

void Map::deleteSectors(core::list<v2s16> &list, bool only_blocks)
{
	/*
		Wait for caches to be removed before continuing.
		
		This disables the existence of caches while locked
	*/
	SharedPtr<JMutexAutoLock> cachelock(m_blockcachelock.waitCaches());

	core::list<v2s16>::Iterator j;
	for(j=list.begin(); j!=list.end(); j++)
	{
		MapSector *sector = m_sectors[*j];
		if(only_blocks)
		{
			sector->deleteBlocks();
		}
		else
		{
			/*
				If sector is in sector cache, remove it from there
			*/
			if(m_sector_cache == sector)
			{
				m_sector_cache = NULL;
			}
			/*
				Remove from map and delete
			*/
			m_sectors.remove(*j);
			delete sector;
		}
	}
}

u32 Map::deleteUnusedSectors(float timeout, bool only_blocks,
		core::list<v3s16> *deleted_blocks)
{
	JMutexAutoLock lock(m_sector_mutex);

	core::list<v2s16> sector_deletion_queue;
	core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapSector *sector = i.getNode()->getValue();
		/*
			Delete sector from memory if it hasn't been used in a long time
		*/
		if(sector->usage_timer > timeout)
		{
			sector_deletion_queue.push_back(i.getNode()->getKey());
			
			if(deleted_blocks != NULL)
			{
				// Collect positions of blocks of sector
				MapSector *sector = i.getNode()->getValue();
				core::list<MapBlock*> blocks;
				sector->getBlocks(blocks);
				for(core::list<MapBlock*>::Iterator i = blocks.begin();
						i != blocks.end(); i++)
				{
					deleted_blocks->push_back((*i)->getPos());
				}
			}
		}
	}
	deleteSectors(sector_deletion_queue, only_blocks);
	return sector_deletion_queue.getSize();
}

void Map::PrintInfo(std::ostream &out)
{
	out<<"Map: ";
}

/*
	ServerMap
*/

ServerMap::ServerMap(std::string savedir, HMParams hmp, MapParams mp):
	Map(dout_server),
	m_heightmap(NULL)
{
	m_savedir = savedir;
	m_map_saving_enabled = false;
	
	try
	{
		// If directory exists, check contents and load if possible
		if(fs::PathExists(m_savedir))
		{
			// If directory is empty, it is safe to save into it.
			if(fs::GetDirListing(m_savedir).size() == 0)
			{
				dstream<<DTIME<<"Server: Empty save directory is valid."
						<<std::endl;
				m_map_saving_enabled = true;
			}
			else
			{
				// Load master heightmap
				loadMasterHeightmap();
				
				// Load sector (0,0) and throw and exception on fail
				if(loadSectorFull(v2s16(0,0)) == false)
					throw LoadError("Failed to load sector (0,0)");

				dstream<<DTIME<<"Server: Successfully loaded master "
						"heightmap and sector (0,0) from "<<savedir<<
						", assuming valid save directory."
						<<std::endl;

				m_map_saving_enabled = true;
				// Map loaded, not creating new one
				return;
			}
		}
		// If directory doesn't exist, it is safe to save to it
		else{
			m_map_saving_enabled = true;
		}
	}
	catch(std::exception &e)
	{
		dstream<<DTIME<<"Server: Failed to load map from "<<savedir
				<<", exception: "<<e.what()<<std::endl;
		dstream<<DTIME<<"Please remove the map or fix it."<<std::endl;
		dstream<<DTIME<<"WARNING: Map saving will be disabled."<<std::endl;
	}

	dstream<<DTIME<<"Initializing new map."<<std::endl;
	
	// Create master heightmap
	ValueGenerator *maxgen =
			ValueGenerator::deSerialize(hmp.randmax);
	ValueGenerator *factorgen =
			ValueGenerator::deSerialize(hmp.randfactor);
	ValueGenerator *basegen =
			ValueGenerator::deSerialize(hmp.base);
	m_heightmap = new UnlimitedHeightmap
			(hmp.blocksize, maxgen, factorgen, basegen);
	
	// Set map parameters
	m_params = mp;
	
	// Create zero sector
	emergeSector(v2s16(0,0));

	// Initially write whole map
	save(false);
}

ServerMap::~ServerMap()
{
	try
	{
		if(m_map_saving_enabled)
		{
			//save(false);
			// Save only changed parts
			save(true);
			dstream<<DTIME<<"Server: saved map to "<<m_savedir<<std::endl;
		}
		else
		{
			dstream<<DTIME<<"Server: map not saved"<<std::endl;
		}
	}
	catch(std::exception &e)
	{
		dstream<<DTIME<<"Server: Failed to save map to "<<m_savedir
				<<", exception: "<<e.what()<<std::endl;
	}
	
	if(m_heightmap != NULL)
		delete m_heightmap;
}

MapSector * ServerMap::emergeSector(v2s16 p2d)
{
	DSTACK("%s: p2d=(%d,%d)",
			__FUNCTION_NAME,
			p2d.X, p2d.Y);
	// Check that it doesn't exist already
	try{
		return getSectorNoGenerate(p2d);
	}
	catch(InvalidPositionException &e)
	{
	}
	
	/*
		Try to load the sector from disk.
	*/
	if(loadSectorFull(p2d) == true)
	{
		return getSectorNoGenerate(p2d);
	}

	/*
		If there is no master heightmap, throw.
	*/
	if(m_heightmap == NULL)
	{
		throw InvalidPositionException("emergeSector(): no heightmap");
	}

	/*
		Do not generate over-limit
	*/
	if(p2d.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p2d.X > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p2d.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p2d.Y > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE)
		throw InvalidPositionException("emergeSector(): pos. over limit");

	/*
		Generate sector and heightmaps
	*/
	
	// Number of heightmaps in sector in each direction
	u16 hm_split = SECTOR_HEIGHTMAP_SPLIT;

	// Heightmap side width
	s16 hm_d = MAP_BLOCKSIZE / hm_split;

	ServerMapSector *sector = new ServerMapSector(this, p2d, hm_split);

	/*dstream<<"Generating sector ("<<p2d.X<<","<<p2d.Y<<")"
			" heightmaps and objects"<<std::endl;*/
	
	// Loop through sub-heightmaps
	for(s16 y=0; y<hm_split; y++)
	for(s16 x=0; x<hm_split; x++)
	{
		v2s16 p_in_sector = v2s16(x,y);
		v2s16 mhm_p = p2d * hm_split + p_in_sector;
		f32 corners[4] = {
			m_heightmap->getGroundHeight(mhm_p+v2s16(0,0)),
			m_heightmap->getGroundHeight(mhm_p+v2s16(1,0)),
			m_heightmap->getGroundHeight(mhm_p+v2s16(1,1)),
			m_heightmap->getGroundHeight(mhm_p+v2s16(0,1)),
		};

		/*dstream<<"p_in_sector=("<<p_in_sector.X<<","<<p_in_sector.Y<<")"
				<<" mhm_p=("<<mhm_p.X<<","<<mhm_p.Y<<")"
				<<std::endl;*/

		FixedHeightmap *hm = new FixedHeightmap(&m_hwrapper,
				mhm_p, hm_d);
		sector->setHeightmap(p_in_sector, hm);

		//TODO: Make these values configurable
		hm->generateContinued(1.0, 0.2, corners);
		//hm->generateContinued(2.0, 0.2, corners);

		//hm->print();
		
	}

	/*
		Generate objects
	*/
	
	core::map<v3s16, u8> *objects = new core::map<v3s16, u8>;
	sector->setObjects(objects);
	
	v2s16 mhm_p = p2d * hm_split;
	f32 corners[4] = {
		m_heightmap->getGroundHeight(mhm_p+v2s16(0,0)*hm_split),
		m_heightmap->getGroundHeight(mhm_p+v2s16(1,0)*hm_split),
		m_heightmap->getGroundHeight(mhm_p+v2s16(1,1)*hm_split),
		m_heightmap->getGroundHeight(mhm_p+v2s16(0,1)*hm_split),
	};
	
	float avgheight = (corners[0]+corners[1]+corners[2]+corners[3])/4.0;
	float avgslope = 0.0;
	avgslope += fabs(avgheight - corners[0]);
	avgslope += fabs(avgheight - corners[1]);
	avgslope += fabs(avgheight - corners[2]);
	avgslope += fabs(avgheight - corners[3]);
	avgslope /= 4.0;
	avgslope /= MAP_BLOCKSIZE;
	//dstream<<"avgslope="<<avgslope<<std::endl;

	float pitness = 0.0;
	v2f32 a;
	a = m_heightmap->getSlope(p2d+v2s16(0,0));
	pitness += -a.X;
	pitness += -a.Y;
	a = m_heightmap->getSlope(p2d+v2s16(0,1));
	pitness += -a.X;
	pitness += a.Y;
	a = m_heightmap->getSlope(p2d+v2s16(1,1));
	pitness += a.X;
	pitness += a.Y;
	a = m_heightmap->getSlope(p2d+v2s16(1,0));
	pitness += a.X;
	pitness += -a.Y;
	pitness /= 4.0;
	pitness /= MAP_BLOCKSIZE;
	//dstream<<"pitness="<<pitness<<std::endl;
	
	/*
		Plant some trees if there is not much slope
	*/
	{
		// Avgslope is the derivative of a hill
		float t = avgslope * avgslope;
		float a = MAP_BLOCKSIZE * m_params.plants_amount;
		u32 tree_max;
		if(t > 0.03)
			tree_max = a / (t/0.03);
		else
			tree_max = a;
		u32 count = (rand()%(tree_max+1));
		//u32 count = tree_max;
		for(u32 i=0; i<count; i++)
		{
			s16 x = (rand()%(MAP_BLOCKSIZE-2))+1;
			s16 z = (rand()%(MAP_BLOCKSIZE-2))+1;
			s16 y = sector->getGroundHeight(v2s16(x,z))+1;
			if(y < WATER_LEVEL)
				continue;
			objects->insert(v3s16(x, y, z),
					SECTOR_OBJECT_TREE_1);
		}
	}
	/*
		Plant some bushes if sector is pit-like
	*/
	{
		// Pitness usually goes at around -0.5...0.5
		u32 bush_max = 0;
		u32 a = MAP_BLOCKSIZE * 3.0 * m_params.plants_amount;
		if(pitness > 0)
			bush_max = (pitness*a*4);
		if(bush_max > a)
			bush_max = a;
		u32 count = (rand()%(bush_max+1));
		for(u32 i=0; i<count; i++)
		{
			s16 x = rand()%(MAP_BLOCKSIZE-0)+0;
			s16 z = rand()%(MAP_BLOCKSIZE-0)+0;
			s16 y = sector->getGroundHeight(v2s16(x,z))+1;
			if(y < WATER_LEVEL)
				continue;
			objects->insert(v3s16(x, y, z),
					SECTOR_OBJECT_BUSH_1);
		}
	}
	/*
		Add ravine (randomly)
	*/
	if(m_params.ravines_amount != 0)
	{
		if(rand()%(s32)(20.0 / m_params.ravines_amount) == 0)
		{
			s16 s = 6;
			s16 x = rand()%(MAP_BLOCKSIZE-s*2-1)+s;
			s16 z = rand()%(MAP_BLOCKSIZE-s*2-1)+s;
			/*s16 x = 8;
			s16 z = 8;*/
			s16 y = sector->getGroundHeight(v2s16(x,z))+1;
			objects->insert(v3s16(x, y, z),
					SECTOR_OBJECT_RAVINE);
		}
	}

	/*
		Insert to container
	*/
	JMutexAutoLock lock(m_sector_mutex);
	m_sectors.insert(p2d, sector);
	
	return sector;
}

MapBlock * ServerMap::emergeBlock(
		v3s16 p,
		bool only_from_disk,
		core::map<v3s16, MapBlock*> &changed_blocks,
		core::map<v3s16, MapBlock*> &lighting_invalidated_blocks
)
{
	DSTACK("%s: p=(%d,%d,%d), only_from_disk=%d",
			__FUNCTION_NAME,
			p.X, p.Y, p.Z, only_from_disk);
			
	/*dstream<<"ServerMap::emergeBlock(): "
			<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<", only_from_disk="<<only_from_disk<<std::endl;*/
	v2s16 p2d(p.X, p.Z);
	s16 block_y = p.Y;
	/*
		This will create or load a sector if not found in memory.
		If block exists on disk, it will be loaded.

		NOTE: On old save formats, this will be slow, as it generates
		      lighting on blocks for them.
	*/
	ServerMapSector *sector = (ServerMapSector*)emergeSector(p2d);
	assert(sector->getId() == MAPSECTOR_SERVER);

	// Try to get a block from the sector
	MapBlock *block = NULL;
	bool not_on_disk = false;
	try{
		block = sector->getBlockNoCreate(block_y);
		if(block->isDummy() == true)
			not_on_disk = true;
		else
			return block;
	}
	catch(InvalidPositionException &e)
	{
		not_on_disk = true;
	}
	
	/*
		If block was not found on disk and not going to generate a
		new one, make sure there is a dummy block in place.
	*/
	if(not_on_disk && only_from_disk)
	{
		if(block == NULL)
		{
			// Create dummy block
			block = new MapBlock(this, p, true);

			// Add block to sector
			sector->insertBlock(block);
		}
		// Done.
		return block;
	}

	//dstream<<"Not found on disk, generating."<<std::endl;

	/*
		Do not generate over-limit
	*/
	if(blockpos_over_limit(p))
		throw InvalidPositionException("emergeBlock(): pos. over limit");

	/*
		OK; Not found.

		Go on generating the block.

		TODO: If a dungeon gets generated so that it's side gets
		      revealed to the outside air, the lighting should be
			  recalculated.
	*/
	
	/*
		If block doesn't exist, create one.
		If it exists, it is a dummy. In that case unDummify() it.
	*/
	if(block == NULL)
	{
		block = sector->createBlankBlockNoInsert(block_y);
	}
	else
	{
		// Remove the block so that nobody can get a half-generated one.
		sector->removeBlock(block);
		// Allocate the block to be a proper one.
		block->unDummify();
	}

	// Randomize a bit. This makes dungeons.
	/*bool low_block_is_empty = false;
	if(rand() % 4 == 0)
		low_block_is_empty = true;*/
	
	const s32 ued = 4;
	bool underground_emptiness[ued*ued*ued];
	for(s32 i=0; i<ued*ued*ued; i++)
	{
		underground_emptiness[i] = ((rand() % 4) == 0);
	}
	
	// This is the basic material of what the visible flat ground
	// will consist of
	u8 material = MATERIAL_GRASS;
	
	s32 lowest_ground_y = 32767;
	
	// DEBUG
	//sector->printHeightmaps();

	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		//dstream<<"emergeBlock: x0="<<x0<<", z0="<<z0<<std::endl;

		float surface_y_f = sector->getGroundHeight(v2s16(x0,z0));
		//assert(surface_y_f > GROUNDHEIGHT_VALID_MINVALUE);
		if(surface_y_f < GROUNDHEIGHT_VALID_MINVALUE)
		{
			dstream<<"WARNING: Surface height not found in sector "
					"for block that is being emerged"<<std::endl;
			surface_y_f = 0.0;
		}

		s16 surface_y = surface_y_f;
		//avg_ground_y += surface_y;
		if(surface_y < lowest_ground_y)
			lowest_ground_y = surface_y;

		s32 surface_depth = 0;
		
		float slope = sector->getSlope(v2s16(x0,z0)).getLength();
		
		float min_slope = 0.45;
		float max_slope = 0.85;
		float min_slope_depth = 5.0;
		float max_slope_depth = 0;
		if(slope < min_slope)
			surface_depth = min_slope_depth;
		else if(slope > max_slope)
			surface_depth = max_slope_depth;
		else
			surface_depth = (1.-(slope-min_slope)/max_slope) * min_slope_depth;

		for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
		{
			s16 real_y = block_y * MAP_BLOCKSIZE + y0;
			MapNode n;
			/*
				Calculate lighting
				
				NOTE: If there are some man-made structures above the
				newly created block, they won't be taken into account.
			*/
			if(real_y > surface_y)
				n.setLight(LIGHT_SUN);
			/*
				Calculate material
			*/
			// If node is very low
			if(real_y <= surface_y - 7){
				// Create dungeons
				if(underground_emptiness[
						ued*ued*(z0*ued/MAP_BLOCKSIZE)
						+ued*(y0*ued/MAP_BLOCKSIZE)
						+(x0*ued/MAP_BLOCKSIZE)])
				{
					n.d = MATERIAL_AIR;
				}
				else
				{
					n.d = MATERIAL_STONE;
				}
			}
			// If node is under surface level
			else if(real_y <= surface_y - surface_depth)
				n.d = MATERIAL_STONE;
			// If node is at or under heightmap y
			else if(real_y <= surface_y)
			{
				// If under water level, it's mud
				if(real_y < WATER_LEVEL)
					n.d = MATERIAL_MUD;
				// Else it's the main material
				else
					n.d = material;
			}
			// If node is over heightmap y
			else{
				// If under water level, it's water
				if(real_y < WATER_LEVEL)
				{
					n.d = MATERIAL_WATER;
					n.setLight(diminish_light(LIGHT_SUN, WATER_LEVEL-real_y+1));
				}
				// else air
				else
					n.d = MATERIAL_AIR;
			}
			block->setNode(v3s16(x0,y0,z0), n);
		}
	}

	/*
		Calculate is_underground
	*/
	// Probably underground if the highest part of block is under lowest
	// ground height
	bool is_underground = (block_y+1) * MAP_BLOCKSIZE < lowest_ground_y;
	block->setIsUnderground(is_underground);

	/*
		Force lighting update if underground.
		This is needed because of ravines.
	*/

	if(is_underground)
	{
		lighting_invalidated_blocks[block->getPos()] = block;
	}
	
	/*
		Add some minerals
	*/

	if(is_underground)
	{
		s16 underground_level = lowest_ground_y/MAP_BLOCKSIZE - block_y;
		for(s16 i=0; i<underground_level*3; i++)
		{
			if(rand()%2 == 0)
			{
				v3s16 cp(
					(rand()%(MAP_BLOCKSIZE-2))+1,
					(rand()%(MAP_BLOCKSIZE-2))+1,
					(rand()%(MAP_BLOCKSIZE-2))+1
				);

				MapNode n;
				n.d = MATERIAL_MESE;
				
				if(is_ground_material(block->getNode(cp).d))
					if(rand()%8 == 0)
						block->setNode(cp, n);

				for(u16 i=0; i<26; i++)
				{
					if(is_ground_material(block->getNode(cp+g_26dirs[i]).d))
						if(rand()%8 == 0)
							block->setNode(cp+g_26dirs[i], n);
				}
			}
		}
	}
	
	/*
		Create a few rats in empty blocks underground
	*/
	if(is_underground)
	{
		//for(u16 i=0; i<2; i++)
		{
			v3s16 cp(
				(rand()%(MAP_BLOCKSIZE-2))+1,
				(rand()%(MAP_BLOCKSIZE-2))+1,
				(rand()%(MAP_BLOCKSIZE-2))+1
			);

			// Check that the place is empty
			//if(!is_ground_material(block->getNode(cp).d))
			if(1)
			{
				RatObject *obj = new RatObject(NULL, -1, intToFloat(cp));
				block->addObject(obj);
			}
		}
	}
	
	/*
		Add block to sector.
	*/
	sector->insertBlock(block);
	
	/*
		Do some interpolation for dungeons
	*/

#if 0	
	{
	TimeTaker timer("interpolation", g_device);
	
	MapVoxelManipulator vmanip(this);
	
	v3s16 relpos = block->getPosRelative();

	vmanip.interpolate(VoxelArea(relpos-v3s16(1,1,1),
			relpos+v3s16(1,1,1)*(MAP_BLOCKSIZE+1)));
	/*vmanip.interpolate(VoxelArea(relpos,
			relpos+v3s16(1,1,1)*(MAP_BLOCKSIZE-1)));*/
	
	core::map<v3s16, MapBlock*> modified_blocks;
	vmanip.blitBack(modified_blocks);
	dstream<<"blitBack modified "<<modified_blocks.size()
			<<" blocks"<<std::endl;

	// Add modified blocks to changed_blocks and lighting_invalidated_blocks
	for(core::map<v3s16, MapBlock*>::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();

		changed_blocks.insert(block->getPos(), block);
		//lighting_invalidated_blocks.insert(block->getPos(), block);
	}

	}
#endif

	/*
		Sector object stuff
	*/
		
	// An y-wise container of changed blocks
	core::map<s16, MapBlock*> changed_blocks_sector;

	/*
		Check if any sector's objects can be placed now.
		If so, place them.
	*/
	core::map<v3s16, u8> *objects = sector->getObjects();
	core::list<v3s16> objects_to_remove;
	for(core::map<v3s16, u8>::Iterator i = objects->getIterator();
			i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		v2s16 p2d(p.X,p.Z);
		u8 d = i.getNode()->getValue();

		//v3s16 p = p_sector - v3s16(0, block_y*MAP_BLOCKSIZE, 0);
		
		try
		{

		if(d == SECTOR_OBJECT_TEST)
		{
			if(sector->isValidArea(p + v3s16(0,0,0),
					p + v3s16(0,0,0), &changed_blocks_sector))
			{
				MapNode n;
				n.d = MATERIAL_LIGHT;
				sector->setNode(p, n);
				objects_to_remove.push_back(p);
			}
		}
		else if(d == SECTOR_OBJECT_TREE_1)
		{
			v3s16 p_min = p + v3s16(-1,0,-1);
			v3s16 p_max = p + v3s16(1,4,1);
			if(sector->isValidArea(p_min, p_max,
					&changed_blocks_sector))
			{
				MapNode n;
				n.d = MATERIAL_TREE;
				sector->setNode(p+v3s16(0,0,0), n);
				sector->setNode(p+v3s16(0,1,0), n);
				sector->setNode(p+v3s16(0,2,0), n);
				sector->setNode(p+v3s16(0,3,0), n);

				n.d = MATERIAL_LEAVES;

				sector->setNode(p+v3s16(0,4,0), n);
				
				sector->setNode(p+v3s16(-1,4,0), n);
				sector->setNode(p+v3s16(1,4,0), n);
				sector->setNode(p+v3s16(0,4,-1), n);
				sector->setNode(p+v3s16(0,4,1), n);
				sector->setNode(p+v3s16(1,4,1), n);
				sector->setNode(p+v3s16(-1,4,1), n);
				sector->setNode(p+v3s16(-1,4,-1), n);
				sector->setNode(p+v3s16(1,4,-1), n);

				sector->setNode(p+v3s16(-1,3,0), n);
				sector->setNode(p+v3s16(1,3,0), n);
				sector->setNode(p+v3s16(0,3,-1), n);
				sector->setNode(p+v3s16(0,3,1), n);
				sector->setNode(p+v3s16(1,3,1), n);
				sector->setNode(p+v3s16(-1,3,1), n);
				sector->setNode(p+v3s16(-1,3,-1), n);
				sector->setNode(p+v3s16(1,3,-1), n);
				
				objects_to_remove.push_back(p);
				
				// Lighting has to be recalculated for this one.
				sector->getBlocksInArea(p_min, p_max, 
						lighting_invalidated_blocks);
			}
		}
		else if(d == SECTOR_OBJECT_BUSH_1)
		{
			if(sector->isValidArea(p + v3s16(0,0,0),
					p + v3s16(0,0,0), &changed_blocks_sector))
			{
				MapNode n;
				n.d = MATERIAL_LEAVES;
				sector->setNode(p+v3s16(0,0,0), n);
				
				objects_to_remove.push_back(p);
			}
		}
		else if(d == SECTOR_OBJECT_RAVINE)
		{
			s16 maxdepth = -20;
			v3s16 p_min = p + v3s16(-6,maxdepth,-6);
			v3s16 p_max = p + v3s16(6,6,6);
			if(sector->isValidArea(p_min, p_max,
					&changed_blocks_sector))
			{
				MapNode n;
				n.d = MATERIAL_STONE;
				MapNode n2;
				n2.d = MATERIAL_AIR;
				s16 depth = maxdepth + (rand()%10);
				s16 z = 0;
				s16 minz = -6 - (-2);
				s16 maxz = 6 -1;
				for(s16 x=-6; x<=6; x++)
				{
					z += -1 + (rand()%3);
					if(z < minz)
						z = minz;
					if(z > maxz)
						z = maxz;
					for(s16 y=depth+(rand()%2); y<=6; y++)
					{
						/*std::cout<<"("<<p2.X<<","<<p2.Y<<","<<p2.Z<<")"
								<<std::endl;*/
						{
							v3s16 p2 = p + v3s16(x,y,z-2);
							if(is_ground_material(sector->getNode(p2).d))
								sector->setNode(p2, n);
						}
						{
							v3s16 p2 = p + v3s16(x,y,z-1);
							if(is_ground_material(sector->getNode(p2).d))
								sector->setNode(p2, n2);
						}
						{
							v3s16 p2 = p + v3s16(x,y,z+0);
							if(is_ground_material(sector->getNode(p2).d))
								sector->setNode(p2, n2);
						}
						{
							v3s16 p2 = p + v3s16(x,y,z+1);
							if(is_ground_material(sector->getNode(p2).d))
								sector->setNode(p2, n);
						}

						//if(sector->getNode(p+v3s16(x,y,z+1)).solidness()==2)
						//if(p.Y+y <= sector->getGroundHeight(p2d+v2s16(x,z-2))+0.5)
					}
				}
				
				objects_to_remove.push_back(p);
				
				// Lighting has to be recalculated for this one.
				sector->getBlocksInArea(p_min, p_max, 
						lighting_invalidated_blocks);
			}
		}
		else
		{
			dstream<<"ServerMap::emergeBlock(): "
					"Invalid heightmap object"
					<<std::endl;
		}

		}//try
		catch(InvalidPositionException &e)
		{
			dstream<<"WARNING: "<<__FUNCTION_NAME
					<<": while inserting object "<<(int)d
					<<" to ("<<p.X<<","<<p.Y<<","<<p.Z<<"):"
					<<" InvalidPositionException.what()="
					<<e.what()<<std::endl;
			// This is not too fatal and seems to happen sometimes.
			assert(0);
		}
	}

	for(core::list<v3s16>::Iterator i = objects_to_remove.begin();
			i != objects_to_remove.end(); i++)
	{
		objects->remove(*i);
	}

	for(core::map<s16, MapBlock*>::Iterator
			i = changed_blocks_sector.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();

		changed_blocks.insert(block->getPos(), block);
	}

	return block;
}

void ServerMap::createDir(std::string path)
{
	if(fs::CreateDir(path) == false)
	{
		m_dout<<DTIME<<"ServerMap: Failed to create directory "
				<<"\""<<path<<"\""<<std::endl;
		throw BaseException("ServerMap failed to create directory");
	}
}

std::string ServerMap::getSectorSubDir(v2s16 pos)
{
	char cc[9];
	snprintf(cc, 9, "%.4x%.4x",
			(unsigned int)pos.X&0xffff,
			(unsigned int)pos.Y&0xffff);

	return std::string(cc);
}

std::string ServerMap::getSectorDir(v2s16 pos)
{
	return m_savedir + "/sectors/" + getSectorSubDir(pos);
}

v2s16 ServerMap::getSectorPos(std::string dirname)
{
	if(dirname.size() != 8)
		throw InvalidFilenameException("Invalid sector directory name");
	unsigned int x, y;
	int r = sscanf(dirname.c_str(), "%4x%4x", &x, &y);
	if(r != 2)
		throw InvalidFilenameException("Invalid sector directory name");
	v2s16 pos((s16)x, (s16)y);
	return pos;
}

v3s16 ServerMap::getBlockPos(std::string sectordir, std::string blockfile)
{
	v2s16 p2d = getSectorPos(sectordir);

	if(blockfile.size() != 4){
		throw InvalidFilenameException("Invalid block filename");
	}
	unsigned int y;
	int r = sscanf(blockfile.c_str(), "%4x", &y);
	if(r != 1)
		throw InvalidFilenameException("Invalid block filename");
	return v3s16(p2d.X, y, p2d.Y);
}

// Debug helpers
#define ENABLE_SECTOR_SAVING 1
#define ENABLE_SECTOR_LOADING 1
#define ENABLE_BLOCK_SAVING 1
#define ENABLE_BLOCK_LOADING 1

void ServerMap::save(bool only_changed)
{
	DSTACK(__FUNCTION_NAME);
	if(m_map_saving_enabled == false)
	{
		dstream<<DTIME<<"WARNING: Not saving map, saving disabled."<<std::endl;
		return;
	}
	
	if(only_changed == false)
		dstream<<DTIME<<"ServerMap: Saving whole map, this can take time."
				<<std::endl;
	
	saveMasterHeightmap();
	
	u32 sector_meta_count = 0;
	u32 block_count = 0;
	
	{ //sectorlock
	JMutexAutoLock lock(m_sector_mutex);
	
	core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
	for(; i.atEnd() == false; i++)
	{
		ServerMapSector *sector = (ServerMapSector*)i.getNode()->getValue();
		assert(sector->getId() == MAPSECTOR_SERVER);
		
		if(ENABLE_SECTOR_SAVING)
		{
			if(sector->differs_from_disk || only_changed == false)
			{
				saveSectorMeta(sector);
				sector_meta_count++;
			}
		}
		if(ENABLE_BLOCK_SAVING)
		{
			core::list<MapBlock*> blocks;
			sector->getBlocks(blocks);
			core::list<MapBlock*>::Iterator j;
			for(j=blocks.begin(); j!=blocks.end(); j++)
			{
				MapBlock *block = *j;
				if(block->getChangedFlag() || only_changed == false)
				{
					saveBlock(block);
					block_count++;
				}
			}
		}
	}

	}//sectorlock
	
	u32 deleted_count = 0;
	deleted_count = deleteUnusedSectors
			(SERVERMAP_DELETE_UNUSED_SECTORS_TIMEOUT);
	
	/*
		Only print if something happened or saved whole map
	*/
	if(only_changed == false || sector_meta_count != 0
			|| block_count != 0 || deleted_count != 0)
	{
		dstream<<DTIME<<"ServerMap: Written: "
				<<sector_meta_count<<" sector metadata files, "
				<<block_count<<" block files, "
				<<deleted_count<<" sectors unloaded from memory."
				<<std::endl;
	}
}

void ServerMap::loadAll()
{
	DSTACK(__FUNCTION_NAME);
	dstream<<DTIME<<"ServerMap: Loading map..."<<std::endl;

	loadMasterHeightmap();

	std::vector<fs::DirListNode> list = fs::GetDirListing(m_savedir+"/sectors/");

	dstream<<DTIME<<"There are "<<list.size()<<" sectors."<<std::endl;
	
	JMutexAutoLock lock(m_sector_mutex);
	
	s32 counter = 0;
	s32 printed_counter = -100000;
	s32 count = list.size();

	std::vector<fs::DirListNode>::iterator i;
	for(i=list.begin(); i!=list.end(); i++)
	{
		if(counter > printed_counter + 10)
		{
			dstream<<DTIME<<counter<<"/"<<count<<std::endl;
			printed_counter = counter;
		}
		counter++;

		MapSector *sector = NULL;

		// We want directories
		if(i->dir == false)
			continue;
		try{
			sector = loadSectorMeta(i->name);
		}
		catch(InvalidFilenameException &e)
		{
			// This catches unknown crap in directory
		}
		
		if(ENABLE_BLOCK_LOADING)
		{
			std::vector<fs::DirListNode> list2 = fs::GetDirListing
					(m_savedir+"/sectors/"+i->name);
			std::vector<fs::DirListNode>::iterator i2;
			for(i2=list2.begin(); i2!=list2.end(); i2++)
			{
				// We want files
				if(i2->dir)
					continue;
				try{
					loadBlock(i->name, i2->name, sector);
				}
				catch(InvalidFilenameException &e)
				{
					// This catches unknown crap in directory
				}
			}
		}
	}
	dstream<<DTIME<<"ServerMap: Map loaded."<<std::endl;
}

void ServerMap::saveMasterHeightmap()
{
	DSTACK(__FUNCTION_NAME);
	createDir(m_savedir);
	
	std::string fullpath = m_savedir + "/master_heightmap";
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open master heightmap");
	
	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST;

#if 0
	SharedBuffer<u8> hmdata = m_heightmap->serialize(version);
	/*
		[0] u8 serialization version
		[1] X master heightmap
	*/
	u32 fullsize = 1 + hmdata.getSize();
	SharedBuffer<u8> data(fullsize);

	data[0] = version;
	memcpy(&data[1], *hmdata, hmdata.getSize());

	o.write((const char*)*data, fullsize);
#endif
	
	m_heightmap->serialize(o, version);
}

void ServerMap::loadMasterHeightmap()
{
	DSTACK(__FUNCTION_NAME);
	std::string fullpath = m_savedir + "/master_heightmap";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
		throw FileNotGoodException("Cannot open master heightmap");
	
	if(m_heightmap != NULL)
		delete m_heightmap;
		
	m_heightmap = UnlimitedHeightmap::deSerialize(is);
}

void ServerMap::saveSectorMeta(ServerMapSector *sector)
{
	DSTACK(__FUNCTION_NAME);
	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST;
	// Get destination
	v2s16 pos = sector->getPos();
	createDir(m_savedir);
	createDir(m_savedir+"/sectors");
	std::string dir = getSectorDir(pos);
	createDir(dir);
	
	std::string fullpath = dir + "/heightmap";
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open master heightmap");

	sector->serialize(o, version);
	
	sector->differs_from_disk = false;
}

MapSector* ServerMap::loadSectorMeta(std::string dirname)
{
	DSTACK(__FUNCTION_NAME);
	// Get destination
	v2s16 p2d = getSectorPos(dirname);
	std::string dir = m_savedir + "/sectors/" + dirname;
	
	std::string fullpath = dir + "/heightmap";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
		throw FileNotGoodException("Cannot open sector heightmap");

	ServerMapSector *sector = ServerMapSector::deSerialize
			(is, this, p2d, &m_hwrapper, m_sectors);
	
	sector->differs_from_disk = false;

	return sector;
}

bool ServerMap::loadSectorFull(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);
	std::string sectorsubdir = getSectorSubDir(p2d);

	MapSector *sector = NULL;

	JMutexAutoLock lock(m_sector_mutex);

	try{
		sector = loadSectorMeta(sectorsubdir);
	}
	catch(InvalidFilenameException &e)
	{
		return false;
	}
	catch(FileNotGoodException &e)
	{
		return false;
	}
	catch(std::exception &e)
	{
		return false;
	}

	if(ENABLE_BLOCK_LOADING)
	{
		std::vector<fs::DirListNode> list2 = fs::GetDirListing
				(m_savedir+"/sectors/"+sectorsubdir);
		std::vector<fs::DirListNode>::iterator i2;
		for(i2=list2.begin(); i2!=list2.end(); i2++)
		{
			// We want files
			if(i2->dir)
				continue;
			try{
				loadBlock(sectorsubdir, i2->name, sector);
			}
			catch(InvalidFilenameException &e)
			{
				// This catches unknown crap in directory
			}
		}
	}
	return true;
}

#if 0
bool ServerMap::deFlushSector(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);
	// See if it already exists in memory
	try{
		MapSector *sector = getSectorNoGenerate(p2d);
		return true;
	}
	catch(InvalidPositionException &e)
	{
		/*
			Try to load the sector from disk.
		*/
		if(loadSectorFull(p2d) == true)
		{
			return true;
		}
	}
	return false;
}
#endif

void ServerMap::saveBlock(MapBlock *block)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Dummy blocks are not written
	*/
	if(block->isDummy())
	{
		/*v3s16 p = block->getPos();
		dstream<<"ServerMap::saveBlock(): WARNING: Not writing dummy block "
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		return;
	}

	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST;
	// Get destination
	v3s16 p3d = block->getPos();
	v2s16 p2d(p3d.X, p3d.Z);
	createDir(m_savedir);
	createDir(m_savedir+"/sectors");
	std::string dir = getSectorDir(p2d);
	createDir(dir);
	
	// Block file is map/sectors/xxxxxxxx/xxxx
	char cc[5];
	snprintf(cc, 5, "%.4x", (unsigned int)p3d.Y&0xffff);
	std::string fullpath = dir + "/" + cc;
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open block data");

	/*
		[0] u8 serialization version
		[1] data
	*/
	o.write((char*)&version, 1);
	
	block->serialize(o, version);

	/*
		Versions up from 9 have block objects.
	*/
	if(version >= 9)
	{
		block->serializeObjects(o, version);
	}
	
	// We just wrote it to the disk
	block->resetChangedFlag();
}

void ServerMap::loadBlock(std::string sectordir, std::string blockfile, MapSector *sector)
{
	DSTACK(__FUNCTION_NAME);

	try{

	// Block file is map/sectors/xxxxxxxx/xxxx
	std::string fullpath = m_savedir+"/sectors/"+sectordir+"/"+blockfile;
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
		throw FileNotGoodException("Cannot open block file");

	v3s16 p3d = getBlockPos(sectordir, blockfile);
	v2s16 p2d(p3d.X, p3d.Z);
	
	assert(sector->getPos() == p2d);
	
	u8 version = SER_FMT_VER_INVALID;
	is.read((char*)&version, 1);

	/*u32 block_size = MapBlock::serializedLength(version);
	SharedBuffer<u8> data(block_size);
	is.read((char*)*data, block_size);*/

	// This will always return a sector because we're the server
	//MapSector *sector = emergeSector(p2d);

	MapBlock *block = NULL;
	bool created_new = false;
	try{
		block = sector->getBlockNoCreate(p3d.Y);
	}
	catch(InvalidPositionException &e)
	{
		block = sector->createBlankBlockNoInsert(p3d.Y);
		created_new = true;
	}
	
	// deserialize block data
	block->deSerialize(is, version);
	
	/*
		Versions up from 9 have block objects.
	*/
	if(version >= 9)
	{
		block->updateObjects(is, version, NULL);
	}

	if(created_new)
		sector->insertBlock(block);
	
	/*
		Convert old formats to new and save
	*/

	// Save old format blocks in new format
	if(version < SER_FMT_VER_HIGHEST)
	{
		saveBlock(block);
	}
	
	// We just loaded it from the disk, so it's up-to-date.
	block->resetChangedFlag();

	}
	catch(SerializationError &e)
	{
		dstream<<"WARNING: Invalid block data on disk "
				"(SerializationError). Ignoring."
				<<std::endl;
	}
}

// Gets from master heightmap
void ServerMap::getSectorCorners(v2s16 p2d, s16 *corners)
{
	assert(m_heightmap != NULL);
	/*
		Corner definition:
		v2s16(0,0),
		v2s16(1,0),
		v2s16(1,1),
		v2s16(0,1),
	*/
	corners[0] = m_heightmap->getGroundHeight
			((p2d+v2s16(0,0))*SECTOR_HEIGHTMAP_SPLIT);
	corners[1] = m_heightmap->getGroundHeight
			((p2d+v2s16(1,0))*SECTOR_HEIGHTMAP_SPLIT);
	corners[2] = m_heightmap->getGroundHeight
			((p2d+v2s16(1,1))*SECTOR_HEIGHTMAP_SPLIT);
	corners[3] = m_heightmap->getGroundHeight
			((p2d+v2s16(0,1))*SECTOR_HEIGHTMAP_SPLIT);
}

void ServerMap::PrintInfo(std::ostream &out)
{
	out<<"ServerMap: ";
}

/*
	ClientMap
*/

ClientMap::ClientMap(
		Client *client,
		video::SMaterial *materials,
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id
):
	Map(dout_client),
	scene::ISceneNode(parent, mgr, id),
	m_client(client),
	m_materials(materials),
	mesh(NULL)
{
	/*m_box = core::aabbox3d<f32>(0,0,0,
			map->getW()*BS, map->getH()*BS, map->getD()*BS);*/
	/*m_box = core::aabbox3d<f32>(0,0,0,
			map->getSizeNodes().X * BS,
			map->getSizeNodes().Y * BS,
			map->getSizeNodes().Z * BS);*/
	m_box = core::aabbox3d<f32>(-BS*1000000,-BS*1000000,-BS*1000000,
			BS*1000000,BS*1000000,BS*1000000);

	mesh_mutex.Init();
}

ClientMap::~ClientMap()
{
	JMutexAutoLock lock(mesh_mutex);
	
	if(mesh != NULL)
	{
		mesh->drop();
		mesh = NULL;
	}
}

MapSector * ClientMap::emergeSector(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);
	// Check that it doesn't exist already
	try{
		return getSectorNoGenerate(p2d);
	}
	catch(InvalidPositionException &e)
	{
	}
	
	// Create a sector with no heightmaps
	ClientMapSector *sector = new ClientMapSector(this, p2d);
	
	{
		JMutexAutoLock lock(m_sector_mutex);
		m_sectors.insert(p2d, sector);
	}
	
	return sector;
}

void ClientMap::deSerializeSector(v2s16 p2d, std::istream &is)
{
	DSTACK(__FUNCTION_NAME);
	ClientMapSector *sector = NULL;

	JMutexAutoLock lock(m_sector_mutex);
	
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p2d);

	if(n != NULL)
	{
		sector = (ClientMapSector*)n->getValue();
		assert(sector->getId() == MAPSECTOR_CLIENT);
	}
	else
	{
		sector = new ClientMapSector(this, p2d);
		{
			JMutexAutoLock lock(m_sector_mutex);
			m_sectors.insert(p2d, sector);
		}
	}

	sector->deSerialize(is);
}

void ClientMap::renderMap(video::IVideoDriver* driver,
	video::SMaterial *materials, s32 pass)
{
	//m_dout<<DTIME<<"Rendering map..."<<std::endl;
	DSTACK(__FUNCTION_NAME);

	bool is_transparent_pass = pass == scene::ESNRP_TRANSPARENT;
#if 0
	/*
		Draw master heightmap mesh
	*/
	
	{
		JMutexAutoLock lock(mesh_mutex);
		if(mesh != NULL)
		{
			u32 c = mesh->getMeshBufferCount();

			for(u32 i=0; i<c; i++)
			{
				scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
				const video::SMaterial& material = buf->getMaterial();
				video::IMaterialRenderer* rnd =
						driver->getMaterialRenderer(material.MaterialType);
				bool transparent = (rnd && rnd->isTransparent());
				// Render transparent on transparent pass and likewise.
				if(transparent == is_transparent_pass)
				{
					driver->setMaterial(buf->getMaterial());
					driver->drawMeshBuffer(buf);
				}
			}
		}
	}
#endif

	/*
		Get time for measuring timeout.
		
		Measuring time is very useful for long delays when the
		machine is swapping a lot.
	*/
	int time1 = time(0);

	/*
		Collect all blocks that are in the view range

		Should not optimize more here as we want to auto-update
		all changed nodes in viewing range at the next step.
	*/

	s16 viewing_range_nodes;
	bool viewing_range_all;
	{
		JMutexAutoLock lock(g_range_mutex);
		viewing_range_nodes = g_viewing_range_nodes;
		viewing_range_all = g_viewing_range_all;
	}

	m_camera_mutex.Lock();
	v3f camera_position = m_camera_position;
	v3f camera_direction = m_camera_direction;
	m_camera_mutex.Unlock();

	/*
		Get all blocks and draw all visible ones
	*/

	v3s16 cam_pos_nodes(
			camera_position.X / BS,
			camera_position.Y / BS,
			camera_position.Z / BS);

	v3s16 box_nodes_d = viewing_range_nodes * v3s16(1,1,1);

	v3s16 p_nodes_min = cam_pos_nodes - box_nodes_d;
	v3s16 p_nodes_max = cam_pos_nodes + box_nodes_d;

	// Take a fair amount as we will be dropping more out later
	v3s16 p_blocks_min(
			p_nodes_min.X / MAP_BLOCKSIZE - 1,
			p_nodes_min.Y / MAP_BLOCKSIZE - 1,
			p_nodes_min.Z / MAP_BLOCKSIZE - 1);
	v3s16 p_blocks_max(
			p_nodes_max.X / MAP_BLOCKSIZE + 1,
			p_nodes_max.Y / MAP_BLOCKSIZE + 1,
			p_nodes_max.Z / MAP_BLOCKSIZE + 1);
	
	u32 vertex_count = 0;
	
	core::map<v2s16, MapSector*>::Iterator si;

	//NOTE: The sectors map should be locked but we're not doing it
	// because it'd cause too much delays

	si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		{
			static int timecheck_counter = 0;
			timecheck_counter++;
			if(timecheck_counter > 50)
			{
				int time2 = time(0);
				if(time2 > time1 + 4)
				{
					dstream<<"ClientMap::renderMap(): "
						"Rendering takes ages, returning."
						<<std::endl;
					return;
				}
			}
		}

		MapSector *sector = si.getNode()->getValue();
		v2s16 sp = sector->getPos();
		
		if(viewing_range_all == false)
		{
			if(sp.X < p_blocks_min.X
			|| sp.X > p_blocks_max.X
			|| sp.Y < p_blocks_min.Z
			|| sp.Y > p_blocks_max.Z)
				continue;
		}

		core::list< MapBlock * > sectorblocks;
		sector->getBlocks(sectorblocks);
		
		/*
			Draw blocks
		*/

		core::list< MapBlock * >::Iterator i;
		for(i=sectorblocks.begin(); i!=sectorblocks.end(); i++)
		{
			MapBlock *block = *i;

			/*
				Compare block position to camera position, skip
				if not seen on display
			*/
			
			v3s16 blockpos_nodes = block->getPosRelative();
			
			// Block center position
			v3f blockpos(
					((float)blockpos_nodes.X + MAP_BLOCKSIZE/2) * BS,
					((float)blockpos_nodes.Y + MAP_BLOCKSIZE/2) * BS,
					((float)blockpos_nodes.Z + MAP_BLOCKSIZE/2) * BS
			);

			// Block position relative to camera
			v3f blockpos_relative = blockpos - camera_position;

			// Distance in camera direction (+=front, -=back)
			f32 dforward = blockpos_relative.dotProduct(camera_direction);

			// Total distance
			f32 d = blockpos_relative.getLength();
			
			if(viewing_range_all == false)
			{
				// If block is far away, don't draw it
				if(d > viewing_range_nodes * BS)
					continue;
			}
			
			// Maximum radius of a block
			f32 block_max_radius = 0.5*1.44*1.44*MAP_BLOCKSIZE*BS;
			
			// If block is (nearly) touching the camera, don't
			// bother validating further (that is, render it anyway)
			if(d > block_max_radius * 1.5)
			{
				// Cosine of the angle between the camera direction
				// and the block direction (camera_direction is an unit vector)
				f32 cosangle = dforward / d;
				
				// Compensate for the size of the block
				// (as the block has to be shown even if it's a bit off FOV)
				// This is an estimate.
				cosangle += block_max_radius / dforward;

				// If block is not in the field of view, skip it
				//if(cosangle < cos(FOV_ANGLE/2))
				if(cosangle < cos(FOV_ANGLE/2. * 4./3.))
					continue;
			}
			
			/*
				Draw the faces of the block
			*/
			
			{
				JMutexAutoLock lock(block->mesh_mutex);

				// Cancel if block has no mesh
				if(block->mesh == NULL)
					continue;

				u32 c = block->mesh->getMeshBufferCount();

				for(u32 i=0; i<c; i++)
				{
					scene::IMeshBuffer *buf = block->mesh->getMeshBuffer(i);
					const video::SMaterial& material = buf->getMaterial();
					video::IMaterialRenderer* rnd =
							driver->getMaterialRenderer(material.MaterialType);
					bool transparent = (rnd && rnd->isTransparent());
					// Render transparent on transparent pass and likewise.
					if(transparent == is_transparent_pass)
					{
						driver->setMaterial(buf->getMaterial());
						driver->drawMeshBuffer(buf);
						vertex_count += buf->getVertexCount();
					}
				}
			}
		} // foreach sectorblocks
	}

	/*dstream<<"renderMap(): is_transparent_pass="<<is_transparent_pass
			<<", rendered "<<vertex_count<<" vertices."<<std::endl;*/
}

void ClientMap::updateMesh()
{
#if 0
	DSTACK(__FUNCTION_NAME);
	//TODO
	/*
		Check what sectors don't draw anything useful at ground level
		and create a mesh of the rough heightmap at those positions.
	*/

	m_camera_mutex.Lock();
	v3f camera_position = m_camera_position;
	v3f camera_direction = m_camera_direction;
	m_camera_mutex.Unlock();

	v3s16 cam_pos_nodes(
			camera_position.X / BS,
			camera_position.Y / BS,
			camera_position.Z / BS);

	v3s16 box_nodes_d = HEIGHTMAP_RANGE_NODES * v3s16(1,1,1);

	v3s16 p_nodes_min = cam_pos_nodes - box_nodes_d;
	v3s16 p_nodes_max = cam_pos_nodes + box_nodes_d;

	// Take a fair amount as we will be dropping more out later
	v3s16 p_blocks_min(
			p_nodes_min.X / MAP_BLOCKSIZE - 1,
			p_nodes_min.Y / MAP_BLOCKSIZE - 1,
			p_nodes_min.Z / MAP_BLOCKSIZE - 1);
	v3s16 p_blocks_max(
			p_nodes_max.X / MAP_BLOCKSIZE + 1,
			p_nodes_max.Y / MAP_BLOCKSIZE + 1,
			p_nodes_max.Z / MAP_BLOCKSIZE + 1);
	
	/*
		Initialize new mesh
	*/
	
	scene::SMesh *mesh_new = new scene::SMesh();
	//scene::IMeshBuffer *buf = NULL;
	scene::SMeshBuffer *buf = NULL;

	u8 material_in_use = 0;

	/*
		Loop through sectors
	*/
	
	for(core::map<v2s16, MapSector*>::Iterator
			si = m_sectors.getIterator();
			si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();
		
		if(sector->getId() != MAPSECTOR_CLIENT)
		{
			dstream<<"WARNING: Client has a non-client sector"
					<<std::endl;
			continue;
		}
		
		ClientMapSector *cs = (ClientMapSector*)sector;

		v2s16 sp = sector->getPos();

		if(sp.X < p_blocks_min.X
		|| sp.X > p_blocks_max.X
		|| sp.Y < p_blocks_min.Z
		|| sp.Y > p_blocks_max.Z)
			continue;
		
		/*
			Get some ground level info
		*/
		
		s16 a = -5;

		s16 cn[4] = 
		{
			cs->getCorner(0)+a,
			cs->getCorner(1)+a,
			cs->getCorner(2)+a,
			cs->getCorner(3)+a,
		};
		s16 cn_avg = (cn[0]+cn[1]+cn[2]+cn[3])/4;
		s16 cn_min = 32767;
		s16 cn_max = -32768;
		for(s16 i=0; i<4; i++)
		{
			if(cn[i] < cn_min)
				cn_min = cn[i];
			if(cn[i] > cn_max)
				cn_max = cn[i];
		}
		s16 cn_slope = cn_max - cn_min;
		
		/*
			Generate this part of the heightmap mesh
		*/

		u8 material;
		if(cn_avg + MAP_BLOCKSIZE/4 <= WATER_LEVEL)
			material = 0;
		else if(cn_slope <= MAP_BLOCKSIZE)
			material = 1;
		else
			material = 2;

		if(material != material_in_use || buf == NULL)
		{
			// Try to get a meshbuffer associated with the material
			buf = (scene::SMeshBuffer*)mesh_new->getMeshBuffer
					(g_mesh_materials[material]);
			// If not found, create one
			if(buf == NULL)
			{
				// This is a "Standard MeshBuffer",
				// it's a typedeffed CMeshBuffer<video::S3DVertex>
				buf = new scene::SMeshBuffer();

				// Set material
				buf->Material = g_mesh_materials[material];
				// Use VBO
				//buf->setHardwareMappingHint(scene::EHM_STATIC);
				// Add to mesh
				mesh_new->addMeshBuffer(buf);
				// Mesh grabbed it
				buf->drop();
			}
			material_in_use = material;
		}

		// Sector side width in floating-point units
		f32 sd = BS * MAP_BLOCKSIZE;
		// Sector position in global floating-point units
		v3f spf = v3f((f32)sp.X, 0, (f32)sp.Y) * sd;

		//video::SColor c(255,255,255,255);
		u8 cc = 180;
		video::SColor c(255,cc,cc,cc);
		
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(spf.X,   (f32)BS*cn[0],spf.Z,   0,0,0, c, 0,1),
			video::S3DVertex(spf.X+sd,(f32)BS*cn[1],spf.Z,   0,0,0, c, 1,1),
			video::S3DVertex(spf.X+sd,(f32)BS*cn[2],spf.Z+sd,0,0,0, c, 1,0),
			video::S3DVertex(spf.X,   (f32)BS*cn[3],spf.Z+sd,0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		
		buf->append(vertices, 4, indices, 6);
	}
	
	// Set VBO on
	//mesh_new->setHardwareMappingHint(scene::EHM_STATIC);

	/*
		Replace the mesh
	*/

	mesh_mutex.Lock();

	scene::SMesh *mesh_old = mesh;

	//DEBUG
	/*mesh = NULL;
	mesh_new->drop();*/
	mesh = mesh_new;
	
	mesh_mutex.Unlock();

	if(mesh_old != NULL)
	{
		/*dstream<<"mesh_old refcount="<<mesh_old->getReferenceCount()
				<<std::endl;
		scene::IMeshBuffer *buf = mesh_new->getMeshBuffer
				(g_materials[MATERIAL_GRASS]);
		if(buf != NULL)
			dstream<<"grass buf refcount="<<buf->getReferenceCount()
					<<std::endl;*/

		mesh_old->drop();
	}
	else
	{
		dstream<<"WARNING: There was no old master heightmap mesh"<<std::endl;
	}
#endif
}

void ClientMap::PrintInfo(std::ostream &out)
{
	out<<"ClientMap: ";
}


/*
	MapVoxelManipulator
*/

MapVoxelManipulator::MapVoxelManipulator(Map *map)
{
	m_map = map;
}

MapVoxelManipulator::~MapVoxelManipulator()
{
	dstream<<"MapVoxelManipulator: blocks: "<<m_loaded_blocks.size()
			<<std::endl;
}

void MapVoxelManipulator::emerge(VoxelArea a)
{
	TimeTaker timer1("emerge", g_device, &emerge_time);

	// Units of these are MapBlocks
	v3s16 p_min = getNodeBlockPos(a.MinEdge);
	v3s16 p_max = getNodeBlockPos(a.MaxEdge);

	VoxelArea block_area_nodes
			(p_min*MAP_BLOCKSIZE, (p_max+1)*MAP_BLOCKSIZE-v3s16(1,1,1));

	addArea(block_area_nodes);

	for(s32 z=p_min.Z; z<=p_max.Z; z++)
	for(s32 y=p_min.Y; y<=p_max.Y; y++)
	for(s32 x=p_min.X; x<=p_max.X; x++)
	{
		v3s16 p(x,y,z);
		core::map<v3s16, bool>::Node *n;
		n = m_loaded_blocks.find(p);
		if(n != NULL)
			continue;
		
		bool block_data_inexistent = false;
		try
		{
			TimeTaker timer1("emerge load", g_device, &emerge_load_time);

			dstream<<"Loading block ("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<std::endl;
			
			MapBlock *block = m_map->getBlockNoCreate(p);
			if(block->isDummy())
				block_data_inexistent = true;
			else
				block->copyTo(*this);
		}
		catch(InvalidPositionException &e)
		{
			block_data_inexistent = true;
		}

		if(block_data_inexistent)
		{
			VoxelArea a(p*MAP_BLOCKSIZE, (p+1)*MAP_BLOCKSIZE-v3s16(1,1,1));
			// Fill with VOXELFLAG_INEXISTENT
			for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
			for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
			{
				s32 i = m_area.index(a.MinEdge.X,y,z);
				memset(&m_flags[i], VOXELFLAG_INEXISTENT, MAP_BLOCKSIZE);
			}
		}

		m_loaded_blocks.insert(p, true);
	}

	//dstream<<"emerge done"<<std::endl;
}

/*
	TODO: Add an option to only update eg. water and air nodes.
	      This will make it interfere less with important stuff if
		  run on background.
*/
void MapVoxelManipulator::blitBack
		(core::map<v3s16, MapBlock*> & modified_blocks)
{
	TimeTaker timer1("blitBack", g_device);
	
	/*
		Initialize block cache
	*/
	v3s16 blockpos_last;
	MapBlock *block = NULL;
	bool block_checked_in_modified = false;

	for(s32 z=m_area.MinEdge.Z; z<=m_area.MaxEdge.Z; z++)
	for(s32 y=m_area.MinEdge.Y; y<=m_area.MaxEdge.Y; y++)
	for(s32 x=m_area.MinEdge.X; x<=m_area.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);

		u8 f = m_flags[m_area.index(p)];
		if(f & (VOXELFLAG_NOT_LOADED|VOXELFLAG_INEXISTENT))
			continue;

		MapNode &n = m_data[m_area.index(p)];
			
		v3s16 blockpos = getNodeBlockPos(p);
		
		try
		{
			// Get block
			if(block == NULL || blockpos != blockpos_last){
				block = m_map->getBlockNoCreate(blockpos);
				blockpos_last = blockpos;
				block_checked_in_modified = false;
			}
			
			// Calculate relative position in block
			v3s16 relpos = p - blockpos * MAP_BLOCKSIZE;

			// Don't continue if nothing has changed here
			if(block->getNode(relpos) == n)
				continue;

			//m_map->setNode(m_area.MinEdge + p, n);
			block->setNode(relpos, n);
			
			/*
				Make sure block is in modified_blocks
			*/
			if(block_checked_in_modified == false)
			{
				modified_blocks[blockpos] = block;
				block_checked_in_modified = true;
			}
		}
		catch(InvalidPositionException &e)
		{
		}
	}
}

//END
