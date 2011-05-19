/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "main.h"
#include "jmutexautolock.h"
#include "client.h"
#include "filesys.h"
#include "utility.h"
#include "voxel.h"
#include "porting.h"
#include "mineral.h"
#include "noise.h"
#include "serverobject.h"

/*
	Map
*/

Map::Map(std::ostream &dout):
	m_dout(dout),
	m_sector_cache(NULL)
{
	/*m_sector_mutex.Init();
	assert(m_sector_mutex.IsInitialized());*/
}

Map::~Map()
{
	/*
		Free all MapSectors
	*/
	core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapSector *sector = i.getNode()->getValue();
		delete sector;
	}
}

void Map::addEventReceiver(MapEventReceiver *event_receiver)
{
	m_event_receivers.insert(event_receiver, false);
}

void Map::removeEventReceiver(MapEventReceiver *event_receiver)
{
	if(m_event_receivers.find(event_receiver) == NULL)
		return;
	m_event_receivers.remove(event_receiver);
}

void Map::dispatchEvent(MapEditEvent *event)
{
	for(core::map<MapEventReceiver*, bool>::Iterator
			i = m_event_receivers.getIterator();
			i.atEnd()==false; i++)
	{
		MapEventReceiver* event_receiver = i.getNode()->getKey();
		event_receiver->onMapEditEvent(event);
	}
}

MapSector * Map::getSectorNoGenerateNoExNoLock(v2s16 p)
{
	if(m_sector_cache != NULL && p == m_sector_cache_p){
		MapSector * sector = m_sector_cache;
		// Reset inactivity timer
		sector->usage_timer = 0.0;
		return sector;
	}
	
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
	
	if(n == NULL)
		return NULL;
	
	MapSector *sector = n->getValue();
	
	// Cache the last result
	m_sector_cache_p = p;
	m_sector_cache = sector;

	// Reset inactivity timer
	sector->usage_timer = 0.0;
	return sector;
}

MapSector * Map::getSectorNoGenerateNoEx(v2s16 p)
{
	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out

	return getSectorNoGenerateNoExNoLock(p);
}

MapSector * Map::getSectorNoGenerate(v2s16 p)
{
	MapSector *sector = getSectorNoGenerateNoEx(p);
	if(sector == NULL)
		throw InvalidPositionException();
	
	return sector;
}

MapBlock * Map::getBlockNoCreate(v3s16 p3d)
{	
	v2s16 p2d(p3d.X, p3d.Z);
	MapSector * sector = getSectorNoGenerate(p2d);

	MapBlock *block = sector->getBlockNoCreate(p3d.Y);

	return block;
}

MapBlock * Map::getBlockNoCreateNoEx(v3s16 p3d)
{
	try
	{
		v2s16 p2d(p3d.X, p3d.Z);
		MapSector * sector = getSectorNoGenerate(p2d);
		MapBlock *block = sector->getBlockNoCreate(p3d.Y);
		return block;
	}
	catch(InvalidPositionException &e)
	{
		return NULL;
	}
}

/*MapBlock * Map::getBlockCreate(v3s16 p3d)
{
	v2s16 p2d(p3d.X, p3d.Z);
	MapSector * sector = getSectorCreate(p2d);
	assert(sector);
	MapBlock *block = sector->getBlockNoCreate(p3d.Y);
	if(block)
		return block;
	block = sector->createBlankBlock(p3d.Y);
	return block;
}*/

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
void Map::unspreadLight(enum LightBank bank,
		core::map<v3s16, u8> & from_nodes,
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
				if(n2.getLight(bank) < oldlight)
				{
					/*
						And the neighbor is transparent and it has some light
					*/
					if(n2.light_propagates() && n2.getLight(bank) != 0)
					{
						/*
							Set light to 0 and add to queue
						*/

						u8 current_light = n2.getLight(bank);
						n2.setLight(bank, 0);
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
					
					/*// DEBUG
					if(light_sources.find(n2pos) != NULL)
						light_sources.remove(n2pos);*/
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
		unspreadLight(bank, unlighted_nodes, light_sources, modified_blocks);
}

/*
	A single-node wrapper of the above
*/
void Map::unLightNeighbors(enum LightBank bank,
		v3s16 pos, u8 lightwas,
		core::map<v3s16, bool> & light_sources,
		core::map<v3s16, MapBlock*>  & modified_blocks)
{
	core::map<v3s16, u8> from_nodes;
	from_nodes.insert(pos, lightwas);

	unspreadLight(bank, from_nodes, light_sources, modified_blocks);
}

/*
	Lights neighbors of from_nodes, collects all them and then
	goes on recursively.
*/
void Map::spreadLight(enum LightBank bank,
		core::map<v3s16, bool> & from_nodes,
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

		u8 oldlight = n.getLight(bank);
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
				if(n2.getLight(bank) > undiminish_light(oldlight))
				{
					lighted_nodes.insert(n2pos, true);
					//lighted_nodes.push_back(n2pos);
					changed = true;
				}
				/*
					If the neighbor is dimmer than how much light this node
					would spread on it, add to list
				*/
				if(n2.getLight(bank) < newlight)
				{
					if(n2.light_propagates())
					{
						n2.setLight(bank, newlight);
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
		spreadLight(bank, lighted_nodes, modified_blocks);
}

/*
	A single-node source variation of the above.
*/
void Map::lightNeighbors(enum LightBank bank,
		v3s16 pos,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	core::map<v3s16, bool> from_nodes;
	from_nodes.insert(pos, true);
	spreadLight(bank, from_nodes, modified_blocks);
}

v3s16 Map::getBrightestNeighbour(enum LightBank bank, v3s16 p)
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
		if(n2.getLight(bank) > brightest_light || found_something == false){
			brightest_light = n2.getLight(bank);
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

	Mud is turned into grass in where the sunlight stops.
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
			n.setLight(LIGHTBANK_DAY, LIGHT_SUN);
			block->setNode(relpos, n);

			modified_blocks.insert(blockpos, block);
		}
		else
		{
			// Turn mud into grass
			if(n.d == CONTENT_MUD)
			{
				n.d = CONTENT_GRASS;
				block->setNode(relpos, n);
				modified_blocks.insert(blockpos, block);
			}

			// Sunlight goes no further
			break;
		}
	}
	return y + 1;
}

void Map::updateLighting(enum LightBank bank,
		core::map<v3s16, MapBlock*> & a_blocks,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	/*m_dout<<DTIME<<"Map::updateLighting(): "
			<<a_blocks.size()<<" blocks."<<std::endl;*/
	
	//TimeTaker timer("updateLighting");
	
	// For debugging
	//bool debug=true;
	//u32 count_was = modified_blocks.size();
	
	core::map<v3s16, MapBlock*> blocks_to_update;

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

			blocks_to_update.insert(pos, block);

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
					u8 oldlight = n.getLight(bank);
					n.setLight(bank, 0);
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
			
			if(bank == LIGHTBANK_DAY)
			{
				bool bottom_valid = block->propagateSunlight(light_sources);

				// If bottom is valid, we're done.
				if(bottom_valid)
					break;
			}
			else if(bank == LIGHTBANK_NIGHT)
			{
				// For night lighting, sunlight is not propagated
				break;
			}
			else
			{
				// Invalid lighting bank
				assert(0);
			}
				
			/*dstream<<"Bottom for sunlight-propagated block ("
					<<pos.X<<","<<pos.Y<<","<<pos.Z<<") not valid"
					<<std::endl;*/

			// Bottom sunlight is not valid; get the block and loop to it

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

#if 0
	{
		TimeTaker timer("unspreadLight");
		unspreadLight(bank, unlight_from, light_sources, modified_blocks);
	}
	
	if(debug)
	{
		u32 diff = modified_blocks.size() - count_was;
		count_was = modified_blocks.size();
		dstream<<"unspreadLight modified "<<diff<<std::endl;
	}

	{
		TimeTaker timer("spreadLight");
		spreadLight(bank, light_sources, modified_blocks);
	}
	
	if(debug)
	{
		u32 diff = modified_blocks.size() - count_was;
		count_was = modified_blocks.size();
		dstream<<"spreadLight modified "<<diff<<std::endl;
	}
#endif
	
	{
		//MapVoxelManipulator vmanip(this);
		
		// Make a manual voxel manipulator and load all the blocks
		// that touch the requested blocks
		ManualMapVoxelManipulator vmanip(this);
		core::map<v3s16, MapBlock*>::Iterator i;
		i = blocks_to_update.getIterator();
		for(; i.atEnd() == false; i++)
		{
			MapBlock *block = i.getNode()->getValue();
			v3s16 p = block->getPos();
			
			// Add all surrounding blocks
			vmanip.initialEmerge(p - v3s16(1,1,1), p + v3s16(1,1,1));

			/*
				Add all surrounding blocks that have up-to-date lighting
				NOTE: This doesn't quite do the job (not everything
				      appropriate is lighted)
			*/
			/*for(s16 z=-1; z<=1; z++)
			for(s16 y=-1; y<=1; y++)
			for(s16 x=-1; x<=1; x++)
			{
				v3s16 p(x,y,z);
				MapBlock *block = getBlockNoCreateNoEx(p);
				if(block == NULL)
					continue;
				if(block->isDummy())
					continue;
				if(block->getLightingExpired())
					continue;
				vmanip.initialEmerge(p, p);
			}*/
			
			// Lighting of block will be updated completely
			block->setLightingExpired(false);
		}

		{
			//TimeTaker timer("unSpreadLight");
			vmanip.unspreadLight(bank, unlight_from, light_sources);
		}
		{
			//TimeTaker timer("spreadLight");
			vmanip.spreadLight(bank, light_sources);
		}
		{
			//TimeTaker timer("blitBack");
			vmanip.blitBack(modified_blocks);
		}
		/*dstream<<"emerge_time="<<emerge_time<<std::endl;
		emerge_time = 0;*/
	}

	//m_dout<<"Done ("<<getTimestamp()<<")"<<std::endl;
}

void Map::updateLighting(core::map<v3s16, MapBlock*> & a_blocks,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	updateLighting(LIGHTBANK_DAY, a_blocks, modified_blocks);
	updateLighting(LIGHTBANK_NIGHT, a_blocks, modified_blocks);
	
	/*
		Update information about whether day and night light differ
	*/
	for(core::map<v3s16, MapBlock*>::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();
		block->updateDayNightDiff();
	}
}

/*
	This is called after changing a node from transparent to opaque.
	The lighting value of the node should be left as-is after changing
	other values. This sets the lighting value to 0.
*/
void Map::addNodeAndUpdate(v3s16 p, MapNode n,
		core::map<v3s16, MapBlock*> &modified_blocks)
{
	/*PrintInfo(m_dout);
	m_dout<<DTIME<<"Map::addNodeAndUpdate(): p=("
			<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
	
	/*
		From this node to nodes underneath:
		If lighting is sunlight (1.0), unlight neighbours and
		set lighting to 0.
		Else discontinue.
	*/

	v3s16 toppos = p + v3s16(0,1,0);
	v3s16 bottompos = p + v3s16(0,-1,0);

	bool node_under_sunlight = true;
	core::map<v3s16, bool> light_sources;

	/*
		If there is a node at top and it doesn't have sunlight,
		there has not been any sunlight going down.

		Otherwise there probably is.
	*/
	try{
		MapNode topnode = getNode(toppos);

		if(topnode.getLight(LIGHTBANK_DAY) != LIGHT_SUN)
			node_under_sunlight = false;
	}
	catch(InvalidPositionException &e)
	{
	}
	
	/*
		If the new node doesn't propagate sunlight and there is
		grass below, change it to mud
	*/
	if(content_features(n.d).sunlight_propagates == false)
	{
		try{
			MapNode bottomnode = getNode(bottompos);
			
			if(bottomnode.d == CONTENT_GRASS
					|| bottomnode.d == CONTENT_GRASS_FOOTSTEPS)
			{
				bottomnode.d = CONTENT_MUD;
				setNode(bottompos, bottomnode);
			}
		}
		catch(InvalidPositionException &e)
		{
		}
	}

	/*
		If the new node is mud and it is under sunlight, change it
		to grass
	*/
	if(n.d == CONTENT_MUD && node_under_sunlight)
	{
		n.d = CONTENT_GRASS;
	}

	/*
		Remove all light that has come out of this node
	*/

	enum LightBank banks[] =
	{
		LIGHTBANK_DAY,
		LIGHTBANK_NIGHT
	};
	for(s32 i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];

		u8 lightwas = getNode(p).getLight(bank);

		// Add the block of the added node to modified_blocks
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * block = getBlockNoCreate(blockpos);
		assert(block != NULL);
		modified_blocks.insert(blockpos, block);
		
		assert(isValidPosition(p));
			
		// Unlight neighbours of node.
		// This means setting light of all consequent dimmer nodes
		// to 0.
		// This also collects the nodes at the border which will spread
		// light again into this.
		unLightNeighbors(bank, p, lightwas, light_sources, modified_blocks);

		n.setLight(bank, 0);
	}

	/*
		If node lets sunlight through and is under sunlight, it has
		sunlight too.
	*/
	if(node_under_sunlight && content_features(n.d).sunlight_propagates)
	{
		n.setLight(LIGHTBANK_DAY, LIGHT_SUN);
	}
	
	/*
		Set the node on the map
	*/
	
	setNode(p, n);

	/*
		Add intial metadata
	*/

	NodeMetadata *meta_proto = content_features(n.d).initial_metadata;
	if(meta_proto)
	{
		NodeMetadata *meta = meta_proto->clone();
		setNodeMetadata(p, meta);
	}
	
	/*
		If node is under sunlight and doesn't let sunlight through,
		take all sunlighted nodes under it and clear light from them
		and from where the light has been spread.
		TODO: This could be optimized by mass-unlighting instead
		      of looping
	*/
	if(node_under_sunlight && !content_features(n.d).sunlight_propagates)
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

			if(n2.getLight(LIGHTBANK_DAY) == LIGHT_SUN)
			{
				unLightNeighbors(LIGHTBANK_DAY,
						n2pos, n2.getLight(LIGHTBANK_DAY),
						light_sources, modified_blocks);
				n2.setLight(LIGHTBANK_DAY, 0);
				setNode(n2pos, n2);
			}
			else
				break;
		}
	}

	for(s32 i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];
		
		/*
			Spread light from all nodes that might be capable of doing so
		*/
		spreadLight(bank, light_sources, modified_blocks);
	}

	/*
		Update information about whether day and night light differ
	*/
	for(core::map<v3s16, MapBlock*>::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();
		block->updateDayNightDiff();
	}

	/*
		Add neighboring liquid nodes and the node itself if it is
		liquid (=water node was added) to transform queue.
	*/
	v3s16 dirs[7] = {
		v3s16(0,0,0), // self
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	for(u16 i=0; i<7; i++)
	{
		try
		{

		v3s16 p2 = p + dirs[i];
		
		MapNode n2 = getNode(p2);
		if(content_liquid(n2.d))
		{
			m_transforming_liquid.push_back(p2);
		}
		
		}catch(InvalidPositionException &e)
		{
		}
	}
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
	u8 replace_material = CONTENT_AIR;
	
	/*
		If there is a node at top and it doesn't have sunlight,
		there will be no sunlight going down.
	*/
	try{
		MapNode topnode = getNode(toppos);

		if(topnode.getLight(LIGHTBANK_DAY) != LIGHT_SUN)
			node_under_sunlight = false;
	}
	catch(InvalidPositionException &e)
	{
	}

	core::map<v3s16, bool> light_sources;

	enum LightBank banks[] =
	{
		LIGHTBANK_DAY,
		LIGHTBANK_NIGHT
	};
	for(s32 i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];
	
		/*
			Unlight neighbors (in case the node is a light source)
		*/
		unLightNeighbors(bank, p,
				getNode(p).getLight(bank),
				light_sources, modified_blocks);
	}

	/*
		Remove node metadata
	*/

	removeNodeMetadata(p);

	/*
		Remove the node.
		This also clears the lighting.
	*/

	MapNode n;
	n.d = replace_material;
	setNode(p, n);
	
	for(s32 i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];
	
		/*
			Recalculate lighting
		*/
		spreadLight(bank, light_sources, modified_blocks);
	}

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
			lightNeighbors(LIGHTBANK_DAY, p2, modified_blocks);
		}
	}
	else
	{
		// Set the lighting of this node to 0
		// TODO: Is this needed? Lighting is cleared up there already.
		try{
			MapNode n = getNode(p);
			n.setLight(LIGHTBANK_DAY, 0);
			setNode(p, n);
		}
		catch(InvalidPositionException &e)
		{
			assert(0);
		}
	}

	for(s32 i=0; i<2; i++)
	{
		enum LightBank bank = banks[i];
	
		// Get the brightest neighbour node and propagate light from it
		v3s16 n2p = getBrightestNeighbour(bank, p);
		try{
			MapNode n2 = getNode(n2p);
			lightNeighbors(bank, n2p, modified_blocks);
		}
		catch(InvalidPositionException &e)
		{
		}
	}

	/*
		Update information about whether day and night light differ
	*/
	for(core::map<v3s16, MapBlock*>::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();
		block->updateDayNightDiff();
	}

	/*
		Add neighboring liquid nodes to transform queue.
	*/
	v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	for(u16 i=0; i<6; i++)
	{
		try
		{

		v3s16 p2 = p + dirs[i];
		
		MapNode n2 = getNode(p2);
		if(content_liquid(n2.d))
		{
			m_transforming_liquid.push_back(p2);
		}
		
		}catch(InvalidPositionException &e)
		{
		}
	}
}

bool Map::addNodeWithEvent(v3s16 p, MapNode n)
{
	MapEditEvent event;
	event.type = MEET_ADDNODE;
	event.p = p;
	event.n = n;

	bool succeeded = true;
	try{
		core::map<v3s16, MapBlock*> modified_blocks;
		addNodeAndUpdate(p, n, modified_blocks);

		// Copy modified_blocks to event
		for(core::map<v3s16, MapBlock*>::Iterator
				i = modified_blocks.getIterator();
				i.atEnd()==false; i++)
		{
			event.modified_blocks.insert(i.getNode()->getKey(), false);
		}
	}
	catch(InvalidPositionException &e){
		succeeded = false;
	}

	dispatchEvent(&event);

	return succeeded;
}

bool Map::removeNodeWithEvent(v3s16 p)
{
	MapEditEvent event;
	event.type = MEET_REMOVENODE;
	event.p = p;

	bool succeeded = true;
	try{
		core::map<v3s16, MapBlock*> modified_blocks;
		removeNodeAndUpdate(p, modified_blocks);

		// Copy modified_blocks to event
		for(core::map<v3s16, MapBlock*>::Iterator
				i = modified_blocks.getIterator();
				i.atEnd()==false; i++)
		{
			event.modified_blocks.insert(i.getNode()->getKey(), false);
		}
	}
	catch(InvalidPositionException &e){
		succeeded = false;
	}

	dispatchEvent(&event);

	return succeeded;
}

bool Map::dayNightDiffed(v3s16 blockpos)
{
	try{
		v3s16 p = blockpos + v3s16(0,0,0);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}
	// Leading edges
	try{
		v3s16 p = blockpos + v3s16(-1,0,0);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,-1,0);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,0,-1);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}
	// Trailing edges
	try{
		v3s16 p = blockpos + v3s16(1,0,0);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,1,0);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,0,1);
		MapBlock *b = getBlockNoCreate(p);
		if(b->dayNightDiffed())
			return true;
	}
	catch(InvalidPositionException &e){}

	return false;
}

/*
	Updates usage timers
*/
void Map::timerUpdate(float dtime)
{
	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out

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
	//SharedPtr<JMutexAutoLock> cachelock(m_blockcachelock.waitCaches());

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
	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out

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

#define WATER_DROP_BOOST 4

void Map::transformLiquids(core::map<v3s16, MapBlock*> & modified_blocks)
{
	DSTACK(__FUNCTION_NAME);
	//TimeTaker timer("transformLiquids()");

	u32 loopcount = 0;
	u32 initial_size = m_transforming_liquid.size();
	
	/*if(initial_size != 0)
		dstream<<"transformLiquids(): initial_size="<<initial_size<<std::endl;*/

	while(m_transforming_liquid.size() != 0)
	{
		/*
			Get a queued transforming liquid node
		*/
		v3s16 p0 = m_transforming_liquid.pop_front();

		MapNode n0 = getNode(p0);
		
		// Don't deal with non-liquids
		if(content_liquid(n0.d) == false)
			continue;

		bool is_source = !content_flowing_liquid(n0.d);
		
		u8 liquid_level = 8;
		if(is_source == false)
			liquid_level = n0.param2 & 0x0f;
		
		// Turn possible source into non-source
		u8 nonsource_c = make_liquid_flowing(n0.d);

		/*
			If not source, check that some node flows into this one
			and what is the level of liquid in this one
		*/
		if(is_source == false)
		{
			s8 new_liquid_level_max = -1;

			v3s16 dirs_from[5] = {
				v3s16(0,1,0), // top
				v3s16(0,0,1), // back
				v3s16(1,0,0), // right
				v3s16(0,0,-1), // front
				v3s16(-1,0,0), // left
			};
			for(u16 i=0; i<5; i++)
			{
				try
				{

				bool from_top = (i==0);

				v3s16 p2 = p0 + dirs_from[i];
				MapNode n2 = getNode(p2);

				if(content_liquid(n2.d))
				{
					u8 n2_nonsource_c = make_liquid_flowing(n2.d);
					// Check that the liquids are the same type
					if(n2_nonsource_c != nonsource_c)
					{
						dstream<<"WARNING: Not handling: different liquids"
								" collide"<<std::endl;
						continue;
					}
					bool n2_is_source = !content_flowing_liquid(n2.d);
					s8 n2_liquid_level = 8;
					if(n2_is_source == false)
						n2_liquid_level = n2.param2 & 0x07;
					
					s8 new_liquid_level = -1;
					if(from_top)
					{
						//new_liquid_level = 7;
						if(n2_liquid_level >= 7 - WATER_DROP_BOOST)
							new_liquid_level = 7;
						else
							new_liquid_level = n2_liquid_level + WATER_DROP_BOOST;
					}
					else if(n2_liquid_level > 0)
					{
						new_liquid_level = n2_liquid_level - 1;
					}

					if(new_liquid_level > new_liquid_level_max)
						new_liquid_level_max = new_liquid_level;
				}

				}catch(InvalidPositionException &e)
				{
				}
			} //for
			
			/*
				If liquid level should be something else, update it and
				add all the neighboring water nodes to the transform queue.
			*/
			if(new_liquid_level_max != liquid_level)
			{
				if(new_liquid_level_max == -1)
				{
					// Remove water alltoghether
					n0.d = CONTENT_AIR;
					n0.param2 = 0;
					setNode(p0, n0);
				}
				else
				{
					n0.param2 = new_liquid_level_max;
					setNode(p0, n0);
				}
				
				// Block has been modified
				{
					v3s16 blockpos = getNodeBlockPos(p0);
					MapBlock *block = getBlockNoCreateNoEx(blockpos);
					if(block != NULL)
						modified_blocks.insert(blockpos, block);
				}
				
				/*
					Add neighboring non-source liquid nodes to transform queue.
				*/
				v3s16 dirs[6] = {
					v3s16(0,0,1), // back
					v3s16(0,1,0), // top
					v3s16(1,0,0), // right
					v3s16(0,0,-1), // front
					v3s16(0,-1,0), // bottom
					v3s16(-1,0,0), // left
				};
				for(u16 i=0; i<6; i++)
				{
					try
					{

					v3s16 p2 = p0 + dirs[i];
					
					MapNode n2 = getNode(p2);
					if(content_flowing_liquid(n2.d))
					{
						m_transforming_liquid.push_back(p2);
					}
					
					}catch(InvalidPositionException &e)
					{
					}
				}
			}
		}
		
		// Get a new one from queue if the node has turned into non-water
		if(content_liquid(n0.d) == false)
			continue;

		/*
			Flow water from this node
		*/
		v3s16 dirs_to[5] = {
			v3s16(0,-1,0), // bottom
			v3s16(0,0,1), // back
			v3s16(1,0,0), // right
			v3s16(0,0,-1), // front
			v3s16(-1,0,0), // left
		};
		for(u16 i=0; i<5; i++)
		{
			try
			{

			bool to_bottom = (i == 0);

			// If liquid is at lowest possible height, it's not going
			// anywhere except down
			if(liquid_level == 0 && to_bottom == false)
				continue;
			
			u8 liquid_next_level = 0;
			// If going to bottom
			if(to_bottom)
			{
				//liquid_next_level = 7;
				if(liquid_level >= 7 - WATER_DROP_BOOST)
					liquid_next_level = 7;
				else
					liquid_next_level = liquid_level + WATER_DROP_BOOST;
			}
			else
				liquid_next_level = liquid_level - 1;

			bool n2_changed = false;
			bool flowed = false;
			
			v3s16 p2 = p0 + dirs_to[i];

			MapNode n2 = getNode(p2);
			//dstream<<"[1] n2.param="<<(int)n2.param<<std::endl;

			if(content_liquid(n2.d))
			{
				u8 n2_nonsource_c = make_liquid_flowing(n2.d);
				// Check that the liquids are the same type
				if(n2_nonsource_c != nonsource_c)
				{
					dstream<<"WARNING: Not handling: different liquids"
							" collide"<<std::endl;
					continue;
				}
				bool n2_is_source = !content_flowing_liquid(n2.d);
				u8 n2_liquid_level = 8;
				if(n2_is_source == false)
					n2_liquid_level = n2.param2 & 0x07;
				
				if(to_bottom)
				{
					flowed = true;
				}

				if(n2_is_source)
				{
					// Just flow into the source, nothing changes.
					// n2_changed is not set because destination didn't change
					flowed = true;
				}
				else
				{
					if(liquid_next_level > liquid_level)
					{
						n2.param2 = liquid_next_level;
						setNode(p2, n2);

						n2_changed = true;
						flowed = true;
					}
				}
			}
			else if(n2.d == CONTENT_AIR)
			{
				n2.d = nonsource_c;
				n2.param2 = liquid_next_level;
				setNode(p2, n2);
				
				n2_changed = true;
				flowed = true;
			}
			
			//dstream<<"[2] n2.param="<<(int)n2.param<<std::endl;

			if(n2_changed)
			{
				m_transforming_liquid.push_back(p2);
				
				v3s16 blockpos = getNodeBlockPos(p2);
				MapBlock *block = getBlockNoCreateNoEx(blockpos);
				if(block != NULL)
					modified_blocks.insert(blockpos, block);
			}
			
			// If n2_changed to bottom, don't flow anywhere else
			if(to_bottom && flowed && !is_source)
				break;
				
			}catch(InvalidPositionException &e)
			{
			}
		}

		loopcount++;
		//if(loopcount >= 100000)
		if(loopcount >= initial_size * 1)
			break;
	}
	//dstream<<"Map::transformLiquids(): loopcount="<<loopcount<<std::endl;
}

NodeMetadata* Map::getNodeMetadata(v3s16 p)
{
	v3s16 blockpos = getNodeBlockPos(p);
	v3s16 p_rel = p - blockpos*MAP_BLOCKSIZE;
	MapBlock *block = getBlockNoCreateNoEx(blockpos);
	if(block == NULL)
	{
		dstream<<"WARNING: Map::setNodeMetadata(): Block not found"
				<<std::endl;
		return NULL;
	}
	NodeMetadata *meta = block->m_node_metadata.get(p_rel);
	return meta;
}

void Map::setNodeMetadata(v3s16 p, NodeMetadata *meta)
{
	v3s16 blockpos = getNodeBlockPos(p);
	v3s16 p_rel = p - blockpos*MAP_BLOCKSIZE;
	MapBlock *block = getBlockNoCreateNoEx(blockpos);
	if(block == NULL)
	{
		dstream<<"WARNING: Map::setNodeMetadata(): Block not found"
				<<std::endl;
		return;
	}
	block->m_node_metadata.set(p_rel, meta);
}

void Map::removeNodeMetadata(v3s16 p)
{
	v3s16 blockpos = getNodeBlockPos(p);
	v3s16 p_rel = p - blockpos*MAP_BLOCKSIZE;
	MapBlock *block = getBlockNoCreateNoEx(blockpos);
	if(block == NULL)
	{
		dstream<<"WARNING: Map::removeNodeMetadata(): Block not found"
				<<std::endl;
		return;
	}
	block->m_node_metadata.remove(p_rel);
}

void Map::nodeMetadataStep(float dtime,
		core::map<v3s16, MapBlock*> &changed_blocks)
{
	/*
		NOTE:
		Currently there is no way to ensure that all the necessary
		blocks are loaded when this is run. (They might get unloaded)
		NOTE: ^- Actually, that might not be so. In a quick test it
		reloaded a block with a furnace when I walked back to it from
		a distance.
	*/
	core::map<v2s16, MapSector*>::Iterator si;
	si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();
		core::list< MapBlock * > sectorblocks;
		sector->getBlocks(sectorblocks);
		core::list< MapBlock * >::Iterator i;
		for(i=sectorblocks.begin(); i!=sectorblocks.end(); i++)
		{
			MapBlock *block = *i;
			bool changed = block->m_node_metadata.step(dtime);
			if(changed)
				changed_blocks[block->getPos()] = block;
		}
	}
}

/*
	ServerMap
*/

ServerMap::ServerMap(std::string savedir):
	Map(dout_server),
	m_seed(0),
	m_map_metadata_changed(true)
{
	dstream<<__FUNCTION_NAME<<std::endl;
	
	//m_chunksize = 64;
	//m_chunksize = 16; // Too slow
	m_chunksize = 8; // Takes a few seconds
	//m_chunksize = 4;
	//m_chunksize = 2;
	
	m_seed = (((u64)(myrand()%0xffff)<<0)
			+ ((u64)(myrand()%0xffff)<<16)
			+ ((u64)(myrand()%0xffff)<<32)
			+ ((u64)(myrand()%0xffff)<<48));

	/*
		Experimental and debug stuff
	*/
	
	{
	}
	
	/*
		Try to load map; if not found, create a new one.
	*/

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
				try{
					// Load map metadata (seed, chunksize)
					loadMapMeta();

					// Load chunk metadata
					loadChunkMeta();
				}
				catch(FileNotGoodException &e){
					dstream<<DTIME<<"WARNING: Server: Could not load "
							<<"metafile(s). Disabling chunk-based "
							<<"generation."<<std::endl;
					m_chunksize = 0;
				}
			
				/*// Load sector (0,0) and throw and exception on fail
				if(loadSectorFull(v2s16(0,0)) == false)
					throw LoadError("Failed to load sector (0,0)");*/

				/*dstream<<DTIME<<"Server: Successfully loaded chunk "
						"metadata and sector (0,0) from "<<savedir<<
						", assuming valid save directory."
						<<std::endl;*/

				dstream<<DTIME<<"INFO: Server: Successfully loaded map "
						<<"and chunk metadata from "<<savedir
						<<", assuming valid save directory."
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
		dstream<<DTIME<<"WARNING: Server: Failed to load map from "<<savedir
				<<", exception: "<<e.what()<<std::endl;
		dstream<<"Please remove the map or fix it."<<std::endl;
		dstream<<"WARNING: Map saving will be disabled."<<std::endl;
	}

	dstream<<DTIME<<"INFO: Initializing new map."<<std::endl;
	
	// Create zero sector
	emergeSector(v2s16(0,0));

	// Initially write whole map
	save(false);
}

ServerMap::~ServerMap()
{
	dstream<<__FUNCTION_NAME<<std::endl;
	
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
	
	/*
		Free all MapChunks
	*/
	core::map<v2s16, MapChunk*>::Iterator i = m_chunks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapChunk *chunk = i.getNode()->getValue();
		delete chunk;
	}
}

/*
	Some helper functions for the map generator
*/

s16 find_ground_level(VoxelManipulator &vmanip, v2s16 p2d)
{
	v3s16 em = vmanip.m_area.getExtent();
	s16 y_nodes_max = vmanip.m_area.MaxEdge.Y;
	s16 y_nodes_min = vmanip.m_area.MinEdge.Y;
	u32 i = vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
	s16 y;
	for(y=y_nodes_max; y>=y_nodes_min; y--)
	{
		MapNode &n = vmanip.m_data[i];
		if(content_walkable(n.d))
			break;
			
		vmanip.m_area.add_y(em, i, -1);
	}
	if(y >= y_nodes_min)
		return y;
	else
		return y_nodes_min;
}

s16 find_ground_level_clever(VoxelManipulator &vmanip, v2s16 p2d)
{
	v3s16 em = vmanip.m_area.getExtent();
	s16 y_nodes_max = vmanip.m_area.MaxEdge.Y;
	s16 y_nodes_min = vmanip.m_area.MinEdge.Y;
	u32 i = vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
	s16 y;
	for(y=y_nodes_max; y>=y_nodes_min; y--)
	{
		MapNode &n = vmanip.m_data[i];
		if(content_walkable(n.d)
				&& n.d != CONTENT_TREE
				&& n.d != CONTENT_LEAVES)
			break;
			
		vmanip.m_area.add_y(em, i, -1);
	}
	if(y >= y_nodes_min)
		return y;
	else
		return y_nodes_min;
}

void make_tree(VoxelManipulator &vmanip, v3s16 p0)
{
	MapNode treenode(CONTENT_TREE);
	MapNode leavesnode(CONTENT_LEAVES);

	s16 trunk_h = myrand_range(3, 6);
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
		p1.Y++;
	}
	
	// p1 is now the last piece of the trunk
	p1.Y -= 1;

	VoxelArea leaves_a(v3s16(-2,-2,-2), v3s16(2,2,2));
	//SharedPtr<u8> leaves_d(new u8[leaves_a.getVolume()]);
	Buffer<u8> leaves_d(leaves_a.getVolume());
	for(s32 i=0; i<leaves_a.getVolume(); i++)
		leaves_d[i] = 0;
	
	// Force leaves at near the end of the trunk
	{
		s16 d = 1;
		for(s16 z=-d; z<=d; z++)
		for(s16 y=-d; y<=d; y++)
		for(s16 x=-d; x<=d; x++)
		{
			leaves_d[leaves_a.index(v3s16(x,y,z))] = 1;
		}
	}
	
	// Add leaves randomly
	for(u32 iii=0; iii<7; iii++)
	{
		s16 d = 1;

		v3s16 p(
			myrand_range(leaves_a.MinEdge.X, leaves_a.MaxEdge.X-d),
			myrand_range(leaves_a.MinEdge.Y, leaves_a.MaxEdge.Y-d),
			myrand_range(leaves_a.MinEdge.Z, leaves_a.MaxEdge.Z-d)
		);
		
		for(s16 z=0; z<=d; z++)
		for(s16 y=0; y<=d; y++)
		for(s16 x=0; x<=d; x++)
		{
			leaves_d[leaves_a.index(p+v3s16(x,y,z))] = 1;
		}
	}
	
	// Blit leaves to vmanip
	for(s16 z=leaves_a.MinEdge.Z; z<=leaves_a.MaxEdge.Z; z++)
	for(s16 y=leaves_a.MinEdge.Y; y<=leaves_a.MaxEdge.Y; y++)
	for(s16 x=leaves_a.MinEdge.X; x<=leaves_a.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		p += p1;
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		if(vmanip.m_data[vi].d != CONTENT_AIR)
			continue;
		u32 i = leaves_a.index(x,y,z);
		if(leaves_d[i] == 1)
			vmanip.m_data[vi] = leavesnode;
	}
}

/*
	Helpers for noise
*/

/*// -1->0, 0->1, 1->0
double contour(double v)
{
	v = fabs(v);
	if(v >= 1.0)
		return 0.0;
	return (1.0-v);
}*/

/*
	Noise functions. Make sure seed is mangled differently in each one.
*/

// This affects the shape of the contour
#define CAVE_NOISE_SCALE 10.0

NoiseParams get_cave_noise1_params(u64 seed)
{
	return NoiseParams(NOISE_PERLIN_CONTOUR, seed+52534, 5, 0.7,
			200, CAVE_NOISE_SCALE);
}

NoiseParams get_cave_noise2_params(u64 seed)
{
	return NoiseParams(NOISE_PERLIN_CONTOUR_FLIP_YZ, seed+10325, 5, 0.7,
			200, CAVE_NOISE_SCALE);
}

#define CAVE_NOISE_THRESHOLD (2.5/CAVE_NOISE_SCALE)

bool is_cave(u64 seed, v3s16 p)
{
	double d1 = noise3d_param(get_cave_noise1_params(seed), p.X,p.Y,p.Z);
	double d2 = noise3d_param(get_cave_noise2_params(seed), p.X,p.Y,p.Z);
	return d1*d2 > CAVE_NOISE_THRESHOLD;
}

// Amount of trees per area in nodes
double tree_amount_2d(u64 seed, v2s16 p)
{
	double noise = noise2d_perlin(
			0.5+(float)p.X/250, 0.5+(float)p.Y/250,
			seed+2, 5, 0.66);
	double zeroval = -0.3;
	if(noise < zeroval)
		return 0;
	else
		return 0.04 * (noise-zeroval) / (1.0-zeroval);
}

#define AVERAGE_MUD_AMOUNT 4

double base_rock_level_2d(u64 seed, v2s16 p)
{
	// The base ground level
	double base = (double)WATER_LEVEL - (double)AVERAGE_MUD_AMOUNT
			+ 20. * noise2d_perlin(
			0.5+(float)p.X/500., 0.5+(float)p.Y/500.,
			(seed>>32)+654879876, 6, 0.6);
	
	/*// A bit hillier one
	double base2 = WATER_LEVEL - 4.0 + 40. * noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			(seed>>27)+90340, 6, 0.69);
	if(base2 > base)
		base = base2;*/
#if 1
	// Higher ground level
	double higher = (double)WATER_LEVEL + 25. + 35. * noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			seed+85039, 5, 0.69);
	//higher = 30; // For debugging

	// Limit higher to at least base
	if(higher < base)
		higher = base;
		
	// Steepness factor of cliffs
	double b = 1.0 + 1.0 * noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			seed-932, 7, 0.7);
	b = rangelim(b, 0.0, 1000.0);
	b = pow(b, 5);
	b *= 7;
	b = rangelim(b, 3.0, 1000.0);
	//dstream<<"b="<<b<<std::endl;
	//double b = 20;

	// Offset to more low
	double a_off = -0.2;
	// High/low selector
	/*double a = 0.5 + b * (a_off + noise2d_perlin(
			0.5+(float)p.X/500., 0.5+(float)p.Y/500.,
			seed-359, 6, 0.7));*/
	double a = (double)0.5 + b * (a_off + noise2d_perlin(
			0.5+(float)p.X/250., 0.5+(float)p.Y/250.,
			seed-359, 5, 0.60));
	// Limit
	a = rangelim(a, 0.0, 1.0);

	//dstream<<"a="<<a<<std::endl;
	
	double h = base*(1.0-a) + higher*a;
#else
	double h = base;
#endif
	return h;
}

double get_mud_add_amount(u64 seed, v2s16 p)
{
	return ((float)AVERAGE_MUD_AMOUNT + 3.0 * noise2d_perlin(
			0.5+(float)p.X/200, 0.5+(float)p.Y/200,
			seed+91013, 3, 0.55));
}

/*
	Adds random objects to block, depending on the content of the block
*/
void addRandomObjects(MapBlock *block)
{
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		bool last_node_walkable = false;
		for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
		{
			v3s16 p(x0,y0,z0);
			MapNode n = block->getNodeNoEx(p);
			if(n.d == CONTENT_IGNORE)
				continue;
			if(content_features(n.d).liquid_type != LIQUID_NONE)
				continue;
			if(content_features(n.d).walkable)
			{
				last_node_walkable = true;
				continue;
			}
			if(last_node_walkable)
			{
				// If block contains light information
				if(content_features(n.d).param_type == CPT_LIGHT)
				{
					if(n.getLight(LIGHTBANK_DAY) <= 3)
					{
						if(myrand() % 300 == 0)
						{
							v3f pos_f = intToFloat(p+block->getPosRelative(), BS);
							pos_f.Y -= BS*0.4;
							ServerActiveObject *obj = new RatSAO(NULL, 0, pos_f);
							std::string data = obj->getStaticData();
							StaticObject s_obj(obj->getType(),
									obj->getBasePosition(), data);
							// Add some
							block->m_static_objects.insert(0, s_obj);
							block->m_static_objects.insert(0, s_obj);
							block->m_static_objects.insert(0, s_obj);
							block->m_static_objects.insert(0, s_obj);
							block->m_static_objects.insert(0, s_obj);
							block->m_static_objects.insert(0, s_obj);
							delete obj;
						}
						if(myrand() % 300 == 0)
						{
							v3f pos_f = intToFloat(p+block->getPosRelative(), BS);
							pos_f.Y -= BS*0.4;
							ServerActiveObject *obj = new Oerkki1SAO(NULL,0,pos_f);
							std::string data = obj->getStaticData();
							StaticObject s_obj(obj->getType(),
									obj->getBasePosition(), data);
							// Add one
							block->m_static_objects.insert(0, s_obj);
							delete obj;
						}
					}
				}
			}
			last_node_walkable = false;
		}
	}
	block->setChangedFlag();
}

/*
	This is the main map generation method
*/

#define VMANIP_FLAG_DUNGEON VOXELFLAG_CHECKED1

void makeChunk(ChunkMakeData *data)
{
	if(data->no_op)
		return;

	s16 y_nodes_min = data->y_blocks_min * MAP_BLOCKSIZE;
	s16 y_nodes_max = data->y_blocks_max * MAP_BLOCKSIZE + MAP_BLOCKSIZE - 1;
	s16 h_blocks = data->y_blocks_max - data->y_blocks_min + 1;
	u32 relative_volume = (u32)data->sectorpos_base_size*MAP_BLOCKSIZE
			*(u32)data->sectorpos_base_size*MAP_BLOCKSIZE
			*(u32)h_blocks*MAP_BLOCKSIZE;
	v3s16 bigarea_blocks_min(
		data->sectorpos_bigbase.X,
		data->y_blocks_min,
		data->sectorpos_bigbase.Y
	);
	v3s16 bigarea_blocks_max(
		data->sectorpos_bigbase.X + data->sectorpos_bigbase_size - 1,
		data->y_blocks_max,
		data->sectorpos_bigbase.Y + data->sectorpos_bigbase_size - 1
	);
	s16 lighting_min_d = 0-data->max_spread_amount;
	s16 lighting_max_d = data->sectorpos_base_size*MAP_BLOCKSIZE
			+ data->max_spread_amount-1;

	// Clear all flags
	data->vmanip.clearFlag(0xff);

	TimeTaker timer_generate("makeChunk() generate");

	// Maximum height of the stone surface and obstacles.
	// This is used to disable cave generation from going too high.
	s16 stone_surface_max_y = 0;

	/*
		Generate general ground level to full area
	*/
	{
	// 22ms @cs=8
	TimeTaker timer1("Generating ground level");

	//NoiseBuffer noisebuf1;
	//NoiseBuffer noisebuf2;
	NoiseBuffer noisebuf_cave;
	{
		v3f minpos_f(
			data->sectorpos_bigbase.X*MAP_BLOCKSIZE,
			y_nodes_min,
			data->sectorpos_bigbase.Y*MAP_BLOCKSIZE
		);
		v3f maxpos_f = minpos_f + v3f(
			data->sectorpos_bigbase_size*MAP_BLOCKSIZE,
			y_nodes_max-y_nodes_min,
			data->sectorpos_bigbase_size*MAP_BLOCKSIZE
		);
		//v3f samplelength_f = v3f(4.0, 4.0, 4.0);

		TimeTaker timer("noisebuf.create");

		noisebuf_cave.create(get_cave_noise1_params(data->seed),
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y, maxpos_f.Z,
				4, 4, 4);
		
		noisebuf_cave.multiply(get_cave_noise2_params(data->seed));
		
		/*noisebuf1.create(data->seed+25104, 6, 0.60, 200.0, false,
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y, maxpos_f.Z,
				samplelength_f.X, samplelength_f.Y, samplelength_f.Z);*/
		/*noisebuf1.create(data->seed+25104, 3, 0.60, 25.0, false,
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y, maxpos_f.Z,
				samplelength_f.X, samplelength_f.Y, samplelength_f.Z);
		noisebuf2.create(data->seed+25105, 4, 0.50, 200.0, false,
				minpos_f.X, minpos_f.Y, minpos_f.Z,
				maxpos_f.X, maxpos_f.Y, maxpos_f.Z,
				samplelength_f.X, samplelength_f.Y, samplelength_f.Z);*/
	}

#if 0
	for(s16 x=0; x<data->sectorpos_bigbase_size*MAP_BLOCKSIZE; x++)
	for(s16 z=0; z<data->sectorpos_bigbase_size*MAP_BLOCKSIZE; z++)
	{
		// Node position
		v2s16 p2d = data->sectorpos_bigbase*MAP_BLOCKSIZE + v2s16(x,z);
		
		// Ground height at this point
		float surface_y_f = 0.0;

		// Use perlin noise for ground height
		surface_y_f = base_rock_level_2d(data->seed, p2d);
		//surface_y_f = base_rock_level_2d(data->seed, p2d);
		
		// Convert to integer
		s16 surface_y = (s16)surface_y_f;
		
		// Log it
		if(surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;

		/*
			Fill ground with stone
		*/
		{
			// Use fast index incrementing
			v3s16 em = data->vmanip.m_area.getExtent();
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_nodes_min, p2d.Y));
			for(s16 y=y_nodes_min; y<=y_nodes_max; y++)
			{
				// Skip if already generated.
				// This is done here because there might be a cave at
				// any point in ground, which could look like it
				// wasn't generated.
				if(data->vmanip.m_data[i].d != CONTENT_AIR)
					break;

				/*s16 noiseval = 50.0 * noise3d_perlin(
						0.5+(float)p2d.X/100.0,
						0.5+(float)y/100.0,
						0.5+(float)p2d.Y/100.0,
						data->seed+123, 5, 0.5);*/
				double noiseval = 64.0 * noisebuf1.get(p2d.X, y, p2d.Y);
				/*double noiseval = 30.0 * noisebuf1.get(p2d.X, y, p2d.Y);
				noiseval *= MYMAX(0, -0.2 + noisebuf2.get(p2d.X, y, p2d.Y));*/
				
				//if(y < surface_y + noiseval)
				if(noiseval > 0)
				//if(noiseval > y)
					data->vmanip.m_data[i].d = CONTENT_STONE;

				data->vmanip.m_area.add_y(em, i, 1);
			}
		}
	}
#endif
	
#if 1
	for(s16 x=0; x<data->sectorpos_bigbase_size*MAP_BLOCKSIZE; x++)
	for(s16 z=0; z<data->sectorpos_bigbase_size*MAP_BLOCKSIZE; z++)
	{
		// Node position
		v2s16 p2d = data->sectorpos_bigbase*MAP_BLOCKSIZE + v2s16(x,z);
		
		/*
			Skip of already generated
		*/
		/*{
			v3s16 p(p2d.X, y_nodes_min, p2d.Y);
			if(data->vmanip.m_data[data->vmanip.m_area.index(p)].d != CONTENT_AIR)
				continue;
		}*/

#define CAVE_TEST 0

#if CAVE_TEST
		/*
			Fill ground with stone
		*/
		{
			// Use fast index incrementing
			v3s16 em = data->vmanip.m_area.getExtent();
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_nodes_min, p2d.Y));
			for(s16 y=y_nodes_min; y<=y_nodes_max; y++)
			{
				// Skip if already generated.
				// This is done here because there might be a cave at
				// any point in ground, which could look like it
				// wasn't generated.
				if(data->vmanip.m_data[i].d != CONTENT_AIR)
					break;

				//if(is_cave(data->seed, v3s16(p2d.X, y, p2d.Y)))
				if(noisebuf_cave.get(p2d.X,y,p2d.Y) > CAVE_NOISE_THRESHOLD)
				{
					data->vmanip.m_data[i].d = CONTENT_STONE;
				}
				else
				{
					data->vmanip.m_data[i].d = CONTENT_AIR;
				}

				data->vmanip.m_area.add_y(em, i, 1);
			}
		}
#else
		// Ground height at this point
		float surface_y_f = 0.0;
		
		// Use perlin noise for ground height
		surface_y_f = base_rock_level_2d(data->seed, p2d);
		
		// Convert to integer
		s16 surface_y = (s16)surface_y_f;
		
		// Log it
		if(surface_y > stone_surface_max_y)
			stone_surface_max_y = surface_y;

		/*
			Fill ground with stone
		*/
		{
			// Use fast index incrementing
			v3s16 em = data->vmanip.m_area.getExtent();
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_nodes_min, p2d.Y));
			for(s16 y=y_nodes_min; y<surface_y && y<=y_nodes_max; y++)
			{
				// Skip if already generated.
				// This is done here because there might be a cave at
				// any point in ground, which could look like it
				// wasn't generated.
				if(data->vmanip.m_data[i].d != CONTENT_AIR)
					break;

				//if(is_cave(data->seed, v3s16(p2d.X, y, p2d.Y)))
				if(noisebuf_cave.get(p2d.X,y,p2d.Y) > CAVE_NOISE_THRESHOLD)
				{
					// Set tunnel flag
					data->vmanip.m_flags[i] |= VMANIP_FLAG_DUNGEON;
					// Is now air
					data->vmanip.m_data[i].d = CONTENT_AIR;
				}
				else
				{
					data->vmanip.m_data[i].d = CONTENT_STONE;
				}

				data->vmanip.m_area.add_y(em, i, 1);
			}
		}
#endif // !CAVE_TEST
	}
#endif
	
	}//timer1

	/*
		Randomize some parameters
	*/
	
	//s32 stone_obstacle_count = 0;
	/*s32 stone_obstacle_count =
			rangelim((1.0+noise2d(data->seed+897,
			data->sectorpos_base.X, data->sectorpos_base.Y))/2.0 * 30, 0, 100000);*/
	
	//s16 stone_obstacle_max_height = 0;
	/*s16 stone_obstacle_max_height =
			rangelim((1.0+noise2d(data->seed+5902,
			data->sectorpos_base.X, data->sectorpos_base.Y))/2.0 * 30, 0, 100000);*/
	
	// Set to 0 to disable everything but lighting
#if 1

	/*
		Loop this part, it will make stuff look older and newer nicely
	*/
	const u32 age_loops = 2;
	for(u32 i_age=0; i_age<age_loops; i_age++)
	{ // Aging loop
	/******************************
		BEGINNING OF AGING LOOP
	******************************/

#if 1
	{
	// 24ms @cs=8
	//TimeTaker timer1("caves");

	/*
		Make caves
	*/
	//u32 caves_count = relative_volume / 400000;
	u32 caves_count = 0;
	u32 bruises_count = relative_volume * stone_surface_max_y / 40000000;
	if(stone_surface_max_y < WATER_LEVEL)
		bruises_count = 0;
	/*u32 caves_count = 0;
	u32 bruises_count = 0;*/
	for(u32 jj=0; jj<caves_count+bruises_count; jj++)
	{
		s16 min_tunnel_diameter = 3;
		s16 max_tunnel_diameter = 5;
		u16 tunnel_routepoints = 20;
		
		v3f main_direction(0,0,0);

		bool bruise_surface = (jj > caves_count);

		if(bruise_surface)
		{
			min_tunnel_diameter = 5;
			max_tunnel_diameter = myrand_range(10, 20);
			/*min_tunnel_diameter = MYMAX(0, stone_surface_max_y/6);
			max_tunnel_diameter = myrand_range(MYMAX(0, stone_surface_max_y/6), MYMAX(0, stone_surface_max_y/2));*/
			
			/*s16 tunnel_rou = rangelim(25*(0.5+1.0*noise2d(data->seed+42,
					data->sectorpos_base.X, data->sectorpos_base.Y)), 0, 15);*/

			tunnel_routepoints = 5;
		}
		else
		{
		}

		// Allowed route area size in nodes
		v3s16 ar(
			data->sectorpos_base_size*MAP_BLOCKSIZE,
			h_blocks*MAP_BLOCKSIZE,
			data->sectorpos_base_size*MAP_BLOCKSIZE
		);

		// Area starting point in nodes
		v3s16 of(
			data->sectorpos_base.X*MAP_BLOCKSIZE,
			data->y_blocks_min*MAP_BLOCKSIZE,
			data->sectorpos_base.Y*MAP_BLOCKSIZE
		);

		// Allow a bit more
		//(this should be more than the maximum radius of the tunnel)
		//s16 insure = 5; // Didn't work with max_d = 20
		s16 insure = 10;
		s16 more = data->max_spread_amount - max_tunnel_diameter/2 - insure;
		ar += v3s16(1,0,1) * more * 2;
		of -= v3s16(1,0,1) * more;
		
		s16 route_y_min = 0;
		// Allow half a diameter + 7 over stone surface
		s16 route_y_max = -of.Y + stone_surface_max_y + max_tunnel_diameter/2 + 7;

		/*// If caves, don't go through surface too often
		if(bruise_surface == false)
			route_y_max -= myrand_range(0, max_tunnel_diameter*2);*/

		// Limit maximum to area
		route_y_max = rangelim(route_y_max, 0, ar.Y-1);

		if(bruise_surface)
		{
			/*// Minimum is at y=0
			route_y_min = -of.Y - 0;*/
			// Minimum is at y=max_tunnel_diameter/4
			//route_y_min = -of.Y + max_tunnel_diameter/4;
			//s16 min = -of.Y + max_tunnel_diameter/4;
			s16 min = -of.Y + 0;
			route_y_min = myrand_range(min, min + max_tunnel_diameter);
			route_y_min = rangelim(route_y_min, 0, route_y_max);
		}

		/*dstream<<"route_y_min = "<<route_y_min
				<<", route_y_max = "<<route_y_max<<std::endl;*/

		s16 route_start_y_min = route_y_min;
		s16 route_start_y_max = route_y_max;

		// Start every 2nd cave from surface
		bool coming_from_surface = (jj % 2 == 0 && bruise_surface == false);

		if(coming_from_surface)
		{
			route_start_y_min = -of.Y + stone_surface_max_y + 10;
		}
		
		route_start_y_min = rangelim(route_start_y_min, 0, ar.Y-1);
		route_start_y_max = rangelim(route_start_y_max, route_start_y_min, ar.Y-1);

		// Randomize starting position
		v3f orp(
			(float)(myrand()%ar.X)+0.5,
			(float)(myrand_range(route_start_y_min, route_start_y_max))+0.5,
			(float)(myrand()%ar.Z)+0.5
		);

		MapNode airnode(CONTENT_AIR);
		
		/*
			Generate some tunnel starting from orp
		*/
		
		for(u16 j=0; j<tunnel_routepoints; j++)
		{
			if(j%7==0 && bruise_surface == false)
			{
				main_direction = v3f(
					((float)(myrand()%20)-(float)10)/10,
					((float)(myrand()%20)-(float)10)/30,
					((float)(myrand()%20)-(float)10)/10
				);
				main_direction *= (float)myrand_range(1, 3);
			}

			// Randomize size
			s16 min_d = min_tunnel_diameter;
			s16 max_d = max_tunnel_diameter;
			s16 rs = myrand_range(min_d, max_d);
			
			v3s16 maxlen;
			if(bruise_surface)
			{
				maxlen = v3s16(rs*7,rs*7,rs*7);
			}
			else
			{
				maxlen = v3s16(rs*4, myrand_range(1, rs*3), rs*4);
			}

			v3f vec;
			
			if(coming_from_surface && j < 3)
			{
				vec = v3f(
					(float)(myrand()%(maxlen.X*2))-(float)maxlen.X,
					(float)(myrand()%(maxlen.Y*1))-(float)maxlen.Y,
					(float)(myrand()%(maxlen.Z*2))-(float)maxlen.Z
				);
			}
			else
			{
				vec = v3f(
					(float)(myrand()%(maxlen.X*2))-(float)maxlen.X,
					(float)(myrand()%(maxlen.Y*2))-(float)maxlen.Y,
					(float)(myrand()%(maxlen.Z*2))-(float)maxlen.Z
				);
			}
			
			vec += main_direction;

			v3f rp = orp + vec;
			if(rp.X < 0)
				rp.X = 0;
			else if(rp.X >= ar.X)
				rp.X = ar.X-1;
			if(rp.Y < route_y_min)
				rp.Y = route_y_min;
			else if(rp.Y >= route_y_max)
				rp.Y = route_y_max-1;
			if(rp.Z < 0)
				rp.Z = 0;
			else if(rp.Z >= ar.Z)
				rp.Z = ar.Z-1;
			vec = rp - orp;

			for(float f=0; f<1.0; f+=1.0/vec.getLength())
			{
				v3f fp = orp + vec * f;
				v3s16 cp(fp.X, fp.Y, fp.Z);

				s16 d0 = -rs/2;
				s16 d1 = d0 + rs - 1;
				for(s16 z0=d0; z0<=d1; z0++)
				{
					//s16 si = rs - MYMAX(0, abs(z0)-rs/4);
					s16 si = rs - MYMAX(0, abs(z0)-rs/7);
					for(s16 x0=-si; x0<=si-1; x0++)
					{
						s16 maxabsxz = MYMAX(abs(x0), abs(z0));
						//s16 si2 = rs - MYMAX(0, maxabsxz-rs/4);
						s16 si2 = rs - MYMAX(0, maxabsxz-rs/7);
						//s16 si2 = rs - abs(x0);
						for(s16 y0=-si2+1+2; y0<=si2-1; y0++)
						{
							s16 z = cp.Z + z0;
							s16 y = cp.Y + y0;
							s16 x = cp.X + x0;
							v3s16 p(x,y,z);
							/*if(isInArea(p, ar) == false)
								continue;*/
							// Check only height
							if(y < 0 || y >= ar.Y)
								continue;
							p += of;
							
							//assert(data->vmanip.m_area.contains(p));
							if(data->vmanip.m_area.contains(p) == false)
							{
								dstream<<"WARNING: "<<__FUNCTION_NAME
										<<":"<<__LINE__<<": "
										<<"point not in area"
										<<std::endl;
								continue;
							}
							
							// Just set it to air, it will be changed to
							// water afterwards
							u32 i = data->vmanip.m_area.index(p);
							data->vmanip.m_data[i] = airnode;

							if(bruise_surface == false)
							{
								// Set tunnel flag
								data->vmanip.m_flags[i] |= VMANIP_FLAG_DUNGEON;
							}
						}
					}
				}
			}

			orp = rp;
		}
	
	}

	}//timer1
#endif

#if 1
	{
	// 46ms @cs=8
	//TimeTaker timer1("ore veins");

	/*
		Make ore veins
	*/
	for(u32 jj=0; jj<relative_volume/1000; jj++)
	{
		s16 max_vein_diameter = 3;

		// Allowed route area size in nodes
		v3s16 ar(
			data->sectorpos_base_size*MAP_BLOCKSIZE,
			h_blocks*MAP_BLOCKSIZE,
			data->sectorpos_base_size*MAP_BLOCKSIZE
		);

		// Area starting point in nodes
		v3s16 of(
			data->sectorpos_base.X*MAP_BLOCKSIZE,
			data->y_blocks_min*MAP_BLOCKSIZE,
			data->sectorpos_base.Y*MAP_BLOCKSIZE
		);

		// Allow a bit more
		//(this should be more than the maximum radius of the tunnel)
		s16 insure = 3;
		s16 more = data->max_spread_amount - max_vein_diameter/2 - insure;
		ar += v3s16(1,0,1) * more * 2;
		of -= v3s16(1,0,1) * more;
		
		// Randomize starting position
		v3f orp(
			(float)(myrand()%ar.X)+0.5,
			(float)(myrand()%ar.Y)+0.5,
			(float)(myrand()%ar.Z)+0.5
		);

		// Randomize mineral
		u8 mineral;
		if(myrand()%3 != 0)
			mineral = MINERAL_COAL;
		else
			mineral = MINERAL_IRON;

		/*
			Generate some vein starting from orp
		*/

		for(u16 j=0; j<2; j++)
		{
			/*v3f rp(
				(float)(myrand()%ar.X)+0.5,
				(float)(myrand()%ar.Y)+0.5,
				(float)(myrand()%ar.Z)+0.5
			);
			v3f vec = rp - orp;*/
			
			v3s16 maxlen(5, 5, 5);
			v3f vec(
				(float)(myrand()%(maxlen.X*2))-(float)maxlen.X,
				(float)(myrand()%(maxlen.Y*2))-(float)maxlen.Y,
				(float)(myrand()%(maxlen.Z*2))-(float)maxlen.Z
			);
			v3f rp = orp + vec;
			if(rp.X < 0)
				rp.X = 0;
			else if(rp.X >= ar.X)
				rp.X = ar.X;
			if(rp.Y < 0)
				rp.Y = 0;
			else if(rp.Y >= ar.Y)
				rp.Y = ar.Y;
			if(rp.Z < 0)
				rp.Z = 0;
			else if(rp.Z >= ar.Z)
				rp.Z = ar.Z;
			vec = rp - orp;

			// Randomize size
			s16 min_d = 0;
			s16 max_d = max_vein_diameter;
			s16 rs = myrand_range(min_d, max_d);
			
			for(float f=0; f<1.0; f+=1.0/vec.getLength())
			{
				v3f fp = orp + vec * f;
				v3s16 cp(fp.X, fp.Y, fp.Z);
				s16 d0 = -rs/2;
				s16 d1 = d0 + rs - 1;
				for(s16 z0=d0; z0<=d1; z0++)
				{
					s16 si = rs - abs(z0);
					for(s16 x0=-si; x0<=si-1; x0++)
					{
						s16 si2 = rs - abs(x0);
						for(s16 y0=-si2+1; y0<=si2-1; y0++)
						{
							// Don't put mineral to every place
							if(myrand()%5 != 0)
								continue;

							s16 z = cp.Z + z0;
							s16 y = cp.Y + y0;
							s16 x = cp.X + x0;
							v3s16 p(x,y,z);
							/*if(isInArea(p, ar) == false)
								continue;*/
							// Check only height
							if(y < 0 || y >= ar.Y)
								continue;
							p += of;
							
							assert(data->vmanip.m_area.contains(p));
							
							// Just set it to air, it will be changed to
							// water afterwards
							u32 i = data->vmanip.m_area.index(p);
							MapNode *n = &data->vmanip.m_data[i];
							if(n->d == CONTENT_STONE)
								n->param = mineral;
						}
					}
				}
			}

			orp = rp;
		}
	
	}

	}//timer1
#endif

#if 1
	{
	// 15ms @cs=8
	TimeTaker timer1("add mud");

	/*
		Add mud to the central chunk
	*/
	
	for(s16 x=0; x<data->sectorpos_base_size*MAP_BLOCKSIZE; x++)
	for(s16 z=0; z<data->sectorpos_base_size*MAP_BLOCKSIZE; z++)
	{
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		// Randomize mud amount
		s16 mud_add_amount = get_mud_add_amount(data->seed, p2d) / 2.0;

		// Find ground level
		s16 surface_y = find_ground_level_clever(data->vmanip, p2d);

		/*
			If topmost node is grass, change it to mud.
			It might be if it was flown to there from a neighboring
			chunk and then converted.
		*/
		{
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, surface_y, p2d.Y));
			MapNode *n = &data->vmanip.m_data[i];
			if(n->d == CONTENT_GRASS)
				*n = MapNode(CONTENT_MUD);
				//n->d = CONTENT_MUD;
		}

		/*
			Add mud on ground
		*/
		{
			s16 mudcount = 0;
			v3s16 em = data->vmanip.m_area.getExtent();
			s16 y_start = surface_y+1;
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			for(s16 y=y_start; y<=y_nodes_max; y++)
			{
				if(mudcount >= mud_add_amount)
					break;
					
				MapNode &n = data->vmanip.m_data[i];
				n = MapNode(CONTENT_MUD);
				//n.d = CONTENT_MUD;
				mudcount++;

				data->vmanip.m_area.add_y(em, i, 1);
			}
		}

	}

	}//timer1
#endif

#if 1
	{
	// 340ms @cs=8
	TimeTaker timer1("flow mud");

	/*
		Flow mud away from steep edges
	*/

	// Limit area by 1 because mud is flown into neighbors.
	s16 mudflow_minpos = 0-data->max_spread_amount+1;
	s16 mudflow_maxpos = data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount-2;

	// Iterate a few times
	for(s16 k=0; k<3; k++)
	{

	for(s16 x=mudflow_minpos;
			x<=mudflow_maxpos;
			x++)
	for(s16 z=mudflow_minpos;
			z<=mudflow_maxpos;
			z++)
	{
		// Invert coordinates every 2nd iteration
		if(k%2 == 0)
		{
			x = mudflow_maxpos - (x-mudflow_minpos);
			z = mudflow_maxpos - (z-mudflow_minpos);
		}

		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		v3s16 em = data->vmanip.m_area.getExtent();
		u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
		s16 y=y_nodes_max;

		for(;; y--)
		{
			MapNode *n = NULL;
			// Find mud
			for(; y>=y_nodes_min; y--)
			{
				n = &data->vmanip.m_data[i];
				//if(content_walkable(n->d))
				//	break;
				if(n->d == CONTENT_MUD || n->d == CONTENT_GRASS)
					break;
					
				data->vmanip.m_area.add_y(em, i, -1);
			}

			// Stop if out of area
			//if(data->vmanip.m_area.contains(i) == false)
			if(y < y_nodes_min)
				break;

			/*// If not mud, do nothing to it
			MapNode *n = &data->vmanip.m_data[i];
			if(n->d != CONTENT_MUD && n->d != CONTENT_GRASS)
				continue;*/

			/*
				Don't flow it if the stuff under it is not mud
			*/
			{
				u32 i2 = i;
				data->vmanip.m_area.add_y(em, i2, -1);
				// Cancel if out of area
				if(data->vmanip.m_area.contains(i2) == false)
					continue;
				MapNode *n2 = &data->vmanip.m_data[i2];
				if(n2->d != CONTENT_MUD && n2->d != CONTENT_GRASS)
					continue;
			}

			// Make it exactly mud
			n->d = CONTENT_MUD;
			
			/*s16 recurse_count = 0;
	mudflow_recurse:*/

			v3s16 dirs4[4] = {
				v3s16(0,0,1), // back
				v3s16(1,0,0), // right
				v3s16(0,0,-1), // front
				v3s16(-1,0,0), // left
			};

			// Theck that upper is air or doesn't exist.
			// Cancel dropping if upper keeps it in place
			u32 i3 = i;
			data->vmanip.m_area.add_y(em, i3, 1);
			if(data->vmanip.m_area.contains(i3) == true
					&& content_walkable(data->vmanip.m_data[i3].d) == true)
			{
				continue;
			}

			// Drop mud on side
			
			for(u32 di=0; di<4; di++)
			{
				v3s16 dirp = dirs4[di];
				u32 i2 = i;
				// Move to side
				data->vmanip.m_area.add_p(em, i2, dirp);
				// Fail if out of area
				if(data->vmanip.m_area.contains(i2) == false)
					continue;
				// Check that side is air
				MapNode *n2 = &data->vmanip.m_data[i2];
				if(content_walkable(n2->d))
					continue;
				// Check that under side is air
				data->vmanip.m_area.add_y(em, i2, -1);
				if(data->vmanip.m_area.contains(i2) == false)
					continue;
				n2 = &data->vmanip.m_data[i2];
				if(content_walkable(n2->d))
					continue;
				/*// Check that under that is air (need a drop of 2)
				data->vmanip.m_area.add_y(em, i2, -1);
				if(data->vmanip.m_area.contains(i2) == false)
					continue;
				n2 = &data->vmanip.m_data[i2];
				if(content_walkable(n2->d))
					continue;*/
				// Loop further down until not air
				do{
					data->vmanip.m_area.add_y(em, i2, -1);
					// Fail if out of area
					if(data->vmanip.m_area.contains(i2) == false)
						continue;
					n2 = &data->vmanip.m_data[i2];
				}while(content_walkable(n2->d) == false);
				// Loop one up so that we're in air
				data->vmanip.m_area.add_y(em, i2, 1);
				n2 = &data->vmanip.m_data[i2];

				// Move mud to new place
				*n2 = *n;
				// Set old place to be air
				*n = MapNode(CONTENT_AIR);

				// Done
				break;
			}
		}
	}
	
	}

	}//timer1
#endif

#if 1
	{
	// 50ms @cs=8
	TimeTaker timer1("add water");

	/*
		Add water to the central chunk (and a bit more)
	*/
	
	for(s16 x=0-data->max_spread_amount;
			x<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount;
			x++)
	for(s16 z=0-data->max_spread_amount;
			z<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount;
			z++)
	{
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		// Find ground level
		//s16 surface_y = find_ground_level(data->vmanip, p2d);

		/*
			If ground level is over water level, skip.
			NOTE: This leaves caves near water without water,
			which looks especially crappy when the nearby water
			won't start flowing either for some reason
		*/
		/*if(surface_y > WATER_LEVEL)
			continue;*/

		/*
			Add water on ground
		*/
		{
			v3s16 em = data->vmanip.m_area.getExtent();
			u8 light = LIGHT_MAX;
			// Start at global water surface level
			s16 y_start = WATER_LEVEL;
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			MapNode *n = &data->vmanip.m_data[i];

			for(s16 y=y_start; y>=y_nodes_min; y--)
			{
				n = &data->vmanip.m_data[i];
				
				// Stop when there is no water and no air
				if(n->d != CONTENT_AIR && n->d != CONTENT_WATERSOURCE
						&& n->d != CONTENT_WATER)
				{

					break;
				}
				
				// Make water only not in caves
				if(!(data->vmanip.m_flags[i]&VMANIP_FLAG_DUNGEON))
				{
					n->d = CONTENT_WATERSOURCE;
					//n->setLight(LIGHTBANK_DAY, light);

					// Add to transforming liquid queue (in case it'd
					// start flowing)
					v3s16 p = v3s16(p2d.X, y, p2d.Y);
					data->transforming_liquid.push_back(p);
				}
				
				// Next one
				data->vmanip.m_area.add_y(em, i, -1);
				if(light > 0)
					light--;
			}
		}

	}

	}//timer1
#endif
	
	} // Aging loop
	/***********************
		END OF AGING LOOP
	************************/

#if 1
	{
	//TimeTaker timer1("convert mud to sand");

	/*
		Convert mud to sand
	*/
	
	//s16 mud_add_amount = myrand_range(2, 4);
	//s16 mud_add_amount = 0;
	
	/*for(s16 x=0; x<data->sectorpos_base_size*MAP_BLOCKSIZE; x++)
	for(s16 z=0; z<data->sectorpos_base_size*MAP_BLOCKSIZE; z++)*/
	for(s16 x=0-data->max_spread_amount+1;
			x<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount-1;
			x++)
	for(s16 z=0-data->max_spread_amount+1;
			z<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount-1;
			z++)
	{
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		// Determine whether to have sand here
		double sandnoise = noise2d_perlin(
				0.5+(float)p2d.X/500, 0.5+(float)p2d.Y/500,
				data->seed+59420, 3, 0.50);

		bool have_sand = (sandnoise > -0.15);

		if(have_sand == false)
			continue;

		// Find ground level
		s16 surface_y = find_ground_level_clever(data->vmanip, p2d);
		
		if(surface_y > WATER_LEVEL + 2)
			continue;

		{
			v3s16 em = data->vmanip.m_area.getExtent();
			s16 y_start = surface_y;
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			u32 not_sand_counter = 0;
			for(s16 y=y_start; y>=y_nodes_min; y--)
			{
				MapNode *n = &data->vmanip.m_data[i];
				if(n->d == CONTENT_MUD || n->d == CONTENT_GRASS)
				{
					n->d = CONTENT_SAND;
				}
				else
				{
					not_sand_counter++;
					if(not_sand_counter > 3)
						break;
				}

				data->vmanip.m_area.add_y(em, i, -1);
			}
		}

	}

	}//timer1
#endif

#if 1
	{
	// 1ms @cs=8
	//TimeTaker timer1("generate trees");

	/*
		Generate some trees
	*/
	{
		// Divide area into parts
		s16 div = 8;
		s16 sidelen = data->sectorpos_base_size*MAP_BLOCKSIZE / div;
		double area = sidelen * sidelen;
		for(s16 x0=0; x0<div; x0++)
		for(s16 z0=0; z0<div; z0++)
		{
			// Center position of part of division
			v2s16 p2d_center(
				data->sectorpos_base.X*MAP_BLOCKSIZE + sidelen/2 + sidelen*x0,
				data->sectorpos_base.Y*MAP_BLOCKSIZE + sidelen/2 + sidelen*z0
			);
			// Minimum edge of part of division
			v2s16 p2d_min(
				data->sectorpos_base.X*MAP_BLOCKSIZE + sidelen*x0,
				data->sectorpos_base.Y*MAP_BLOCKSIZE + sidelen*z0
			);
			// Maximum edge of part of division
			v2s16 p2d_max(
				data->sectorpos_base.X*MAP_BLOCKSIZE + sidelen + sidelen*x0 - 1,
				data->sectorpos_base.Y*MAP_BLOCKSIZE + sidelen + sidelen*z0 - 1
			);
			// Amount of trees
			u32 tree_count = area * tree_amount_2d(data->seed, p2d_center);
			// Put trees in random places on part of division
			for(u32 i=0; i<tree_count; i++)
			{
				s16 x = myrand_range(p2d_min.X, p2d_max.X);
				s16 z = myrand_range(p2d_min.Y, p2d_max.Y);
				s16 y = find_ground_level(data->vmanip, v2s16(x,z));
				// Don't make a tree under water level
				if(y < WATER_LEVEL)
					continue;
				// Don't make a tree so high that it doesn't fit
				if(y > y_nodes_max - 6)
					continue;
				v3s16 p(x,y,z);
				/*
					Trees grow only on mud and grass
				*/
				{
					u32 i = data->vmanip.m_area.index(v3s16(p));
					MapNode *n = &data->vmanip.m_data[i];
					if(n->d != CONTENT_MUD && n->d != CONTENT_GRASS)
						continue;
				}
				p.Y++;
				// Make a tree
				make_tree(data->vmanip, p);
			}
		}
		/*u32 tree_max = relative_area / 60;
		//u32 count = myrand_range(0, tree_max);
		for(u32 i=0; i<count; i++)
		{
			s16 x = myrand_range(0, data->sectorpos_base_size*MAP_BLOCKSIZE-1);
			s16 z = myrand_range(0, data->sectorpos_base_size*MAP_BLOCKSIZE-1);
			x += data->sectorpos_base.X*MAP_BLOCKSIZE;
			z += data->sectorpos_base.Y*MAP_BLOCKSIZE;
			s16 y = find_ground_level(data->vmanip, v2s16(x,z));
			// Don't make a tree under water level
			if(y < WATER_LEVEL)
				continue;
			v3s16 p(x,y+1,z);
			// Make a tree
			make_tree(data->vmanip, p);
		}*/
	}

	}//timer1
#endif

#if 1
	{
	// 19ms @cs=8
	//TimeTaker timer1("grow grass");

	/*
		Grow grass
	*/

	/*for(s16 x=0-4; x<data->sectorpos_base_size*MAP_BLOCKSIZE+4; x++)
	for(s16 z=0-4; z<data->sectorpos_base_size*MAP_BLOCKSIZE+4; z++)*/
	for(s16 x=0-data->max_spread_amount;
			x<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount;
			x++)
	for(s16 z=0-data->max_spread_amount;
			z<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount;
			z++)
	{
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		/*
			Find the lowest surface to which enough light ends up
			to make grass grow.

			Basically just wait until not air and not leaves.
		*/
		s16 surface_y = 0;
		{
			v3s16 em = data->vmanip.m_area.getExtent();
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_nodes_max, p2d.Y));
			s16 y;
			// Go to ground level
			for(y=y_nodes_max; y>=y_nodes_min; y--)
			{
				MapNode &n = data->vmanip.m_data[i];
				if(n.d != CONTENT_AIR
						&& n.d != CONTENT_LEAVES)
					break;
				data->vmanip.m_area.add_y(em, i, -1);
			}
			if(y >= y_nodes_min)
				surface_y = y;
			else
				surface_y = y_nodes_min;
		}
		
		u32 i = data->vmanip.m_area.index(p2d.X, surface_y, p2d.Y);
		MapNode *n = &data->vmanip.m_data[i];
		if(n->d == CONTENT_MUD)
			n->d = CONTENT_GRASS;
	}

	}//timer1
#endif

#endif // Disable all but lighting

	/*
		Initial lighting (sunlight)
	*/

	core::map<v3s16, bool> light_sources;

	{
	// 750ms @cs=8, can't optimize more
	TimeTaker timer1("initial lighting");

	// NOTE: This is no used... umm... for some reason!
#if 0
	/*
		Go through the edges and add all nodes that have light to light_sources
	*/
	
	// Four edges
	for(s16 i=0; i<4; i++)
	// Edge length
	for(s16 j=lighting_min_d;
			j<=lighting_max_d;
			j++)
	{
		s16 x;
		s16 z;
		// +-X
		if(i == 0 || i == 1)
		{
			x = (i==0) ? lighting_min_d : lighting_max_d;
			if(i == 0)
				z = lighting_min_d;
			else
				z = lighting_max_d;
		}
		// +-Z
		else
		{
			z = (i==0) ? lighting_min_d : lighting_max_d;
			if(i == 0)
				x = lighting_min_d;
			else
				x = lighting_max_d;
		}
		
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);

		{
			v3s16 em = data->vmanip.m_area.getExtent();
			s16 y_start = y_nodes_max;
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			for(s16 y=y_start; y>=y_nodes_min; y--)
			{
				MapNode *n = &data->vmanip.m_data[i];
				if(n->getLight(LIGHTBANK_DAY) != 0)
				{
					light_sources.insert(v3s16(p2d.X, y, p2d.Y), true);
				}
				//NOTE: This is broken, at least the index has to
				// be incremented
			}
		}
	}
#endif

#if 1
	/*
		Go through the edges and apply sunlight to them, not caring
		about neighbors
	*/
	
	// Four edges
	for(s16 i=0; i<4; i++)
	// Edge length
	for(s16 j=lighting_min_d;
			j<=lighting_max_d;
			j++)
	{
		s16 x;
		s16 z;
		// +-X
		if(i == 0 || i == 1)
		{
			x = (i==0) ? lighting_min_d : lighting_max_d;
			if(i == 0)
				z = lighting_min_d;
			else
				z = lighting_max_d;
		}
		// +-Z
		else
		{
			z = (i==0) ? lighting_min_d : lighting_max_d;
			if(i == 0)
				x = lighting_min_d;
			else
				x = lighting_max_d;
		}
		
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		// Loop from top to down
		{
			u8 light = LIGHT_SUN;
			v3s16 em = data->vmanip.m_area.getExtent();
			s16 y_start = y_nodes_max;
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			for(s16 y=y_start; y>=y_nodes_min; y--)
			{
				MapNode *n = &data->vmanip.m_data[i];
				if(light_propagates_content(n->d) == false)
				{
					light = 0;
				}
				else if(light != LIGHT_SUN
					|| sunlight_propagates_content(n->d) == false)
				{
					if(light > 0)
						light--;
				}
				
				n->setLight(LIGHTBANK_DAY, light);
				n->setLight(LIGHTBANK_NIGHT, 0);
				
				if(light != 0)
				{
					// Insert light source
					light_sources.insert(v3s16(p2d.X, y, p2d.Y), true);
				}
				
				// Increment index by y
				data->vmanip.m_area.add_y(em, i, -1);
			}
		}
	}
#endif

	/*for(s16 x=0; x<data->sectorpos_base_size*MAP_BLOCKSIZE; x++)
	for(s16 z=0; z<data->sectorpos_base_size*MAP_BLOCKSIZE; z++)*/
	/*for(s16 x=0-data->max_spread_amount+1;
			x<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount-1;
			x++)
	for(s16 z=0-data->max_spread_amount+1;
			z<data->sectorpos_base_size*MAP_BLOCKSIZE+data->max_spread_amount-1;
			z++)*/
#if 1
	/*
		This has to be 1 smaller than the actual area, because
		neighboring nodes are checked.
	*/
	for(s16 x=lighting_min_d+1;
			x<=lighting_max_d-1;
			x++)
	for(s16 z=lighting_min_d+1;
			z<=lighting_max_d-1;
			z++)
	{
		// Node position in 2d
		v2s16 p2d = data->sectorpos_base*MAP_BLOCKSIZE + v2s16(x,z);
		
		/*
			Apply initial sunlight
		*/
		{
			u8 light = LIGHT_SUN;
			bool add_to_sources = false;
			v3s16 em = data->vmanip.m_area.getExtent();
			s16 y_start = y_nodes_max;
			u32 i = data->vmanip.m_area.index(v3s16(p2d.X, y_start, p2d.Y));
			for(s16 y=y_start; y>=y_nodes_min; y--)
			{
				MapNode *n = &data->vmanip.m_data[i];

				if(light_propagates_content(n->d) == false)
				{
					light = 0;
				}
				else if(light != LIGHT_SUN
					|| sunlight_propagates_content(n->d) == false)
				{
					if(light > 0)
						light--;
				}
				
				// This doesn't take much time
				if(add_to_sources == false)
				{
					/*
						Check sides. If side is not air or water, start
						adding to light_sources.
					*/
					v3s16 dirs4[4] = {
						v3s16(0,0,1), // back
						v3s16(1,0,0), // right
						v3s16(0,0,-1), // front
						v3s16(-1,0,0), // left
					};
					for(u32 di=0; di<4; di++)
					{
						v3s16 dirp = dirs4[di];
						u32 i2 = i;
						data->vmanip.m_area.add_p(em, i2, dirp);
						MapNode *n2 = &data->vmanip.m_data[i2];
						if(
							n2->d != CONTENT_AIR
							&& n2->d != CONTENT_WATERSOURCE
							&& n2->d != CONTENT_WATER
						){
							add_to_sources = true;
							break;
						}
					}
				}
				
				n->setLight(LIGHTBANK_DAY, light);
				n->setLight(LIGHTBANK_NIGHT, 0);
				
				// This doesn't take much time
				if(light != 0 && add_to_sources)
				{
					// Insert light source
					light_sources.insert(v3s16(p2d.X, y, p2d.Y), true);
				}
				
				// Increment index by y
				data->vmanip.m_area.add_y(em, i, -1);
			}
		}
	}
#endif

	}//timer1

	// Spread light around
	{
		TimeTaker timer("makeChunk() spreadLight");
		data->vmanip.spreadLight(LIGHTBANK_DAY, light_sources);
	}
	
	/*
		Generation ended
	*/

	timer_generate.stop();
}

//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################
//###################################################################

void ServerMap::initChunkMake(ChunkMakeData &data, v2s16 chunkpos)
{
	if(m_chunksize == 0)
	{
		data.no_op = true;
		return;
	}

	data.no_op = false;

	// The distance how far into the neighbors the generator is allowed to go.
	s16 max_spread_amount_sectors = 2;
	assert(max_spread_amount_sectors <= m_chunksize);
	s16 max_spread_amount = max_spread_amount_sectors * MAP_BLOCKSIZE;

	s16 y_blocks_min = -4;
	s16 y_blocks_max = 3;

	v2s16 sectorpos_base = chunk_to_sector(chunkpos);
	s16 sectorpos_base_size = m_chunksize;

	v2s16 sectorpos_bigbase =
			sectorpos_base - v2s16(1,1) * max_spread_amount_sectors;
	s16 sectorpos_bigbase_size =
			sectorpos_base_size + 2 * max_spread_amount_sectors;
			
	data.seed = m_seed;
	data.chunkpos = chunkpos;
	data.y_blocks_min = y_blocks_min;
	data.y_blocks_max = y_blocks_max;
	data.sectorpos_base = sectorpos_base;
	data.sectorpos_base_size = sectorpos_base_size;
	data.sectorpos_bigbase = sectorpos_bigbase;
	data.sectorpos_bigbase_size = sectorpos_bigbase_size;
	data.max_spread_amount = max_spread_amount;

	/*
		Create the whole area of this and the neighboring chunks
	*/
	{
		TimeTaker timer("initChunkMake() create area");
		
		for(s16 x=0; x<sectorpos_bigbase_size; x++)
		for(s16 z=0; z<sectorpos_bigbase_size; z++)
		{
			v2s16 sectorpos = sectorpos_bigbase + v2s16(x,z);
			ServerMapSector *sector = createSector(sectorpos);
			assert(sector);

			for(s16 y=y_blocks_min; y<=y_blocks_max; y++)
			{
				v3s16 blockpos(sectorpos.X, y, sectorpos.Y);
				MapBlock *block = createBlock(blockpos);

				// Lighting won't be calculated
				//block->setLightingExpired(true);
				// Lighting will be calculated
				block->setLightingExpired(false);

				/*
					Block gets sunlight if this is true.

					This should be set to true when the top side of a block
					is completely exposed to the sky.

					Actually this doesn't matter now because the
					initial lighting is done here.
				*/
				block->setIsUnderground(y != y_blocks_max);
			}
		}
	}
	
	/*
		Now we have a big empty area.

		Make a ManualMapVoxelManipulator that contains this and the
		neighboring chunks
	*/
	
	v3s16 bigarea_blocks_min(
		sectorpos_bigbase.X,
		y_blocks_min,
		sectorpos_bigbase.Y
	);
	v3s16 bigarea_blocks_max(
		sectorpos_bigbase.X + sectorpos_bigbase_size - 1,
		y_blocks_max,
		sectorpos_bigbase.Y + sectorpos_bigbase_size - 1
	);
	
	data.vmanip.setMap(this);
	// Add the area
	{
		TimeTaker timer("initChunkMake() initialEmerge");
		data.vmanip.initialEmerge(bigarea_blocks_min, bigarea_blocks_max);
	}
	
}

MapChunk* ServerMap::finishChunkMake(ChunkMakeData &data,
		core::map<v3s16, MapBlock*> &changed_blocks)
{
	if(data.no_op)
		return NULL;
	
	/*
		Blit generated stuff to map
	*/
	{
		// 70ms @cs=8
		//TimeTaker timer("generateChunkRaw() blitBackAll");
		data.vmanip.blitBackAll(&changed_blocks);
	}

	/*
		Update day/night difference cache of the MapBlocks
	*/
	{
		for(core::map<v3s16, MapBlock*>::Iterator i = changed_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			MapBlock *block = i.getNode()->getValue();
			block->updateDayNightDiff();
		}
	}

	/*
		Copy transforming liquid information
	*/
	while(data.transforming_liquid.size() > 0)
	{
		v3s16 p = data.transforming_liquid.pop_front();
		m_transforming_liquid.push_back(p);
	}

	/*
		Add random objects to blocks
	*/
	{
		for(s16 x=0; x<data.sectorpos_base_size; x++)
		for(s16 z=0; z<data.sectorpos_base_size; z++)
		{
			v2s16 sectorpos = data.sectorpos_base + v2s16(x,z);
			ServerMapSector *sector = createSector(sectorpos);
			assert(sector);

			for(s16 y=data.y_blocks_min; y<=data.y_blocks_max; y++)
			{
				v3s16 blockpos(sectorpos.X, y, sectorpos.Y);
				MapBlock *block = createBlock(blockpos);
				addRandomObjects(block);
			}
		}
	}

	/*
		Create chunk metadata
	*/

	for(s16 x=-1; x<=1; x++)
	for(s16 y=-1; y<=1; y++)
	{
		v2s16 chunkpos0 = data.chunkpos + v2s16(x,y);
		// Add chunk meta information
		MapChunk *chunk = getChunk(chunkpos0);
		if(chunk == NULL)
		{
			chunk = new MapChunk();
			m_chunks.insert(chunkpos0, chunk);
		}
		//chunk->setIsVolatile(true);
		if(chunk->getGenLevel() > GENERATED_PARTLY)
			chunk->setGenLevel(GENERATED_PARTLY);
	}

	/*
		Set central chunk non-volatile
	*/
	MapChunk *chunk = getChunk(data.chunkpos);
	assert(chunk);
	// Set non-volatile
	//chunk->setIsVolatile(false);
	chunk->setGenLevel(GENERATED_FULLY);
	
	/*
		Save changed parts of map
	*/
	save(true);
	
	return chunk;
}

#if 0
// NOTE: Deprecated
MapChunk* ServerMap::generateChunkRaw(v2s16 chunkpos,
		core::map<v3s16, MapBlock*> &changed_blocks,
		bool force)
{
	DSTACK(__FUNCTION_NAME);

	/*
		Don't generate if already fully generated
	*/
	if(force == false)
	{
		MapChunk *chunk = getChunk(chunkpos);
		if(chunk != NULL && chunk->getGenLevel() == GENERATED_FULLY)
		{
			dstream<<"generateChunkRaw(): Chunk "
					<<"("<<chunkpos.X<<","<<chunkpos.Y<<")"
					<<" already generated"<<std::endl;
			return chunk;
		}
	}

	dstream<<"generateChunkRaw(): Generating chunk "
			<<"("<<chunkpos.X<<","<<chunkpos.Y<<")"
			<<std::endl;
	
	TimeTaker timer("generateChunkRaw()");

	ChunkMakeData data;
	
	// Initialize generation
	initChunkMake(data, chunkpos);
	
	// Generate stuff
	makeChunk(&data);

	// Finalize generation
	MapChunk *chunk = finishChunkMake(data, changed_blocks);

	/*
		Return central chunk (which was requested)
	*/
	return chunk;
}

// NOTE: Deprecated
MapChunk* ServerMap::generateChunk(v2s16 chunkpos1,
		core::map<v3s16, MapBlock*> &changed_blocks)
{
	dstream<<"generateChunk(): Generating chunk "
			<<"("<<chunkpos1.X<<","<<chunkpos1.Y<<")"
			<<std::endl;
	
	/*for(s16 x=-1; x<=1; x++)
	for(s16 y=-1; y<=1; y++)*/
	for(s16 x=-0; x<=0; x++)
	for(s16 y=-0; y<=0; y++)
	{
		v2s16 chunkpos0 = chunkpos1 + v2s16(x,y);
		MapChunk *chunk = getChunk(chunkpos0);
		// Skip if already generated
		if(chunk != NULL && chunk->getGenLevel() == GENERATED_FULLY)
			continue;
		generateChunkRaw(chunkpos0, changed_blocks);
	}
	
	assert(chunkNonVolatile(chunkpos1));

	MapChunk *chunk = getChunk(chunkpos1);
	return chunk;
}
#endif

ServerMapSector * ServerMap::createSector(v2s16 p2d)
{
	DSTACK("%s: p2d=(%d,%d)",
			__FUNCTION_NAME,
			p2d.X, p2d.Y);
	
	/*
		Check if it exists already in memory
	*/
	ServerMapSector *sector = (ServerMapSector*)getSectorNoGenerateNoEx(p2d);
	if(sector != NULL)
		return sector;
	
	/*
		Try to load it from disk (with blocks)
	*/
	if(loadSectorFull(p2d) == true)
	{
		ServerMapSector *sector = (ServerMapSector*)getSectorNoGenerateNoEx(p2d);
		if(sector == NULL)
		{
			dstream<<"ServerMap::createSector(): loadSectorFull didn't make a sector"<<std::endl;
			throw InvalidPositionException("");
		}
		return sector;
	}

	/*
		Do not create over-limit
	*/
	if(p2d.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p2d.X > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p2d.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p2d.Y > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE)
		throw InvalidPositionException("createSector(): pos. over limit");

	/*
		Generate blank sector
	*/
	
	sector = new ServerMapSector(this, p2d);
	
	// Sector position on map in nodes
	v2s16 nodepos2d = p2d * MAP_BLOCKSIZE;

	/*
		Insert to container
	*/
	m_sectors.insert(p2d, sector);
	
	return sector;
}

#if 0
MapSector * ServerMap::emergeSector(v2s16 p2d,
		core::map<v3s16, MapBlock*> &changed_blocks)
{
	DSTACK("%s: p2d=(%d,%d)",
			__FUNCTION_NAME,
			p2d.X, p2d.Y);
	
	/*
		Check chunk status
	*/
	v2s16 chunkpos = sector_to_chunk(p2d);
	/*bool chunk_nonvolatile = false;
	MapChunk *chunk = getChunk(chunkpos);
	if(chunk && chunk->getIsVolatile() == false)
		chunk_nonvolatile = true;*/
	bool chunk_nonvolatile = chunkNonVolatile(chunkpos);

	/*
		If chunk is not fully generated, generate chunk
	*/
	if(chunk_nonvolatile == false)
	{
		// Generate chunk and neighbors
		generateChunk(chunkpos, changed_blocks);
	}
	
	/*
		Return sector if it exists now
	*/
	MapSector *sector = getSectorNoGenerateNoEx(p2d);
	if(sector != NULL)
		return sector;
	
	/*
		Try to load it from disk
	*/
	if(loadSectorFull(p2d) == true)
	{
		MapSector *sector = getSectorNoGenerateNoEx(p2d);
		if(sector == NULL)
		{
			dstream<<"ServerMap::emergeSector(): loadSectorFull didn't make a sector"<<std::endl;
			throw InvalidPositionException("");
		}
		return sector;
	}

	/*
		generateChunk should have generated the sector
	*/
	//assert(0);
	
	dstream<<"WARNING: ServerMap::emergeSector: Cannot find sector ("
			<<p2d.X<<","<<p2d.Y<<" and chunk is already generated. "
			<<std::endl;

#if 0
	dstream<<"WARNING: Creating an empty sector."<<std::endl;

	return createSector(p2d);
	
#endif
	
#if 1
	dstream<<"WARNING: Forcing regeneration of chunk."<<std::endl;

	// Generate chunk
	generateChunkRaw(chunkpos, changed_blocks, true);

	/*
		Return sector if it exists now
	*/
	sector = getSectorNoGenerateNoEx(p2d);
	if(sector != NULL)
		return sector;
	
	dstream<<"ERROR: Could not get sector from anywhere."<<std::endl;
	
	assert(0);
#endif
	
	/*
		Generate directly
	*/
	//return generateSector();
}
#endif

/*
	NOTE: This is not used for main map generation, only for blocks
	that are very high or low
*/
MapBlock * ServerMap::generateBlock(
		v3s16 p,
		MapBlock *original_dummy,
		ServerMapSector *sector,
		core::map<v3s16, MapBlock*> &changed_blocks,
		core::map<v3s16, MapBlock*> &lighting_invalidated_blocks
)
{
	DSTACK("%s: p=(%d,%d,%d)",
			__FUNCTION_NAME,
			p.X, p.Y, p.Z);

	// If chunks are disabled
	/*if(m_chunksize == 0)
	{
		dstream<<"ServerMap::generateBlock(): Chunks disabled -> "
				<<"not generating."<<std::endl;
		return NULL;
	}*/
	
	/*dstream<<"generateBlock(): "
			<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<std::endl;*/
	
	MapBlock *block = original_dummy;
			
	v2s16 p2d(p.X, p.Z);
	s16 block_y = p.Y;
	v2s16 p2d_nodes = p2d * MAP_BLOCKSIZE;
	
	/*
		Do not generate over-limit
	*/
	if(blockpos_over_limit(p))
	{
		dstream<<__FUNCTION_NAME<<": Block position over limit"<<std::endl;
		throw InvalidPositionException("generateBlock(): pos. over limit");
	}

	/*
		If block doesn't exist, create one.
		If it exists, it is a dummy. In that case unDummify() it.

		NOTE: This already sets the map as the parent of the block
	*/
	if(block == NULL)
	{
		block = sector->createBlankBlockNoInsert(block_y);
	}
	else
	{
		// Remove the block so that nobody can get a half-generated one.
		sector->removeBlock(block);
		// Allocate the block to contain the generated data
		block->unDummify();
	}
	
#if 0
	/*
		Generate a completely empty block
	*/
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
		{
			MapNode n;
			n.d = CONTENT_AIR;
			block->setNode(v3s16(x0,y0,z0), n);
		}
	}
#else
	/*
		Generate a proper block
	*/
	
	u8 water_material = CONTENT_WATERSOURCE;
	
	s32 lowest_ground_y = 32767;
	s32 highest_ground_y = -32768;
	
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		//dstream<<"generateBlock: x0="<<x0<<", z0="<<z0<<std::endl;

		//s16 surface_y = 0;

		s16 mud_add_amount = get_mud_add_amount(m_seed, p2d_nodes+v2s16(x0,z0));

		s16 surface_y = base_rock_level_2d(m_seed, p2d_nodes+v2s16(x0,z0))
				+ mud_add_amount;

		/*// Less mud if it is high
		if(surface_y >= 64)
		{
			mud_add_amount--;
			surface_y--;
		}*/

		// If chunks are disabled
		if(m_chunksize == 0)
			surface_y = WATER_LEVEL + 1;

		if(surface_y < lowest_ground_y)
			lowest_ground_y = surface_y;
		if(surface_y > highest_ground_y)
			highest_ground_y = surface_y;

		s32 surface_depth = AVERAGE_MUD_AMOUNT;
		
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
				n.setLight(LIGHTBANK_DAY, LIGHT_SUN);

			/*
				Calculate material
			*/

			// If node is over heightmap y, it's air or water
			if(real_y > surface_y)
			{
				// If under water level, it's water
				if(real_y < WATER_LEVEL)
				{
					n.d = water_material;
					n.setLight(LIGHTBANK_DAY,
							diminish_light(LIGHT_SUN, WATER_LEVEL-real_y+1));
					/*
						Add to transforming liquid queue (in case it'd
						start flowing)
					*/
					v3s16 real_pos = v3s16(x0,y0,z0) + p*MAP_BLOCKSIZE;
					m_transforming_liquid.push_back(real_pos);
				}
				// else air
				else
					n.d = CONTENT_AIR;
			}
			// Else it's ground or caves (air)
			else
			{
				// If it's cave, it's cave
				if(is_cave(m_seed, block->getPosRelative()+v3s16(x0,y0,z0)))
				{
					n.d = CONTENT_AIR;
				}
				// If it's surface_depth under ground, it's stone
				else if(real_y <= surface_y - surface_depth)
				{
					n.d = CONTENT_STONE;
				}
				else
				{
					// It is mud if it is under the first ground
					// level or under water
					if(real_y < WATER_LEVEL || real_y <= surface_y - 1)
					{
						n.d = CONTENT_MUD;
					}
					else
					{
						n.d = CONTENT_GRASS;
					}

					//n.d = CONTENT_MUD;
					
					/*// If under water level, it's mud
					if(real_y < WATER_LEVEL)
						n.d = CONTENT_MUD;
					// Only the topmost node is grass
					else if(real_y <= surface_y - 1)
						n.d = CONTENT_MUD;
					else
						n.d = CONTENT_GRASS;*/
				}
			}

			block->setNode(v3s16(x0,y0,z0), n);
		}
	}
	
	/*
		Calculate some helper variables
	*/
	
	// Completely underground if the highest part of block is under lowest
	// ground height.
	// This has to be very sure; it's probably one too strict now but
	// that's just better.
	bool completely_underground =
			block_y * MAP_BLOCKSIZE + MAP_BLOCKSIZE < lowest_ground_y;

	bool some_part_underground = block_y * MAP_BLOCKSIZE <= highest_ground_y;

	bool mostly_underwater_surface = false;
	if(highest_ground_y < WATER_LEVEL
			&& some_part_underground && !completely_underground)
		mostly_underwater_surface = true;

#if 0
	/*
		Generate caves
	*/

	float caves_amount = 0.5;

	// Initialize temporary table
	const s32 ued = MAP_BLOCKSIZE;
	bool underground_emptiness[ued*ued*ued];
	for(s32 i=0; i<ued*ued*ued; i++)
	{
		underground_emptiness[i] = 0;
	}
	
	// Fill table
#if 1
	{
		/*
			Initialize orp and ors. Try to find if some neighboring
			MapBlock has a tunnel ended in its side
		*/

		v3f orp(
			(float)(myrand()%ued)+0.5,
			(float)(myrand()%ued)+0.5,
			(float)(myrand()%ued)+0.5
		);
		
		bool found_existing = false;

		// Check z-
		try
		{
			s16 z = -1;
			for(s16 y=0; y<ued; y++)
			for(s16 x=0; x<ued; x++)
			{
				v3s16 ap = v3s16(x,y,z) + block->getPosRelative();
				if(getNode(ap).d == CONTENT_AIR)
				{
					orp = v3f(x+1,y+1,0);
					found_existing = true;
					goto continue_generating;
				}
			}
		}
		catch(InvalidPositionException &e){}
		
		// Check z+
		try
		{
			s16 z = ued;
			for(s16 y=0; y<ued; y++)
			for(s16 x=0; x<ued; x++)
			{
				v3s16 ap = v3s16(x,y,z) + block->getPosRelative();
				if(getNode(ap).d == CONTENT_AIR)
				{
					orp = v3f(x+1,y+1,ued-1);
					found_existing = true;
					goto continue_generating;
				}
			}
		}
		catch(InvalidPositionException &e){}
		
		// Check x-
		try
		{
			s16 x = -1;
			for(s16 y=0; y<ued; y++)
			for(s16 z=0; z<ued; z++)
			{
				v3s16 ap = v3s16(x,y,z) + block->getPosRelative();
				if(getNode(ap).d == CONTENT_AIR)
				{
					orp = v3f(0,y+1,z+1);
					found_existing = true;
					goto continue_generating;
				}
			}
		}
		catch(InvalidPositionException &e){}
		
		// Check x+
		try
		{
			s16 x = ued;
			for(s16 y=0; y<ued; y++)
			for(s16 z=0; z<ued; z++)
			{
				v3s16 ap = v3s16(x,y,z) + block->getPosRelative();
				if(getNode(ap).d == CONTENT_AIR)
				{
					orp = v3f(ued-1,y+1,z+1);
					found_existing = true;
					goto continue_generating;
				}
			}
		}
		catch(InvalidPositionException &e){}

		// Check y-
		try
		{
			s16 y = -1;
			for(s16 x=0; x<ued; x++)
			for(s16 z=0; z<ued; z++)
			{
				v3s16 ap = v3s16(x,y,z) + block->getPosRelative();
				if(getNode(ap).d == CONTENT_AIR)
				{
					orp = v3f(x+1,0,z+1);
					found_existing = true;
					goto continue_generating;
				}
			}
		}
		catch(InvalidPositionException &e){}
		
		// Check y+
		try
		{
			s16 y = ued;
			for(s16 x=0; x<ued; x++)
			for(s16 z=0; z<ued; z++)
			{
				v3s16 ap = v3s16(x,y,z) + block->getPosRelative();
				if(getNode(ap).d == CONTENT_AIR)
				{
					orp = v3f(x+1,ued-1,z+1);
					found_existing = true;
					goto continue_generating;
				}
			}
		}
		catch(InvalidPositionException &e){}

continue_generating:
		
		/*
			Choose whether to actually generate cave
		*/
		bool do_generate_caves = true;
		// Don't generate if no part is underground
		if(!some_part_underground)
		{
			do_generate_caves = false;
		}
		// Don't generate if mostly underwater surface
		/*else if(mostly_underwater_surface)
		{
			do_generate_caves = false;
		}*/
		// Partly underground = cave
		else if(!completely_underground)
		{
			do_generate_caves = (rand() % 100 <= (s32)(caves_amount*100));
		}
		// Found existing cave underground
		else if(found_existing && completely_underground)
		{
			do_generate_caves = (rand() % 100 <= (s32)(caves_amount*100));
		}
		// Underground and no caves found
		else
		{
			do_generate_caves = (rand() % 300 <= (s32)(caves_amount*100));
		}

		if(do_generate_caves)
		{
			/*
				Generate some tunnel starting from orp and ors
			*/
			for(u16 i=0; i<3; i++)
			{
				v3f rp(
					(float)(myrand()%ued)+0.5,
					(float)(myrand()%ued)+0.5,
					(float)(myrand()%ued)+0.5
				);
				s16 min_d = 0;
				s16 max_d = 4;
				s16 rs = (myrand()%(max_d-min_d+1))+min_d;
				
				v3f vec = rp - orp;

				for(float f=0; f<1.0; f+=0.04)
				{
					v3f fp = orp + vec * f;
					v3s16 cp(fp.X, fp.Y, fp.Z);
					s16 d0 = -rs/2;
					s16 d1 = d0 + rs - 1;
					for(s16 z0=d0; z0<=d1; z0++)
					{
						s16 si = rs - abs(z0);
						for(s16 x0=-si; x0<=si-1; x0++)
						{
							s16 si2 = rs - abs(x0);
							for(s16 y0=-si2+1; y0<=si2-1; y0++)
							{
								s16 z = cp.Z + z0;
								s16 y = cp.Y + y0;
								s16 x = cp.X + x0;
								v3s16 p(x,y,z);
								if(isInArea(p, ued) == false)
									continue;
								underground_emptiness[ued*ued*z + ued*y + x] = 1;
							}
						}
					}
				}

				orp = rp;
			}
		}
	}
#endif

	// Set to true if has caves.
	// Set when some non-air is changed to air when making caves.
	bool has_caves = false;

	/*
		Apply temporary cave data to block
	*/

	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
		{
			MapNode n = block->getNode(v3s16(x0,y0,z0));

			// Create caves
			if(underground_emptiness[
					ued*ued*(z0*ued/MAP_BLOCKSIZE)
					+ued*(y0*ued/MAP_BLOCKSIZE)
					+(x0*ued/MAP_BLOCKSIZE)])
			{
				if(content_features(n.d).walkable/*is_ground_content(n.d)*/)
				{
					// Has now caves
					has_caves = true;
					// Set air to node
					n.d = CONTENT_AIR;
				}
			}

			block->setNode(v3s16(x0,y0,z0), n);
		}
	}
#endif
	
	/*
		This is used for guessing whether or not the block should
		receive sunlight from the top if the block above doesn't exist
	*/
	block->setIsUnderground(completely_underground);

	/*
		Force lighting update if some part of block is partly
		underground and has caves.
	*/
	/*if(some_part_underground && !completely_underground && has_caves)
	{
		//dstream<<"Half-ground caves"<<std::endl;
		lighting_invalidated_blocks[block->getPos()] = block;
	}*/
	
	// DEBUG: Always update lighting
	//lighting_invalidated_blocks[block->getPos()] = block;

	/*
		Add some minerals
	*/

	if(some_part_underground)
	{
		s16 underground_level = (lowest_ground_y/MAP_BLOCKSIZE - block_y)+1;

		/*
			Add meseblocks
		*/
		for(s16 i=0; i<underground_level/4 + 1; i++)
		{
			if(myrand()%50 == 0)
			{
				v3s16 cp(
					(myrand()%(MAP_BLOCKSIZE-2))+1,
					(myrand()%(MAP_BLOCKSIZE-2))+1,
					(myrand()%(MAP_BLOCKSIZE-2))+1
				);

				MapNode n;
				n.d = CONTENT_MESE;
				
				for(u16 i=0; i<27; i++)
				{
					if(block->getNode(cp+g_27dirs[i]).d == CONTENT_STONE)
						if(myrand()%8 == 0)
							block->setNode(cp+g_27dirs[i], n);
				}
			}
		}

		/*
			Add coal
		*/
		u16 coal_amount = 30;
		u16 coal_rareness = 60 / coal_amount;
		if(coal_rareness == 0)
			coal_rareness = 1;
		if(myrand()%coal_rareness == 0)
		{
			u16 a = myrand() % 16;
			u16 amount = coal_amount * a*a*a / 1000;
			for(s16 i=0; i<amount; i++)
			{
				v3s16 cp(
					(myrand()%(MAP_BLOCKSIZE-2))+1,
					(myrand()%(MAP_BLOCKSIZE-2))+1,
					(myrand()%(MAP_BLOCKSIZE-2))+1
				);

				MapNode n;
				n.d = CONTENT_STONE;
				n.param = MINERAL_COAL;

				for(u16 i=0; i<27; i++)
				{
					if(block->getNode(cp+g_27dirs[i]).d == CONTENT_STONE)
						if(myrand()%8 == 0)
							block->setNode(cp+g_27dirs[i], n);
				}
			}
		}

		/*
			Add iron
		*/
		//TODO: change to iron_amount or whatever
		u16 iron_amount = 15;
		u16 iron_rareness = 60 / iron_amount;
		if(iron_rareness == 0)
			iron_rareness = 1;
		if(myrand()%iron_rareness == 0)
		{
			u16 a = myrand() % 16;
			u16 amount = iron_amount * a*a*a / 1000;
			for(s16 i=0; i<amount; i++)
			{
				v3s16 cp(
					(myrand()%(MAP_BLOCKSIZE-2))+1,
					(myrand()%(MAP_BLOCKSIZE-2))+1,
					(myrand()%(MAP_BLOCKSIZE-2))+1
				);

				MapNode n;
				n.d = CONTENT_STONE;
				n.param = MINERAL_IRON;

				for(u16 i=0; i<27; i++)
				{
					if(block->getNode(cp+g_27dirs[i]).d == CONTENT_STONE)
						if(myrand()%8 == 0)
							block->setNode(cp+g_27dirs[i], n);
				}
			}
		}
	}
	
	/*
		Create a few rats in empty blocks underground
	*/
	if(completely_underground)
	{
		//for(u16 i=0; i<2; i++)
		{
			v3s16 cp(
				(myrand()%(MAP_BLOCKSIZE-2))+1,
				(myrand()%(MAP_BLOCKSIZE-2))+1,
				(myrand()%(MAP_BLOCKSIZE-2))+1
			);

			// Check that the place is empty
			//if(!is_ground_content(block->getNode(cp).d))
			if(1)
			{
				RatObject *obj = new RatObject(NULL, -1, intToFloat(cp, BS));
				block->addObject(obj);
			}
		}
	}

#endif // end of proper block generation
	
	/*
		Add block to sector.
	*/
	sector->insertBlock(block);
	
	// Lighting is invalid after generation.
	block->setLightingExpired(true);
	
#if 0
	/*
		Debug information
	*/
	dstream
	<<"lighting_invalidated_blocks.size()"
	<<", has_caves"
	<<", completely_ug"
	<<", some_part_ug"
	<<"  "<<lighting_invalidated_blocks.size()
	<<", "<<has_caves
	<<", "<<completely_underground
	<<", "<<some_part_underground
	<<std::endl;
#endif

	return block;
}

MapBlock * ServerMap::createBlock(v3s16 p)
{
	DSTACK("%s: p=(%d,%d,%d)",
			__FUNCTION_NAME, p.X, p.Y, p.Z);
	
	/*
		Do not create over-limit
	*/
	if(p.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.X > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Y > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Z < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Z > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE)
		throw InvalidPositionException("createBlock(): pos. over limit");
	
	v2s16 p2d(p.X, p.Z);
	s16 block_y = p.Y;
	/*
		This will create or load a sector if not found in memory.
		If block exists on disk, it will be loaded.

		NOTE: On old save formats, this will be slow, as it generates
		      lighting on blocks for them.
	*/
	ServerMapSector *sector;
	try{
		sector = (ServerMapSector*)createSector(p2d);
		assert(sector->getId() == MAPSECTOR_SERVER);
	}
	catch(InvalidPositionException &e)
	{
		dstream<<"createBlock: createSector() failed"<<std::endl;
		throw e;
	}
	/*
		NOTE: This should not be done, or at least the exception
		should not be passed on as std::exception, because it
		won't be catched at all.
	*/
	/*catch(std::exception &e)
	{
		dstream<<"createBlock: createSector() failed: "
				<<e.what()<<std::endl;
		throw e;
	}*/

	/*
		Try to get a block from the sector
	*/

	MapBlock *block = sector->getBlockNoCreateNoEx(block_y);
	if(block)
		return block;
	// Create blank
	block = sector->createBlankBlock(block_y);
	return block;
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
	
	/*
		Do not generate over-limit
	*/
	if(p.X < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.X > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Y < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Y > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Z < -MAP_GENERATION_LIMIT / MAP_BLOCKSIZE
	|| p.Z > MAP_GENERATION_LIMIT / MAP_BLOCKSIZE)
		throw InvalidPositionException("emergeBlock(): pos. over limit");
	
	v2s16 p2d(p.X, p.Z);
	s16 block_y = p.Y;
	/*
		This will create or load a sector if not found in memory.
		If block exists on disk, it will be loaded.
	*/
	ServerMapSector *sector;
	try{
		sector = (ServerMapSector*)emergeSector(p2d, changed_blocks);
		assert(sector->getId() == MAPSECTOR_SERVER);
	}
	catch(InvalidPositionException &e)
	{
		dstream<<"emergeBlock: emergeSector() failed: "
				<<e.what()<<std::endl;
		dstream<<"Path to failed sector: "<<getSectorDir(p2d)
				<<std::endl
				<<"You could try to delete it."<<std::endl;
		throw e;
	}
	catch(VersionMismatchException &e)
	{
		dstream<<"emergeBlock: emergeSector() failed: "
				<<e.what()<<std::endl;
		dstream<<"Path to failed sector: "<<getSectorDir(p2d)
				<<std::endl
				<<"You could try to delete it."<<std::endl;
		throw e;
	}
	/*
		NOTE: This should not be done, or at least the exception
		should not be passed on as std::exception, because it
		won't be catched at all.
	*/
	/*catch(std::exception &e)
	{
		dstream<<"emergeBlock: emergeSector() failed: "
				<<e.what()<<std::endl;
		dstream<<"Path to failed sector: "<<getSectorDir(p2d)
				<<std::endl
				<<"You could try to delete it."<<std::endl;
		throw e;
	}*/

	/*
		Try to get a block from the sector
	*/

	bool does_not_exist = false;
	bool lighting_expired = false;
	MapBlock *block = sector->getBlockNoCreateNoEx(block_y);

	if(block == NULL)
	{
		does_not_exist = true;
	}
	else if(block->isDummy() == true)
	{
		does_not_exist = true;
	}
	else if(block->getLightingExpired())
	{
		lighting_expired = true;
	}
	else
	{
		// Valid block
		//dstream<<"emergeBlock(): Returning already valid block"<<std::endl;
		return block;
	}
	
	/*
		If block was not found on disk and not going to generate a
		new one, make sure there is a dummy block in place.
	*/
	if(only_from_disk && (does_not_exist || lighting_expired))
	{
		//dstream<<"emergeBlock(): Was not on disk but not generating"<<std::endl;

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
	// 0ms
	//TimeTaker("emergeBlock() generate");

	//dstream<<"emergeBlock(): Didn't find valid block -> making one"<<std::endl;

	/*
		If the block doesn't exist, generate the block.
	*/
	if(does_not_exist)
	{
		block = generateBlock(p, block, sector, changed_blocks,
				lighting_invalidated_blocks); 
	}

	if(lighting_expired)
	{
		lighting_invalidated_blocks.insert(p, block);
	}

	/*
		Initially update sunlight
	*/
	
	{
		core::map<v3s16, bool> light_sources;
		bool black_air_left = false;
		bool bottom_invalid =
				block->propagateSunlight(light_sources, true,
				&black_air_left, true);

		// If sunlight didn't reach everywhere and part of block is
		// above ground, lighting has to be properly updated
		//if(black_air_left && some_part_underground)
		if(black_air_left)
		{
			lighting_invalidated_blocks[block->getPos()] = block;
		}

		if(bottom_invalid)
		{
			lighting_invalidated_blocks[block->getPos()] = block;
		}
	}
	
	return block;
}

s16 ServerMap::findGroundLevel(v2s16 p2d)
{
	/*
		Uh, just do something random...
	*/
	// Find existing map from top to down
	s16 max=63;
	s16 min=-64;
	v3s16 p(p2d.X, max, p2d.Y);
	for(; p.Y>min; p.Y--)
	{
		MapNode n = getNodeNoEx(p);
		if(n.d != CONTENT_IGNORE)
			break;
	}
	if(p.Y == min)
		goto plan_b;
	// If this node is not air, go to plan b
	if(getNodeNoEx(p).d != CONTENT_AIR)
		goto plan_b;
	// Search existing walkable and return it
	for(; p.Y>min; p.Y--)
	{
		MapNode n = getNodeNoEx(p);
		if(content_walkable(n.d) && n.d != CONTENT_IGNORE)
			return p.Y;
	}
	// Move to plan b
plan_b:
	/*
		Plan B: Get from map generator perlin noise function
	*/
	// This won't work if proper generation is disabled
	if(m_chunksize == 0)
		return WATER_LEVEL+2;
	double level = base_rock_level_2d(m_seed, p2d) + AVERAGE_MUD_AMOUNT;
	return (s16)level;
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
	
	if(only_changed == false || m_map_metadata_changed)
	{
		saveMapMeta();
	}

	// Disable saving chunk metadata if chunks are disabled
	if(m_chunksize != 0)
	{
		if(only_changed == false || anyChunkModified())
			saveChunkMeta();
	}
	
	u32 sector_meta_count = 0;
	u32 block_count = 0;
	
	{ //sectorlock
	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
	
	core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
	for(; i.atEnd() == false; i++)
	{
		ServerMapSector *sector = (ServerMapSector*)i.getNode()->getValue();
		assert(sector->getId() == MAPSECTOR_SERVER);
	
		if(sector->differs_from_disk || only_changed == false)
		{
			saveSectorMeta(sector);
			sector_meta_count++;
		}
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

				/*dstream<<"ServerMap: Written block ("
						<<block->getPos().X<<","
						<<block->getPos().Y<<","
						<<block->getPos().Z<<")"
						<<std::endl;*/
			}
		}
	}

	}//sectorlock
	
	/*
		Only print if something happened or saved whole map
	*/
	if(only_changed == false || sector_meta_count != 0
			|| block_count != 0)
	{
		dstream<<DTIME<<"ServerMap: Written: "
				<<sector_meta_count<<" sector metadata files, "
				<<block_count<<" block files"
				<<std::endl;
	}
}

#if 0
// NOTE: Doing this is insane. Deprecated and probably broken.
void ServerMap::loadAll()
{
	DSTACK(__FUNCTION_NAME);
	dstream<<DTIME<<"ServerMap: Loading map..."<<std::endl;
	
	loadMapMeta();
	loadChunkMeta();

	std::vector<fs::DirListNode> list = fs::GetDirListing(m_savedir+"/sectors/");

	dstream<<DTIME<<"There are "<<list.size()<<" sectors."<<std::endl;
	
	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
	
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
	dstream<<DTIME<<"ServerMap: Map loaded."<<std::endl;
}
#endif

#if 0
void ServerMap::saveMasterHeightmap()
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<"DEPRECATED: "<<__FUNCTION_NAME<<std::endl;

	createDir(m_savedir);
	
	/*std::string fullpath = m_savedir + "/master_heightmap";
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open master heightmap");*/
	
	// Format used for writing
	//u8 version = SER_FMT_VER_HIGHEST;
}

void ServerMap::loadMasterHeightmap()
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<"DEPRECATED: "<<__FUNCTION_NAME<<std::endl;

	/*std::string fullpath = m_savedir + "/master_heightmap";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
		throw FileNotGoodException("Cannot open master heightmap");*/
}
#endif

void ServerMap::saveMapMeta()
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<"INFO: ServerMap::saveMapMeta(): "
			<<"seed="<<m_seed<<", chunksize="<<m_chunksize
			<<std::endl;

	createDir(m_savedir);
	
	std::string fullpath = m_savedir + "/map_meta.txt";
	std::ofstream os(fullpath.c_str(), std::ios_base::binary);
	if(os.good() == false)
	{
		dstream<<"ERROR: ServerMap::saveMapMeta(): "
				<<"could not open"<<fullpath<<std::endl;
		throw FileNotGoodException("Cannot open chunk metadata");
	}
	
	Settings params;
	params.setU64("seed", m_seed);
	params.setS32("chunksize", m_chunksize);

	params.writeLines(os);

	os<<"[end_of_params]\n";
	
	m_map_metadata_changed = false;
}

void ServerMap::loadMapMeta()
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<"INFO: ServerMap::loadMapMeta(): Loading map metadata"
			<<std::endl;

	std::string fullpath = m_savedir + "/map_meta.txt";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
	{
		dstream<<"ERROR: ServerMap::loadMapMeta(): "
				<<"could not open"<<fullpath<<std::endl;
		throw FileNotGoodException("Cannot open map metadata");
	}

	Settings params;

	for(;;)
	{
		if(is.eof())
			throw SerializationError
					("ServerMap::loadMapMeta(): [end_of_params] not found");
		std::string line;
		std::getline(is, line);
		std::string trimmedline = trim(line);
		if(trimmedline == "[end_of_params]")
			break;
		params.parseConfigLine(line);
	}

	m_seed = params.getU64("seed");
	m_chunksize = params.getS32("chunksize");

	dstream<<"INFO: ServerMap::loadMapMeta(): "
			<<"seed="<<m_seed<<", chunksize="<<m_chunksize
			<<std::endl;
}

void ServerMap::saveChunkMeta()
{
	DSTACK(__FUNCTION_NAME);

	// This should not be called if chunks are disabled.
	assert(m_chunksize != 0);
	
	u32 count = m_chunks.size();

	dstream<<"INFO: ServerMap::saveChunkMeta(): Saving metadata of "
			<<count<<" chunks"<<std::endl;

	createDir(m_savedir);
	
	std::string fullpath = m_savedir + "/chunk_meta";
	std::ofstream os(fullpath.c_str(), std::ios_base::binary);
	if(os.good() == false)
	{
		dstream<<"ERROR: ServerMap::saveChunkMeta(): "
				<<"could not open"<<fullpath<<std::endl;
		throw FileNotGoodException("Cannot open chunk metadata");
	}
	
	u8 version = 0;
	
	// Write version
	os.write((char*)&version, 1);

	u8 buf[4];
	
	// Write count
	writeU32(buf, count);
	os.write((char*)buf, 4);
	
	for(core::map<v2s16, MapChunk*>::Iterator
			i = m_chunks.getIterator();
			i.atEnd()==false; i++)
	{
		v2s16 p = i.getNode()->getKey();
		MapChunk *chunk = i.getNode()->getValue();
		// Write position
		writeV2S16(buf, p);
		os.write((char*)buf, 4);
		// Write chunk data
		chunk->serialize(os, version);
	}

	setChunksNonModified();
}

void ServerMap::loadChunkMeta()
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<"INFO: ServerMap::loadChunkMeta(): Loading chunk metadata"
			<<std::endl;

	std::string fullpath = m_savedir + "/chunk_meta";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
	{
		dstream<<"ERROR: ServerMap::loadChunkMeta(): "
				<<"could not open"<<fullpath<<std::endl;
		throw FileNotGoodException("Cannot open chunk metadata");
	}

	u8 version = 0;
	
	// Read version
	is.read((char*)&version, 1);

	u8 buf[4];
	
	// Read count
	is.read((char*)buf, 4);
	u32 count = readU32(buf);

	dstream<<"INFO: ServerMap::loadChunkMeta(): Loading metadata of "
			<<count<<" chunks"<<std::endl;
	
	for(u32 i=0; i<count; i++)
	{
		v2s16 p;
		MapChunk *chunk = new MapChunk();
		// Read position
		is.read((char*)buf, 4);
		p = readV2S16(buf);
		// Read chunk data
		chunk->deSerialize(is, version);
		m_chunks.insert(p, chunk);
	}
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
	
	std::string fullpath = dir + "/meta";
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open sector metafile");

	sector->serialize(o, version);
	
	sector->differs_from_disk = false;
}

MapSector* ServerMap::loadSectorMeta(std::string dirname)
{
	DSTACK(__FUNCTION_NAME);
	// Get destination
	v2s16 p2d = getSectorPos(dirname);
	std::string dir = m_savedir + "/sectors/" + dirname;

	ServerMapSector *sector = NULL;
	
	std::string fullpath = dir + "/meta";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
	{
		// If the directory exists anyway, it probably is in some old
		// format. Just go ahead and create the sector.
		if(fs::PathExists(dir))
		{
			dstream<<"ServerMap::loadSectorMeta(): Sector metafile "
					<<fullpath<<" doesn't exist but directory does."
					<<" Continuing with a sector with no metadata."
					<<std::endl;
			sector = new ServerMapSector(this, p2d);
			m_sectors.insert(p2d, sector);
		}
		else
			throw FileNotGoodException("Cannot open sector metafile");
	}
	else
	{
		sector = ServerMapSector::deSerialize
				(is, this, p2d, m_sectors);
	}
	
	sector->differs_from_disk = false;

	return sector;
}

bool ServerMap::loadSectorFull(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);
	std::string sectorsubdir = getSectorSubDir(p2d);

	MapSector *sector = NULL;

	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out

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
	
	/*
		Load blocks
	*/
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
	return true;
}

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
	
	/*
		Versions up from 15 have static objects.
	*/
	if(version >= 15)
	{
		block->m_static_objects.serialize(o);
	}
	
	// We just wrote it to the disk
	block->resetChangedFlag();
}

void ServerMap::loadBlock(std::string sectordir, std::string blockfile, MapSector *sector)
{
	DSTACK(__FUNCTION_NAME);

	// Block file is map/sectors/xxxxxxxx/xxxx
	std::string fullpath = m_savedir+"/sectors/"+sectordir+"/"+blockfile;
	try{

		std::ifstream is(fullpath.c_str(), std::ios_base::binary);
		if(is.good() == false)
			throw FileNotGoodException("Cannot open block file");
		
		v3s16 p3d = getBlockPos(sectordir, blockfile);
		v2s16 p2d(p3d.X, p3d.Z);
		
		assert(sector->getPos() == p2d);
		
		u8 version = SER_FMT_VER_INVALID;
		is.read((char*)&version, 1);

		if(is.fail())
			throw SerializationError("ServerMap::loadBlock(): Failed"
					" to read MapBlock version");

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
			block->updateObjects(is, version, NULL, 0);
		}

		/*
			Versions up from 15 have static objects.
		*/
		if(version >= 15)
		{
			block->m_static_objects.deSerialize(is);
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
				"(SerializationError). Ignoring. "
				"A new one will be generated."
				<<std::endl;

		// TODO: Backup file; name is in fullpath.
	}
}

void ServerMap::PrintInfo(std::ostream &out)
{
	out<<"ServerMap: ";
}

#ifndef SERVER

/*
	ClientMap
*/

ClientMap::ClientMap(
		Client *client,
		MapDrawControl &control,
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id
):
	Map(dout_client),
	scene::ISceneNode(parent, mgr, id),
	m_client(client),
	m_control(control),
	m_camera_position(0,0,0),
	m_camera_direction(0,0,1)
{
	m_camera_mutex.Init();
	assert(m_camera_mutex.IsInitialized());
	
	m_box = core::aabbox3d<f32>(-BS*1000000,-BS*1000000,-BS*1000000,
			BS*1000000,BS*1000000,BS*1000000);
}

ClientMap::~ClientMap()
{
	/*JMutexAutoLock lock(mesh_mutex);
	
	if(mesh != NULL)
	{
		mesh->drop();
		mesh = NULL;
	}*/
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
	
	// Create a sector
	ClientMapSector *sector = new ClientMapSector(this, p2d);
	
	{
		//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
		m_sectors.insert(p2d, sector);
	}
	
	return sector;
}

void ClientMap::deSerializeSector(v2s16 p2d, std::istream &is)
{
	DSTACK(__FUNCTION_NAME);
	ClientMapSector *sector = NULL;

	//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
	
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
			//JMutexAutoLock lock(m_sector_mutex); // Bulk comment-out
			m_sectors.insert(p2d, sector);
		}
	}

	sector->deSerialize(is);
}

void ClientMap::OnRegisterSceneNode()
{
	if(IsVisible)
	{
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
	}

	ISceneNode::OnRegisterSceneNode();
}

void ClientMap::renderMap(video::IVideoDriver* driver, s32 pass)
{
	//m_dout<<DTIME<<"Rendering map..."<<std::endl;
	DSTACK(__FUNCTION_NAME);

	bool is_transparent_pass = pass == scene::ESNRP_TRANSPARENT;

	/*
		Get time for measuring timeout.
		
		Measuring time is very useful for long delays when the
		machine is swapping a lot.
	*/
	int time1 = time(0);

	//u32 daynight_ratio = m_client->getDayNightRatio();

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

	v3s16 box_nodes_d = m_control.wanted_range * v3s16(1,1,1);

	v3s16 p_nodes_min = cam_pos_nodes - box_nodes_d;
	v3s16 p_nodes_max = cam_pos_nodes + box_nodes_d;

	// Take a fair amount as we will be dropping more out later
	v3s16 p_blocks_min = getNodeBlockPos(p_nodes_min) - v3s16(1,1,1);
	v3s16 p_blocks_max = getNodeBlockPos(p_nodes_max) + v3s16(1,1,1);
	
	u32 vertex_count = 0;
	
	// For limiting number of mesh updates per frame
	u32 mesh_update_count = 0;
	
	u32 blocks_would_have_drawn = 0;
	u32 blocks_drawn = 0;

	//NOTE: The sectors map should be locked but we're not doing it
	// because it'd cause too much delays

	int timecheck_counter = 0;
	core::map<v2s16, MapSector*>::Iterator si;
	si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		{
			timecheck_counter++;
			if(timecheck_counter > 50)
			{
				timecheck_counter = 0;
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
		
		if(m_control.range_all == false)
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
			
			float range = 100000 * BS;
			if(m_control.range_all == false)
				range = m_control.wanted_range * BS;
			
			float d = 0.0;
			if(isBlockInSight(block->getPos(), camera_position,
					camera_direction, range, &d) == false)
			{
				continue;
			}
			
			// This is ugly (spherical distance limit?)
			/*if(m_control.range_all == false &&
					d - 0.5*BS*MAP_BLOCKSIZE > range)
				continue;*/

#if 1
			/*
				Update expired mesh (used for day/night change)

				It doesn't work exactly like it should now with the
				tasked mesh update but whatever.
			*/

			bool mesh_expired = false;
			
			{
				JMutexAutoLock lock(block->mesh_mutex);

				mesh_expired = block->getMeshExpired();

				// Mesh has not been expired and there is no mesh:
				// block has no content
				if(block->mesh == NULL && mesh_expired == false)
					continue;
			}

			f32 faraway = BS*50;
			//f32 faraway = m_control.wanted_range * BS;
			
			/*
				This has to be done with the mesh_mutex unlocked
			*/
			// Pretty random but this should work somewhat nicely
			if(mesh_expired && (
					(mesh_update_count < 3
						&& (d < faraway || mesh_update_count < 2)
					)
					|| 
					(m_control.range_all && mesh_update_count < 20)
				)
			)
			/*if(mesh_expired && mesh_update_count < 6
					&& (d < faraway || mesh_update_count < 3))*/
			{
				mesh_update_count++;

				// Mesh has been expired: generate new mesh
				//block->updateMesh(daynight_ratio);
				m_client->addUpdateMeshTask(block->getPos());

				mesh_expired = false;
			}
			
#endif
			/*
				Draw the faces of the block
			*/
			{
				JMutexAutoLock lock(block->mesh_mutex);

				scene::SMesh *mesh = block->mesh;

				if(mesh == NULL)
					continue;
				
				blocks_would_have_drawn++;
				if(blocks_drawn >= m_control.wanted_max_blocks
						&& m_control.range_all == false
						&& d > m_control.wanted_min_range * BS)
					continue;
				blocks_drawn++;

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
						/*
							This *shouldn't* hurt too much because Irrlicht
							doesn't change opengl textures if the old
							material is set again.
						*/
						driver->setMaterial(buf->getMaterial());
						driver->drawMeshBuffer(buf);
						vertex_count += buf->getVertexCount();
					}
				}
			}
		} // foreach sectorblocks
	}
	
	m_control.blocks_drawn = blocks_drawn;
	m_control.blocks_would_have_drawn = blocks_would_have_drawn;

	/*dstream<<"renderMap(): is_transparent_pass="<<is_transparent_pass
			<<", rendered "<<vertex_count<<" vertices."<<std::endl;*/
}

bool ClientMap::setTempMod(v3s16 p, NodeMod mod,
		core::map<v3s16, MapBlock*> *affected_blocks)
{
	bool changed = false;
	/*
		Add it to all blocks touching it
	*/
	v3s16 dirs[7] = {
		v3s16(0,0,0), // this
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	for(u16 i=0; i<7; i++)
	{
		v3s16 p2 = p + dirs[i];
		// Block position of neighbor (or requested) node
		v3s16 blockpos = getNodeBlockPos(p2);
		MapBlock * blockref = getBlockNoCreateNoEx(blockpos);
		if(blockref == NULL)
			continue;
		// Relative position of requested node
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		if(blockref->setTempMod(relpos, mod))
		{
			changed = true;
		}
	}
	if(changed && affected_blocks!=NULL)
	{
		for(u16 i=0; i<7; i++)
		{
			v3s16 p2 = p + dirs[i];
			// Block position of neighbor (or requested) node
			v3s16 blockpos = getNodeBlockPos(p2);
			MapBlock * blockref = getBlockNoCreateNoEx(blockpos);
			if(blockref == NULL)
				continue;
			affected_blocks->insert(blockpos, blockref);
		}
	}
	return changed;
}

bool ClientMap::clearTempMod(v3s16 p,
		core::map<v3s16, MapBlock*> *affected_blocks)
{
	bool changed = false;
	v3s16 dirs[7] = {
		v3s16(0,0,0), // this
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	for(u16 i=0; i<7; i++)
	{
		v3s16 p2 = p + dirs[i];
		// Block position of neighbor (or requested) node
		v3s16 blockpos = getNodeBlockPos(p2);
		MapBlock * blockref = getBlockNoCreateNoEx(blockpos);
		if(blockref == NULL)
			continue;
		// Relative position of requested node
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		if(blockref->clearTempMod(relpos))
		{
			changed = true;
		}
	}
	if(changed && affected_blocks!=NULL)
	{
		for(u16 i=0; i<7; i++)
		{
			v3s16 p2 = p + dirs[i];
			// Block position of neighbor (or requested) node
			v3s16 blockpos = getNodeBlockPos(p2);
			MapBlock * blockref = getBlockNoCreateNoEx(blockpos);
			if(blockref == NULL)
				continue;
			affected_blocks->insert(blockpos, blockref);
		}
	}
	return changed;
}

void ClientMap::expireMeshes(bool only_daynight_diffed)
{
	TimeTaker timer("expireMeshes()");

	core::map<v2s16, MapSector*>::Iterator si;
	si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();

		core::list< MapBlock * > sectorblocks;
		sector->getBlocks(sectorblocks);
		
		core::list< MapBlock * >::Iterator i;
		for(i=sectorblocks.begin(); i!=sectorblocks.end(); i++)
		{
			MapBlock *block = *i;

			if(only_daynight_diffed && dayNightDiffed(block->getPos()) == false)
			{
				continue;
			}
			
			{
				JMutexAutoLock lock(block->mesh_mutex);
				if(block->mesh != NULL)
				{
					/*block->mesh->drop();
					block->mesh = NULL;*/
					block->setMeshExpired(true);
				}
			}
		}
	}
}

void ClientMap::updateMeshes(v3s16 blockpos, u32 daynight_ratio)
{
	assert(mapType() == MAPTYPE_CLIENT);

	try{
		v3s16 p = blockpos + v3s16(0,0,0);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh(daynight_ratio);
		//b->setMeshExpired(true);
	}
	catch(InvalidPositionException &e){}
	// Leading edge
	try{
		v3s16 p = blockpos + v3s16(-1,0,0);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh(daynight_ratio);
		//b->setMeshExpired(true);
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,-1,0);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh(daynight_ratio);
		//b->setMeshExpired(true);
	}
	catch(InvalidPositionException &e){}
	try{
		v3s16 p = blockpos + v3s16(0,0,-1);
		MapBlock *b = getBlockNoCreate(p);
		b->updateMesh(daynight_ratio);
		//b->setMeshExpired(true);
	}
	catch(InvalidPositionException &e){}
}

#if 0
/*
	Update mesh of block in which the node is, and if the node is at the
	leading edge, update the appropriate leading blocks too.
*/
void ClientMap::updateNodeMeshes(v3s16 nodepos, u32 daynight_ratio)
{
	v3s16 dirs[4] = {
		v3s16(0,0,0),
		v3s16(-1,0,0),
		v3s16(0,-1,0),
		v3s16(0,0,-1),
	};
	v3s16 blockposes[4];
	for(u32 i=0; i<4; i++)
	{
		v3s16 np = nodepos + dirs[i];
		blockposes[i] = getNodeBlockPos(np);
		// Don't update mesh of block if it has been done already
		bool already_updated = false;
		for(u32 j=0; j<i; j++)
		{
			if(blockposes[j] == blockposes[i])
			{
				already_updated = true;
				break;
			}
		}
		if(already_updated)
			continue;
		// Update mesh
		MapBlock *b = getBlockNoCreate(blockposes[i]);
		b->updateMesh(daynight_ratio);
	}
}
#endif

void ClientMap::PrintInfo(std::ostream &out)
{
	out<<"ClientMap: ";
}

#endif // !SERVER

/*
	MapVoxelManipulator
*/

MapVoxelManipulator::MapVoxelManipulator(Map *map)
{
	m_map = map;
}

MapVoxelManipulator::~MapVoxelManipulator()
{
	/*dstream<<"MapVoxelManipulator: blocks: "<<m_loaded_blocks.size()
			<<std::endl;*/
}

void MapVoxelManipulator::emerge(VoxelArea a, s32 caller_id)
{
	TimeTaker timer1("emerge", &emerge_time);

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
			TimeTaker timer1("emerge load", &emerge_load_time);

			/*dstream<<"Loading block (caller_id="<<caller_id<<")"
					<<" ("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<" wanted area: ";
			a.print(dstream);
			dstream<<std::endl;*/
			
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

		m_loaded_blocks.insert(p, !block_data_inexistent);
	}

	//dstream<<"emerge done"<<std::endl;
}

/*
	SUGG: Add an option to only update eg. water and air nodes.
	      This will make it interfere less with important stuff if
		  run on background.
*/
void MapVoxelManipulator::blitBack
		(core::map<v3s16, MapBlock*> & modified_blocks)
{
	if(m_area.getExtent() == v3s16(0,0,0))
		return;
	
	//TimeTaker timer1("blitBack");

	/*dstream<<"blitBack(): m_loaded_blocks.size()="
			<<m_loaded_blocks.size()<<std::endl;*/
	
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

ManualMapVoxelManipulator::ManualMapVoxelManipulator(Map *map):
		MapVoxelManipulator(map),
		m_create_area(false)
{
}

ManualMapVoxelManipulator::~ManualMapVoxelManipulator()
{
}

void ManualMapVoxelManipulator::emerge(VoxelArea a, s32 caller_id)
{
	// Just create the area so that it can be pointed to
	VoxelManipulator::emerge(a, caller_id);
}

void ManualMapVoxelManipulator::initialEmerge(
		v3s16 blockpos_min, v3s16 blockpos_max)
{
	TimeTaker timer1("initialEmerge", &emerge_time);

	// Units of these are MapBlocks
	v3s16 p_min = blockpos_min;
	v3s16 p_max = blockpos_max;

	VoxelArea block_area_nodes
			(p_min*MAP_BLOCKSIZE, (p_max+1)*MAP_BLOCKSIZE-v3s16(1,1,1));
	
	u32 size_MB = block_area_nodes.getVolume()*4/1000000;
	if(size_MB >= 1)
	{
		dstream<<"initialEmerge: area: ";
		block_area_nodes.print(dstream);
		dstream<<" ("<<size_MB<<"MB)";
		dstream<<std::endl;
	}

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
			TimeTaker timer1("emerge load", &emerge_load_time);

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
			/*
				Mark area inexistent
			*/
			VoxelArea a(p*MAP_BLOCKSIZE, (p+1)*MAP_BLOCKSIZE-v3s16(1,1,1));
			// Fill with VOXELFLAG_INEXISTENT
			for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
			for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
			{
				s32 i = m_area.index(a.MinEdge.X,y,z);
				memset(&m_flags[i], VOXELFLAG_INEXISTENT, MAP_BLOCKSIZE);
			}
		}

		m_loaded_blocks.insert(p, !block_data_inexistent);
	}
}

void ManualMapVoxelManipulator::blitBackAll(
		core::map<v3s16, MapBlock*> * modified_blocks)
{
	if(m_area.getExtent() == v3s16(0,0,0))
		return;
	
	/*
		Copy data of all blocks
	*/
	for(core::map<v3s16, bool>::Iterator
			i = m_loaded_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		bool existed = i.getNode()->getValue();
		if(existed == false)
			continue;
		v3s16 p = i.getNode()->getKey();
		MapBlock *block = m_map->getBlockNoCreateNoEx(p);
		if(block == NULL)
		{
			dstream<<"WARNING: "<<__FUNCTION_NAME
					<<": got NULL block "
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<std::endl;
			continue;
		}

		block->copyFrom(*this);

		if(modified_blocks)
			modified_blocks->insert(p, block);
	}
}

//END
