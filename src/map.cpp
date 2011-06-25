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
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "client.h"
#include "filesys.h"
#include "utility.h"
#include "voxel.h"
#include "porting.h"
#include "mineral.h"
#include "noise.h"
#include "serverobject.h"
#include "content_mapnode.h"
#include "mapgen.h"
#include "nodemetadata.h"

extern "C" {
	#include "sqlite3.h"
}
/*
	SQLite format specification:
	- Initially only replaces sectors/ and sectors2/
*/

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
		return sector;
	}
	
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
	
	if(n == NULL)
		return NULL;
	
	MapSector *sector = n->getValue();
	
	// Cache the last result
	m_sector_cache_p = p;
	m_sector_cache = sector;

	return sector;
}

MapSector * Map::getSectorNoGenerateNoEx(v2s16 p)
{
	return getSectorNoGenerateNoExNoLock(p);
}

MapSector * Map::getSectorNoGenerate(v2s16 p)
{
	MapSector *sector = getSectorNoGenerateNoEx(p);
	if(sector == NULL)
		throw InvalidPositionException();
	
	return sector;
}

MapBlock * Map::getBlockNoCreateNoEx(v3s16 p3d)
{
	v2s16 p2d(p3d.X, p3d.Z);
	MapSector * sector = getSectorNoGenerateNoEx(p2d);
	if(sector == NULL)
		return NULL;
	MapBlock *block = sector->getBlockNoCreateNoEx(p3d.Y);
	return block;
}

MapBlock * Map::getBlockNoCreate(v3s16 p3d)
{	
	MapBlock *block = getBlockNoCreateNoEx(p3d);
	if(block == NULL)
		throw InvalidPositionException();
	return block;
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

bool Map::isValidPosition(v3s16 p)
{
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock *block = getBlockNoCreate(blockpos);
	return (block != NULL);
}

// Returns a CONTENT_IGNORE node if not found
MapNode Map::getNodeNoEx(v3s16 p)
{
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock *block = getBlockNoCreateNoEx(blockpos);
	if(block == NULL)
		return MapNode(CONTENT_IGNORE);
	v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
	return block->getNodeNoCheck(relpos);
}

// throws InvalidPositionException if not found
MapNode Map::getNode(v3s16 p)
{
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock *block = getBlockNoCreateNoEx(blockpos);
	if(block == NULL)
		throw InvalidPositionException();
	v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
	return block->getNodeNoCheck(relpos);
}

// throws InvalidPositionException if not found
void Map::setNode(v3s16 p, MapNode & n)
{
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock *block = getBlockNoCreate(blockpos);
	v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
	block->setNodeNoCheck(relpos, n);
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
			/*// Turn mud into grass
			if(n.d == CONTENT_MUD)
			{
				n.d = CONTENT_GRASS;
				block->setNode(relpos, n);
				modified_blocks.insert(blockpos, block);
			}*/

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

#if 1
	/*
		If the new node is solid and there is grass below, change it to mud
	*/
	if(content_features(n.d).walkable == true)
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
#endif

#if 0
	/*
		If the new node is mud and it is under sunlight, change it
		to grass
	*/
	if(n.d == CONTENT_MUD && node_under_sunlight)
	{
		n.d = CONTENT_GRASS;
	}
#endif

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

		core::list<MapBlock*> blocks;
		sector->getBlocks(blocks);
		for(core::list<MapBlock*>::Iterator i = blocks.begin();
				i != blocks.end(); i++)
		{
			(*i)->incrementUsageTimer(dtime);
		}
	}
}

void Map::deleteSectors(core::list<v2s16> &list, bool only_blocks)
{
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

u32 Map::unloadUnusedData(float timeout, bool only_blocks,
		core::list<v3s16> *deleted_blocks)
{
	core::list<v2s16> sector_deletion_queue;

	core::map<v2s16, MapSector*>::Iterator si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();

		bool all_blocks_deleted = true;

		core::list<MapBlock*> blocks;
		sector->getBlocks(blocks);
		for(core::list<MapBlock*>::Iterator i = blocks.begin();
				i != blocks.end(); i++)
		{
			MapBlock *block = (*i);

			if(block->getUsageTimer() > timeout)
			{
				// Save if modified
				if(block->getModified() != MOD_STATE_CLEAN)
					saveBlock(block);
				// Delete from memory
				sector->deleteBlock(block);
			}
			else
			{
				all_blocks_deleted = false;
			}
		}

		if(all_blocks_deleted)
		{
			sector_deletion_queue.push_back(si.getNode()->getKey());
		}
	}

#if 0
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
#endif

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

	//m_chunksize = 8; // Takes a few seconds

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
				}
				catch(FileNotGoodException &e){
					dstream<<DTIME<<"WARNING: Could not load map metadata"
							//<<" Disabling chunk-based generator."
							<<std::endl;
					//m_chunksize = 0;
				}

				/*try{
					// Load chunk metadata
					loadChunkMeta();
				}
				catch(FileNotGoodException &e){
					dstream<<DTIME<<"WARNING: Could not load chunk metadata."
							<<" Disabling chunk-based generator."
							<<std::endl;
					m_chunksize = 0;
				}*/

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

#if 0
	/*
		Free all MapChunks
	*/
	core::map<v2s16, MapChunk*>::Iterator i = m_chunks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapChunk *chunk = i.getNode()->getValue();
		delete chunk;
	}
#endif
}

void ServerMap::initBlockMake(mapgen::BlockMakeData *data, v3s16 blockpos)
{
	/*dstream<<"initBlockMake(): ("<<blockpos.X<<","<<blockpos.Y<<","
			<<blockpos.Z<<")"<<std::endl;*/

	data->no_op = false;
	data->seed = m_seed;
	data->blockpos = blockpos;

	/*
		Create the whole area of this and the neighboring blocks
	*/
	{
		//TimeTaker timer("initBlockMake() create area");
		
		for(s16 x=-1; x<=1; x++)
		for(s16 z=-1; z<=1; z++)
		{
			v2s16 sectorpos(blockpos.X+x, blockpos.Z+z);
			// Sector metadata is loaded from disk if not already loaded.
			ServerMapSector *sector = createSector(sectorpos);
			assert(sector);

			for(s16 y=-1; y<=1; y++)
			{
				MapBlock *block = createBlock(blockpos);

				// Lighting won't be calculated
				block->setLightingExpired(true);
				// Lighting will be calculated
				//block->setLightingExpired(false);

				/*
					Block gets sunlight if this is true.

					This should be set to true when the top side of a block
					is completely exposed to the sky.
				*/
				block->setIsUnderground(false);
			}
		}
	}
	
	/*
		Now we have a big empty area.

		Make a ManualMapVoxelManipulator that contains this and the
		neighboring blocks
	*/
	
	v3s16 bigarea_blocks_min = blockpos - v3s16(1,1,1);
	v3s16 bigarea_blocks_max = blockpos + v3s16(1,1,1);
	
	data->vmanip = new ManualMapVoxelManipulator(this);
	//data->vmanip->setMap(this);

	// Add the area
	{
		//TimeTaker timer("initBlockMake() initialEmerge");
		data->vmanip->initialEmerge(bigarea_blocks_min, bigarea_blocks_max);
	}

	// Data is ready now.
}

MapBlock* ServerMap::finishBlockMake(mapgen::BlockMakeData *data,
		core::map<v3s16, MapBlock*> &changed_blocks)
{
	v3s16 blockpos = data->blockpos;
	/*dstream<<"finishBlockMake(): ("<<blockpos.X<<","<<blockpos.Y<<","
			<<blockpos.Z<<")"<<std::endl;*/

	if(data->no_op)
	{
		dstream<<"finishBlockMake(): no-op"<<std::endl;
		return NULL;
	}

	/*dstream<<"Resulting vmanip:"<<std::endl;
	data->vmanip.print(dstream);*/
	
	/*
		Blit generated stuff to map
		NOTE: blitBackAll adds nearly everything to changed_blocks
	*/
	{
		// 70ms @cs=8
		//TimeTaker timer("finishBlockMake() blitBackAll");
		data->vmanip->blitBackAll(&changed_blocks);
	}
#if 1
	dstream<<"finishBlockMake: changed_blocks.size()="
			<<changed_blocks.size()<<std::endl;
#endif
	/*
		Copy transforming liquid information
	*/
	while(data->transforming_liquid.size() > 0)
	{
		v3s16 p = data->transforming_liquid.pop_front();
		m_transforming_liquid.push_back(p);
	}
	
	/*
		Get central block
	*/
	MapBlock *block = getBlockNoCreateNoEx(data->blockpos);
	assert(block);

	/*
		Set is_underground flag for lighting with sunlight
	*/

	block->setIsUnderground(mapgen::block_is_underground(data->seed, blockpos));

	/*
		Add sunlight to central block.
		This makes in-dark-spawning monsters to not flood the whole thing.
		Do not spread the light, though.
	*/
	/*core::map<v3s16, bool> light_sources;
	bool black_air_left = false;
	block->propagateSunlight(light_sources, true, &black_air_left);*/

	/*
		NOTE: Lighting and object adding shouldn't really be here, but
		lighting is a bit tricky to move properly to makeBlock.
		TODO: Do this the right way anyway.
	*/

	/*
		Update lighting
	*/
	{
		TimeTaker t("finishBlockMake lighting update");

		core::map<v3s16, MapBlock*> lighting_update_blocks;
		// Center block
		lighting_update_blocks.insert(block->getPos(), block);
	#if 0
		// All modified blocks
		for(core::map<v3s16, MapBlock*>::Iterator
				i = changed_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			lighting_update_blocks.insert(i.getNode()->getKey(),
					i.getNode()->getValue());
		}
	#endif
		updateLighting(lighting_update_blocks, changed_blocks);
	}

	/*
		Add random objects to block
	*/
	mapgen::add_random_objects(block);

	/*
		Go through changed blocks
	*/
	for(core::map<v3s16, MapBlock*>::Iterator i = changed_blocks.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();
		assert(block);
		/*
			Update day/night difference cache of the MapBlocks
		*/
		block->updateDayNightDiff();
		/*
			Set block as modified
		*/
		block->raiseModified(MOD_STATE_WRITE_NEEDED);
	}

	/*
		Set central block as generated
	*/
	block->setGenerated(true);
	
	/*
		Save changed parts of map
		NOTE: Will be saved later.
	*/
	//save(true);

	/*dstream<<"finishBlockMake() done for ("<<blockpos.X<<","<<blockpos.Y<<","
			<<blockpos.Z<<")"<<std::endl;*/
	
	return block;
}

ServerMapSector * ServerMap::createSector(v2s16 p2d)
{
	DSTACKF("%s: p2d=(%d,%d)",
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
	//if(loadSectorFull(p2d) == true)

	/*
		Try to load metadata from disk
	*/
	if(loadSectorMeta(p2d) == true)
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

/*
	This is a quick-hand function for calling makeBlock().
*/
MapBlock * ServerMap::generateBlock(
		v3s16 p,
		core::map<v3s16, MapBlock*> &modified_blocks
)
{
	DSTACKF("%s: p=(%d,%d,%d)", __FUNCTION_NAME, p.X, p.Y, p.Z);
	
	/*dstream<<"generateBlock(): "
			<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<std::endl;*/
	
	TimeTaker timer("generateBlock");
	
	//MapBlock *block = original_dummy;
			
	v2s16 p2d(p.X, p.Z);
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
		Create block make data
	*/
	mapgen::BlockMakeData data;
	initBlockMake(&data, p);

	/*
		Generate block
	*/
	{
		TimeTaker t("mapgen::make_block()");
		mapgen::make_block(&data);
	}

	/*
		Blit data back on map, update lighting, add mobs and whatever this does
	*/
	finishBlockMake(&data, modified_blocks);

	/*
		Get central block
	*/
	MapBlock *block = getBlockNoCreateNoEx(p);
	assert(block);

#if 0
	/*
		Check result
	*/
	bool erroneus_content = false;
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		v3s16 p(x0,y0,z0);
		MapNode n = block->getNode(p);
		if(n.d == CONTENT_IGNORE)
		{
			dstream<<"CONTENT_IGNORE at "
					<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<std::endl;
			erroneus_content = true;
			assert(0);
		}
	}
	if(erroneus_content)
	{
		assert(0);
	}
#endif

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
			if(y0%2==0)
				n.d = CONTENT_AIR;
			else
				n.d = CONTENT_STONE;
			block->setNode(v3s16(x0,y0,z0), n);
		}
	}
#endif

	return block;
}

MapBlock * ServerMap::createBlock(v3s16 p)
{
	DSTACKF("%s: p=(%d,%d,%d)",
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
	{
		if(block->isDummy())
			block->unDummify();
		return block;
	}
	// Create blank
	block = sector->createBlankBlock(block_y);
	return block;
}

#if 0
MapBlock * ServerMap::emergeBlock(
		v3s16 p,
		bool only_from_disk,
		core::map<v3s16, MapBlock*> &changed_blocks,
		core::map<v3s16, MapBlock*> &lighting_invalidated_blocks
)
{
	DSTACKF("%s: p=(%d,%d,%d), only_from_disk=%d",
			__FUNCTION_NAME,
			p.X, p.Y, p.Z, only_from_disk);
	
	// This has to be redone or removed
	assert(0);
	return NULL;
}
#endif

#if 0
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
		sector = createSector(p2d);
		//sector = emergeSector(p2d, changed_blocks);
	}
	catch(InvalidPositionException &e)
	{
		dstream<<"emergeBlock: createSector() failed: "
				<<e.what()<<std::endl;
		dstream<<"Path to failed sector: "<<getSectorDir(p2d)
				<<std::endl
				<<"You could try to delete it."<<std::endl;
		throw e;
	}
	catch(VersionMismatchException &e)
	{
		dstream<<"emergeBlock: createSector() failed: "
				<<e.what()<<std::endl;
		dstream<<"Path to failed sector: "<<getSectorDir(p2d)
				<<std::endl
				<<"You could try to delete it."<<std::endl;
		throw e;
	}

	/*
		Try to get a block from the sector
	*/

	bool does_not_exist = false;
	bool lighting_expired = false;
	MapBlock *block = sector->getBlockNoCreateNoEx(block_y);
	
	// If not found, try loading from disk
	if(block == NULL)
	{
		block = loadBlock(p);
	}
	
	// Handle result
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

#if 0
	/*
		Initially update sunlight
	*/
	{
		core::map<v3s16, bool> light_sources;
		bool black_air_left = false;
		bool bottom_invalid =
				block->propagateSunlight(light_sources, true,
				&black_air_left);

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
#endif
	
	return block;
}
#endif

s16 ServerMap::findGroundLevel(v2s16 p2d)
{
#if 0
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
#endif

	/*
		Determine from map generator noise functions
	*/
	
	s16 level = mapgen::find_ground_level_from_noise(m_seed, p2d, 1);
	return level;

	//double level = base_rock_level_2d(m_seed, p2d) + AVERAGE_MUD_AMOUNT;
	//return (s16)level;
}

void ServerMap::createDirs(std::string path)
{
	if(fs::CreateAllDirs(path) == false)
	{
		m_dout<<DTIME<<"ServerMap: Failed to create directory "
				<<"\""<<path<<"\""<<std::endl;
		throw BaseException("ServerMap failed to create directory");
	}
}

std::string ServerMap::getSectorDir(v2s16 pos, int layout)
{
	char cc[9];
	switch(layout)
	{
		case 1:
			snprintf(cc, 9, "%.4x%.4x",
				(unsigned int)pos.X&0xffff,
				(unsigned int)pos.Y&0xffff);

			return m_savedir + "/sectors/" + cc;
		case 2:
			snprintf(cc, 9, "%.3x/%.3x",
				(unsigned int)pos.X&0xfff,
				(unsigned int)pos.Y&0xfff);

			return m_savedir + "/sectors2/" + cc;
		default:
			assert(false);
	}
}

v2s16 ServerMap::getSectorPos(std::string dirname)
{
	unsigned int x, y;
	int r;
	size_t spos = dirname.rfind('/') + 1;
	assert(spos != std::string::npos);
	if(dirname.size() - spos == 8)
	{
		// Old layout
		r = sscanf(dirname.substr(spos).c_str(), "%4x%4x", &x, &y);
	}
	else if(dirname.size() - spos == 3)
	{
		// New layout
		r = sscanf(dirname.substr(spos-4).c_str(), "%3x/%3x", &x, &y);
		// Sign-extend the 12 bit values up to 16 bits...
		if(x&0x800) x|=0xF000;
		if(y&0x800) y|=0xF000;
	}
	else
	{
		assert(false);
	}
	assert(r == 2);
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

std::string ServerMap::getBlockFilename(v3s16 p)
{
	char cc[5];
	snprintf(cc, 5, "%.4x", (unsigned int)p.Y&0xffff);
	return cc;
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

	u32 sector_meta_count = 0;
	u32 block_count = 0;
	u32 block_count_all = 0; // Number of blocks in memory
	
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
			
			block_count_all++;

			if(block->getModified() >= MOD_STATE_WRITE_NEEDED 
					|| only_changed == false)
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

	/*
		Only print if something happened or saved whole map
	*/
	if(only_changed == false || sector_meta_count != 0
			|| block_count != 0)
	{
		dstream<<DTIME<<"ServerMap: Written: "
				<<sector_meta_count<<" sector metadata files, "
				<<block_count<<" block files"
				<<", "<<block_count_all<<" blocks in memory."
				<<std::endl;
	}
}

void ServerMap::saveMapMeta()
{
	DSTACK(__FUNCTION_NAME);
	
	dstream<<"INFO: ServerMap::saveMapMeta(): "
			<<"seed="<<m_seed
			<<std::endl;

	createDirs(m_savedir);
	
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

	dstream<<"INFO: ServerMap::loadMapMeta(): "
			<<"seed="<<m_seed
			<<std::endl;
}

void ServerMap::saveSectorMeta(ServerMapSector *sector)
{
	DSTACK(__FUNCTION_NAME);
	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST;
	// Get destination
	v2s16 pos = sector->getPos();
	std::string dir = getSectorDir(pos);
	createDirs(dir);
	
	std::string fullpath = dir + "/meta";
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open sector metafile");

	sector->serialize(o, version);
	
	sector->differs_from_disk = false;
}

MapSector* ServerMap::loadSectorMeta(std::string sectordir, bool save_after_load)
{
	DSTACK(__FUNCTION_NAME);
	// Get destination
	v2s16 p2d = getSectorPos(sectordir);

	ServerMapSector *sector = NULL;

	std::string fullpath = sectordir + "/meta";
	std::ifstream is(fullpath.c_str(), std::ios_base::binary);
	if(is.good() == false)
	{
		// If the directory exists anyway, it probably is in some old
		// format. Just go ahead and create the sector.
		if(fs::PathExists(sectordir))
		{
			dstream<<"ServerMap::loadSectorMeta(): Sector metafile "
					<<fullpath<<" doesn't exist but directory does."
					<<" Continuing with a sector with no metadata."
					<<std::endl;
			sector = new ServerMapSector(this, p2d);
			m_sectors.insert(p2d, sector);
		}
		else
		{
			throw FileNotGoodException("Cannot open sector metafile");
		}
	}
	else
	{
		sector = ServerMapSector::deSerialize
				(is, this, p2d, m_sectors);
		if(save_after_load)
			saveSectorMeta(sector);
	}
	
	sector->differs_from_disk = false;

	return sector;
}

bool ServerMap::loadSectorMeta(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);

	MapSector *sector = NULL;

	// The directory layout we're going to load from.
	//  1 - original sectors/xxxxzzzz/
	//  2 - new sectors2/xxx/zzz/
	//  If we load from anything but the latest structure, we will
	//  immediately save to the new one, and remove the old.
	int loadlayout = 1;
	std::string sectordir1 = getSectorDir(p2d, 1);
	std::string sectordir;
	if(fs::PathExists(sectordir1))
	{
		sectordir = sectordir1;
	}
	else
	{
		loadlayout = 2;
		sectordir = getSectorDir(p2d, 2);
	}

	try{
		sector = loadSectorMeta(sectordir, loadlayout != 2);
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
	
	return true;
}

#if 0
bool ServerMap::loadSectorFull(v2s16 p2d)
{
	DSTACK(__FUNCTION_NAME);

	MapSector *sector = NULL;

	// The directory layout we're going to load from.
	//  1 - original sectors/xxxxzzzz/
	//  2 - new sectors2/xxx/zzz/
	//  If we load from anything but the latest structure, we will
	//  immediately save to the new one, and remove the old.
	int loadlayout = 1;
	std::string sectordir1 = getSectorDir(p2d, 1);
	std::string sectordir;
	if(fs::PathExists(sectordir1))
	{
		sectordir = sectordir1;
	}
	else
	{
		loadlayout = 2;
		sectordir = getSectorDir(p2d, 2);
	}

	try{
		sector = loadSectorMeta(sectordir, loadlayout != 2);
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
			(sectordir);
	std::vector<fs::DirListNode>::iterator i2;
	for(i2=list2.begin(); i2!=list2.end(); i2++)
	{
		// We want files
		if(i2->dir)
			continue;
		try{
			loadBlock(sectordir, i2->name, sector, loadlayout != 2);
		}
		catch(InvalidFilenameException &e)
		{
			// This catches unknown crap in directory
		}
	}

	if(loadlayout != 2)
	{
		dstream<<"Sector converted to new layout - deleting "<<
			sectordir1<<std::endl;
		fs::RecursiveDelete(sectordir1);
	}

	return true;
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
	std::string sectordir = getSectorDir(p2d);

	createDirs(sectordir);

	std::string fullpath = sectordir+"/"+getBlockFilename(p3d);
	std::ofstream o(fullpath.c_str(), std::ios_base::binary);
	if(o.good() == false)
		throw FileNotGoodException("Cannot open block data");

	/*
		[0] u8 serialization version
		[1] data
	*/
	o.write((char*)&version, 1);
	
	// Write basic data
	block->serialize(o, version);
	
	// Write extra data stored on disk
	block->serializeDiskExtra(o, version);

	// We just wrote it to the disk so clear modified flag
	block->resetModified();
}

void ServerMap::loadBlock(std::string sectordir, std::string blockfile, MapSector *sector, bool save_after_load)
{
	DSTACK(__FUNCTION_NAME);

	std::string fullpath = sectordir+"/"+blockfile;
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
		block = sector->getBlockNoCreateNoEx(p3d.Y);
		if(block == NULL)
		{
			block = sector->createBlankBlockNoInsert(p3d.Y);
			created_new = true;
		}
		
		// Read basic data
		block->deSerialize(is, version);

		// Read extra data stored on disk
		block->deSerializeDiskExtra(is, version);
		
		// If it's a new block, insert it to the map
		if(created_new)
			sector->insertBlock(block);
		
		/*
			Save blocks loaded in old format in new format
		*/

		if(version < SER_FMT_VER_HIGHEST || save_after_load)
		{
			saveBlock(block);
		}
		
		// We just loaded it from the disk, so it's up-to-date.
		block->resetModified();

	}
	catch(SerializationError &e)
	{
		dstream<<"WARNING: Invalid block data on disk "
				<<"fullpath="<<fullpath
				<<" (SerializationError). "
				<<"what()="<<e.what()
				<<std::endl;
				//" Ignoring. A new one will be generated.
		assert(0);

		// TODO: Backup file; name is in fullpath.
	}
}

MapBlock* ServerMap::loadBlock(v3s16 blockpos)
{
	DSTACK(__FUNCTION_NAME);

	v2s16 p2d(blockpos.X, blockpos.Z);

	// The directory layout we're going to load from.
	//  1 - original sectors/xxxxzzzz/
	//  2 - new sectors2/xxx/zzz/
	//  If we load from anything but the latest structure, we will
	//  immediately save to the new one, and remove the old.
	int loadlayout = 1;
	std::string sectordir1 = getSectorDir(p2d, 1);
	std::string sectordir;
	if(fs::PathExists(sectordir1))
	{
		sectordir = sectordir1;
	}
	else
	{
		loadlayout = 2;
		sectordir = getSectorDir(p2d, 2);
	}
	
	/*
		Make sure sector is loaded
	*/
	MapSector *sector = getSectorNoGenerateNoEx(p2d);
	if(sector == NULL)
	{
		try{
			sector = loadSectorMeta(sectordir, loadlayout != 2);
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
	}
	
	/*
		Make sure file exists
	*/

	std::string blockfilename = getBlockFilename(blockpos);
	if(fs::PathExists(sectordir+"/"+blockfilename) == false)
		return NULL;

	/*
		Load block
	*/
	loadBlock(sectordir, blockfilename, sector, loadlayout != 2);
	return getBlockNoCreateNoEx(blockpos);
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

#if 0
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
#endif

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
		This is called two times per frame, reset on the non-transparent one
	*/
	if(pass == scene::ESNRP_SOLID)
	{
		m_last_drawn_sectors.clear();
	}

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
	v3s16 p_blocks_min(
			p_nodes_min.X / MAP_BLOCKSIZE - 1,
			p_nodes_min.Y / MAP_BLOCKSIZE - 1,
			p_nodes_min.Z / MAP_BLOCKSIZE - 1);
	v3s16 p_blocks_max(
			p_nodes_max.X / MAP_BLOCKSIZE,
			p_nodes_max.Y / MAP_BLOCKSIZE,
			p_nodes_max.Z / MAP_BLOCKSIZE);
	
	u32 vertex_count = 0;
	
	// For limiting number of mesh updates per frame
	u32 mesh_update_count = 0;
	
	u32 blocks_would_have_drawn = 0;
	u32 blocks_drawn = 0;

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
		
		u32 sector_blocks_drawn = 0;

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
				sector_blocks_drawn++;

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

		if(sector_blocks_drawn != 0)
		{
			m_last_drawn_sectors[sp] = true;
		}
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
