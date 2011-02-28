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

#ifndef MAP_HEADER
#define MAP_HEADER

#include <jmutex.h>
#include <jthread.h>
#include <iostream>

#ifdef _WIN32
	#include <windows.h>
	#define sleep_s(x) Sleep((x*1000))
#else
	#include <unistd.h>
	#define sleep_s(x) sleep(x)
#endif

#include "common_irrlicht.h"
#include "mapnode.h"
#include "mapblock.h"
#include "mapsector.h"
#include "constants.h"
#include "voxel.h"
#include "mapchunk.h"

/*
	Some exposed functions
*/

double base_rock_level_2d(u64 seed, v2f p);

/*
*/

#define MAPTYPE_BASE 0
#define MAPTYPE_SERVER 1
#define MAPTYPE_CLIENT 2

enum MapEditEventType{
	MEET_ADDNODE,
	MEET_REMOVENODE,
	MEET_OTHER
};

struct MapEditEvent
{
	MapEditEventType type;
	v3s16 p;
	MapNode n;
	core::map<v3s16, bool> modified_blocks;
	u16 already_known_by_peer;

	MapEditEvent():
		type(MEET_OTHER),
		already_known_by_peer(0)
	{
	}
	
	MapEditEvent * clone()
	{
		MapEditEvent *event = new MapEditEvent();
		event->type = type;
		event->p = p;
		event->n = n;
		for(core::map<v3s16, bool>::Iterator
				i = modified_blocks.getIterator();
				i.atEnd()==false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			bool v = i.getNode()->getValue();
			event->modified_blocks.insert(p, v);
		}
		return event;
	}
};

class MapEventReceiver
{
public:
	// event shall be deleted by caller after the call.
	virtual void onMapEditEvent(MapEditEvent *event) = 0;
};

class Map : public NodeContainer
{
public:

	Map(std::ostream &dout);
	virtual ~Map();

	virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_MAP;
	}

	virtual s32 mapType() const
	{
		return MAPTYPE_BASE;
	}
	
	/*
		Drop (client) or delete (server) the map.
	*/
	virtual void drop()
	{
		delete this;
	}

	void addEventReceiver(MapEventReceiver *event_receiver);
	void removeEventReceiver(MapEventReceiver *event_receiver);
	// event shall be deleted by caller after the call.
	void dispatchEvent(MapEditEvent *event);

	// On failure returns NULL
	MapSector * getSectorNoGenerateNoExNoLock(v2s16 p2d);
	// On failure returns NULL
	MapSector * getSectorNoGenerateNoEx(v2s16 p2d);
	// On failure throws InvalidPositionException
	MapSector * getSectorNoGenerate(v2s16 p2d);
	// Gets an existing sector or creates an empty one
	//MapSector * getSectorCreate(v2s16 p2d);

	/*
		This is overloaded by ClientMap and ServerMap to allow
		their differing fetch methods.
	*/
	virtual MapSector * emergeSector(v2s16 p){ return NULL; }
	virtual MapSector * emergeSector(v2s16 p,
			core::map<v3s16, MapBlock*> &changed_blocks){ return NULL; }

	// Returns InvalidPositionException if not found
	MapBlock * getBlockNoCreate(v3s16 p);
	// Returns NULL if not found
	MapBlock * getBlockNoCreateNoEx(v3s16 p);
	// Gets an existing block or creates an empty one
	//MapBlock * getBlockCreate(v3s16 p);
	
	// Returns InvalidPositionException if not found
	bool isNodeUnderground(v3s16 p);
	
	// virtual from NodeContainer
	bool isValidPosition(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock *blockref;
		try{
			blockref = getBlockNoCreate(blockpos);
		}
		catch(InvalidPositionException &e)
		{
			return false;
		}
		return true;
		/*v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		bool is_valid = blockref->isValidPosition(relpos);
		return is_valid;*/
	}
	
	// virtual from NodeContainer
	// throws InvalidPositionException if not found
	MapNode getNode(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlockNoCreate(blockpos);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;

		return blockref->getNodeNoCheck(relpos);
	}

	// virtual from NodeContainer
	// throws InvalidPositionException if not found
	void setNode(v3s16 p, MapNode & n)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlockNoCreate(blockpos);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		blockref->setNodeNoCheck(relpos, n);
	}
	
	// Returns a CONTENT_IGNORE node if not found
	MapNode getNodeNoEx(v3s16 p)
	{
		try{
			v3s16 blockpos = getNodeBlockPos(p);
			MapBlock * blockref = getBlockNoCreate(blockpos);
			v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;

			return blockref->getNodeNoCheck(relpos);
		}
		catch(InvalidPositionException &e)
		{
			return MapNode(CONTENT_IGNORE);
		}
	}

	void unspreadLight(enum LightBank bank,
			core::map<v3s16, u8> & from_nodes,
			core::map<v3s16, bool> & light_sources,
			core::map<v3s16, MapBlock*> & modified_blocks);

	void unLightNeighbors(enum LightBank bank,
			v3s16 pos, u8 lightwas,
			core::map<v3s16, bool> & light_sources,
			core::map<v3s16, MapBlock*> & modified_blocks);
	
	void spreadLight(enum LightBank bank,
			core::map<v3s16, bool> & from_nodes,
			core::map<v3s16, MapBlock*> & modified_blocks);
	
	void lightNeighbors(enum LightBank bank,
			v3s16 pos,
			core::map<v3s16, MapBlock*> & modified_blocks);

	v3s16 getBrightestNeighbour(enum LightBank bank, v3s16 p);

	s16 propagateSunlight(v3s16 start,
			core::map<v3s16, MapBlock*> & modified_blocks);
	
	void updateLighting(enum LightBank bank,
			core::map<v3s16, MapBlock*>  & a_blocks,
			core::map<v3s16, MapBlock*> & modified_blocks);
			
	void updateLighting(core::map<v3s16, MapBlock*>  & a_blocks,
			core::map<v3s16, MapBlock*> & modified_blocks);
			
	/*
		These handle lighting but not faces.
	*/
	void addNodeAndUpdate(v3s16 p, MapNode n,
			core::map<v3s16, MapBlock*> &modified_blocks);
	void removeNodeAndUpdate(v3s16 p,
			core::map<v3s16, MapBlock*> &modified_blocks);

	/*
		Wrappers for the latter ones.
		These emit events.
		Return true if succeeded, false if not.
	*/
	bool addNodeWithEvent(v3s16 p, MapNode n);
	bool removeNodeWithEvent(v3s16 p);
	
	/*
		Takes the blocks at the edges into account
	*/
	bool dayNightDiffed(v3s16 blockpos);

	//core::aabbox3d<s16> getDisplayedBlockArea();

	//bool updateChangedVisibleArea();
	
	virtual void save(bool only_changed){assert(0);};

	/*
		Updates usage timers
	*/
	void timerUpdate(float dtime);
	
	// Takes cache into account
	// sector mutex should be locked when calling
	void deleteSectors(core::list<v2s16> &list, bool only_blocks);
	
	// Returns count of deleted sectors
	u32 deleteUnusedSectors(float timeout, bool only_blocks=false,
			core::list<v3s16> *deleted_blocks=NULL);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);
	
	void transformLiquids(core::map<v3s16, MapBlock*> & modified_blocks);

	/*
		Variables
	*/
	
protected:

	std::ostream &m_dout;

	core::map<MapEventReceiver*, bool> m_event_receivers;
	
	// Mutex is important because on client map is accessed asynchronously
	core::map<v2s16, MapSector*> m_sectors;
	JMutex m_sector_mutex;

	// Be sure to set this to NULL when the cached sector is deleted 
	MapSector *m_sector_cache;
	v2s16 m_sector_cache_p;

	//WrapperHeightmap m_hwrapper;
	
	// Queued transforming water nodes
	UniqueQueue<v3s16> m_transforming_liquid;
};

/*
	ServerMap

	This is the only map class that is able to generate map.
*/

class ServerMap : public Map
{
public:
	/*
		savedir: directory to which map data should be saved
	*/
	ServerMap(std::string savedir);
	~ServerMap();

	s32 mapType() const
	{
		return MAPTYPE_SERVER;
	}

	/*
		Map generation
	*/
	
	// Returns the position of the chunk where the sector is in
	v2s16 sector_to_chunk(v2s16 sectorpos)
	{
		sectorpos.X += m_chunksize / 2;
		sectorpos.Y += m_chunksize / 2;
		v2s16 chunkpos = getContainerPos(sectorpos, m_chunksize);
		return chunkpos;
	}
	
	// Returns the position of the (0,0) sector of the chunk
	v2s16 chunk_to_sector(v2s16 chunkpos)
	{
		v2s16 sectorpos(
			chunkpos.X * m_chunksize,
			chunkpos.Y * m_chunksize
		);
		sectorpos.X -= m_chunksize / 2;
		sectorpos.Y -= m_chunksize / 2;
		return sectorpos;
	}

	/*
		Get a chunk.
	*/
	MapChunk *getChunk(v2s16 chunkpos)
	{
		core::map<v2s16, MapChunk*>::Node *n;
		n = m_chunks.find(chunkpos);
		if(n == NULL)
			return NULL;
		return n->getValue();
	}

	/*
		True if the chunk and its neighbors are fully generated.
		It means the chunk will not be touched in the future by the
		generator. If false, generateChunk will make it true.
	*/
	bool chunkNonVolatile(v2s16 chunkpos)
	{
		/*for(s16 x=-1; x<=1; x++)
		for(s16 y=-1; y<=1; y++)*/
		s16 x=0;
		s16 y=0;
		{
			v2s16 chunkpos0 = chunkpos + v2s16(x,y);
			MapChunk *chunk = getChunk(chunkpos);
			if(chunk == NULL)
				return false;
			if(chunk->getGenLevel() != GENERATED_FULLY)
				return false;
		}
		return true;
	}

	/*
		Generate a chunk.

		All chunks touching this one can be altered also.
	*/
	MapChunk* generateChunkRaw(v2s16 chunkpos,
			core::map<v3s16, MapBlock*> &changed_blocks,
			bool force=false);
	
	/*
		Generate a chunk and its neighbors so that it won't be touched
		anymore.
	*/
	MapChunk* generateChunk(v2s16 chunkpos,
			core::map<v3s16, MapBlock*> &changed_blocks);
	
	/*
		Generate a sector.
		
		This is mainly called by generateChunkRaw.
	*/
	//ServerMapSector * generateSector(v2s16 p);
	
	/*
		Get a sector from somewhere.
		- Check memory
		- Check disk (loads blocks also)
		- Create blank one
	*/
	ServerMapSector * createSector(v2s16 p);

	/*
		Get a sector from somewhere.
		- Check memory
		- Check disk (loads blocks also)
		- Generate chunk
	*/
	MapSector * emergeSector(v2s16 p,
			core::map<v3s16, MapBlock*> &changed_blocks);
	
	MapSector * emergeSector(v2s16 p)
	{
		core::map<v3s16, MapBlock*> changed_blocks;
		return emergeSector(p, changed_blocks);
	}

	MapBlock * generateBlock(
			v3s16 p,
			MapBlock *original_dummy,
			ServerMapSector *sector,
			core::map<v3s16, MapBlock*> &changed_blocks,
			core::map<v3s16, MapBlock*> &lighting_invalidated_blocks
	);
	
	/*
		Get a block from somewhere.
		- Memory
		- Create blank
	*/
	MapBlock * createBlock(v3s16 p);
	
	/*
		only_from_disk, changed_blocks and lighting_invalidated_blocks
		are not properly used by the new map generator.
	*/
	MapBlock * emergeBlock(
			v3s16 p,
			bool only_from_disk,
			core::map<v3s16, MapBlock*> &changed_blocks,
			core::map<v3s16, MapBlock*> &lighting_invalidated_blocks
	);

#if 0
	/*
		Forcefully get a block from somewhere.

		Exceptions:
		- InvalidPositionException: possible if only_from_disk==true
		
		changed_blocks:
		- All already existing blocks that were modified are added.
			- If found on disk, nothing will be added.
			- If generated, the new block will not be included.

		lighting_invalidated_blocks:
		- All blocks that have heavy-to-calculate lighting changes
		  are added.
			- updateLighting() should be called for these.
		
		- A block that is in changed_blocks may not be in
		  lighting_invalidated_blocks.
	*/
	MapBlock * emergeBlock(
			v3s16 p,
			bool only_from_disk,
			core::map<v3s16, MapBlock*> &changed_blocks,
			core::map<v3s16, MapBlock*> &lighting_invalidated_blocks
	);
#endif
	
	// Helper for placing objects on ground level
	s16 findGroundLevel(v2s16 p2d);

	/*
		Misc. helper functions for fiddling with directory and file
		names when saving
	*/
	void createDir(std::string path);
	void createSaveDir();
	// returns something like "xxxxxxxx"
	std::string getSectorSubDir(v2s16 pos);
	// returns something like "map/sectors/xxxxxxxx"
	std::string getSectorDir(v2s16 pos);
	std::string createSectorDir(v2s16 pos);
	// dirname: final directory name
	v2s16 getSectorPos(std::string dirname);
	v3s16 getBlockPos(std::string sectordir, std::string blockfile);

	void save(bool only_changed);
	void loadAll();
	
	// Saves map seed and possibly other stuff
	void saveMapMeta();
	void loadMapMeta();
	
	void saveChunkMeta();
	void loadChunkMeta();
	
	// The sector mutex should be locked when calling most of these
	
	// This only saves sector-specific data such as the heightmap
	// (no MapBlocks)
	// DEPRECATED? Sectors have no metadata anymore.
	void saveSectorMeta(ServerMapSector *sector);
	MapSector* loadSectorMeta(std::string dirname);
	
	// Full load of a sector including all blocks.
	// returns true on success, false on failure.
	bool loadSectorFull(v2s16 p2d);
	// If sector is not found in memory, try to load it from disk.
	// Returns true if sector now resides in memory
	//bool deFlushSector(v2s16 p2d);
	
	void saveBlock(MapBlock *block);
	// This will generate a sector with getSector if not found.
	void loadBlock(std::string sectordir, std::string blockfile, MapSector *sector);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);

	bool isSavingEnabled(){ return m_map_saving_enabled; }

	u64 getSeed(){ return m_seed; }

private:
	// Seed used for all kinds of randomness
	u64 m_seed;

	std::string m_savedir;
	bool m_map_saving_enabled;

	// Chunk size in MapSectors
	s16 m_chunksize;
	// Chunks
	core::map<v2s16, MapChunk*> m_chunks;
};

/*
	ClientMap stuff
*/

#ifndef SERVER

struct MapDrawControl
{
	MapDrawControl():
		range_all(false),
		wanted_range(50),
		wanted_max_blocks(0),
		wanted_min_range(0),
		blocks_drawn(0),
		blocks_would_have_drawn(0)
	{
	}
	// Overrides limits by drawing everything
	bool range_all;
	// Wanted drawing range
	float wanted_range;
	// Maximum number of blocks to draw
	u32 wanted_max_blocks;
	// Blocks in this range are drawn regardless of number of blocks drawn
	float wanted_min_range;
	// Number of blocks rendered is written here by the renderer
	u32 blocks_drawn;
	// Number of blocks that would have been drawn in wanted_range
	u32 blocks_would_have_drawn;
};

class Client;

/*
	ClientMap
	
	This is the only map class that is able to render itself on screen.
*/

class ClientMap : public Map, public scene::ISceneNode
{
public:
	ClientMap(
			Client *client,
			MapDrawControl &control,
			scene::ISceneNode* parent,
			scene::ISceneManager* mgr,
			s32 id
	);

	~ClientMap();

	s32 mapType() const
	{
		return MAPTYPE_CLIENT;
	}

	void drop()
	{
		ISceneNode::drop();
	}

	void updateCamera(v3f pos, v3f dir)
	{
		JMutexAutoLock lock(m_camera_mutex);
		m_camera_position = pos;
		m_camera_direction = dir;
	}

	/*
		Forcefully get a sector from somewhere
	*/
	MapSector * emergeSector(v2s16 p);

	void deSerializeSector(v2s16 p2d, std::istream &is);

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode();

	virtual void render()
	{
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		renderMap(driver, SceneManager->getSceneNodeRenderPass());
	}
	
	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return m_box;
	}

	void renderMap(video::IVideoDriver* driver, s32 pass);

	/*
		Methods for setting temporary modifications to nodes for
		drawing.

		Returns true if something changed.
		
		All blocks whose mesh could have been changed are inserted
		to affected_blocks.
	*/
	bool setTempMod(v3s16 p, NodeMod mod,
			core::map<v3s16, MapBlock*> *affected_blocks=NULL);
	bool clearTempMod(v3s16 p,
			core::map<v3s16, MapBlock*> *affected_blocks=NULL);
	// Efficient implementation needs a cache of TempMods
	//void clearTempMods();

	void expireMeshes(bool only_daynight_diffed);
	
	/*
		Update the faces of the given block and blocks on the
		leading edge.
	*/
	void updateMeshes(v3s16 blockpos, u32 daynight_ratio);
	
	// Update meshes that touch the node
	//void updateNodeMeshes(v3s16 nodepos, u32 daynight_ratio);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);
	
private:
	Client *m_client;
	
	core::aabbox3d<f32> m_box;
	
	// This is the master heightmap mesh
	//scene::SMesh *mesh;
	//JMutex mesh_mutex;
	
	MapDrawControl &m_control;

	v3f m_camera_position;
	v3f m_camera_direction;
	JMutex m_camera_mutex;

};

#endif

class MapVoxelManipulator : public VoxelManipulator
{
public:
	MapVoxelManipulator(Map *map);
	virtual ~MapVoxelManipulator();
	
	virtual void clear()
	{
		VoxelManipulator::clear();
		m_loaded_blocks.clear();
	}

	virtual void emerge(VoxelArea a, s32 caller_id=-1);

	void blitBack(core::map<v3s16, MapBlock*> & modified_blocks);

protected:
	Map *m_map;
	/*
		key = blockpos
		value = block existed when loaded
	*/
	core::map<v3s16, bool> m_loaded_blocks;
};

class ManualMapVoxelManipulator : public MapVoxelManipulator
{
public:
	ManualMapVoxelManipulator(Map *map);
	virtual ~ManualMapVoxelManipulator();
	
	virtual void emerge(VoxelArea a, s32 caller_id=-1);

	void initialEmerge(v3s16 blockpos_min, v3s16 blockpos_max);
	
	// This is much faster with big chunks of generated data
	void blitBackAll(core::map<v3s16, MapBlock*> * modified_blocks);

protected:
	bool m_create_area;
};

#endif

