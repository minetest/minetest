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
#include <malloc.h>

#ifdef _WIN32
	#include <windows.h>
	#define sleep_s(x) Sleep((x*1000))
#else
	#include <unistd.h>
	#define sleep_s(x) sleep(x)
#endif

#include "common_irrlicht.h"
#include "heightmap.h"
#include "mapnode.h"
#include "mapblock.h"
#include "mapsector.h"
#include "constants.h"
#include "voxel.h"

class Map;

#if 0
/*
	A cache for short-term fast access to map data

	NOTE: This doesn't really make anything more efficient
	NOTE: Use VoxelManipulator, if possible
	TODO: Get rid of this?
	NOTE: CONFIRMED: THIS CACHE DOESN'T MAKE ANYTHING ANY FASTER
*/
class MapBlockPointerCache : public NodeContainer
{
public:
	MapBlockPointerCache(Map *map);
	~MapBlockPointerCache();

	virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_MAPBLOCKCACHE;
	}

	MapBlock * getBlockNoCreate(v3s16 p);

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
	}
	
	// virtual from NodeContainer
	MapNode getNode(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlockNoCreate(blockpos);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;

		return blockref->getNodeNoCheck(relpos);
	}

	// virtual from NodeContainer
	void setNode(v3s16 p, MapNode & n)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * block = getBlockNoCreate(blockpos);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		block->setNodeNoCheck(relpos, n);
		m_modified_blocks[blockpos] = block;
	}

	core::map<v3s16, MapBlock*> m_modified_blocks;
	
private:
	Map *m_map;
	core::map<v3s16, MapBlock*> m_blocks;

	u32 m_from_cache_count;
	u32 m_from_map_count;
};
#endif

class CacheLock
{
public:
	CacheLock()
	{
		m_count = 0;
		m_count_mutex.Init();
		m_cache_mutex.Init();
		m_waitcache_mutex.Init();
	}

	void cacheCreated()
	{
		//dstream<<"cacheCreated() begin"<<std::endl;
		JMutexAutoLock waitcachelock(m_waitcache_mutex);
		JMutexAutoLock countlock(m_count_mutex);

		// If this is the first cache, grab the cache lock
		if(m_count == 0)
			m_cache_mutex.Lock();
			
		m_count++;

		//dstream<<"cacheCreated() end"<<std::endl;
	}

	void cacheRemoved()
	{
		//dstream<<"cacheRemoved() begin"<<std::endl;
		JMutexAutoLock countlock(m_count_mutex);

		assert(m_count > 0);

		m_count--;
		
		// If this is the last one, release the cache lock
		if(m_count == 0)
			m_cache_mutex.Unlock();

		//dstream<<"cacheRemoved() end"<<std::endl;
	}

	/*
		This lock should be taken when removing stuff that can be
		pointed by the cache.

		You'll want to grab this in a SharedPtr.
	*/
	JMutexAutoLock * waitCaches()
	{
		//dstream<<"waitCaches() begin"<<std::endl;
		JMutexAutoLock waitcachelock(m_waitcache_mutex);
		JMutexAutoLock *lock = new JMutexAutoLock(m_cache_mutex);
		//dstream<<"waitCaches() end"<<std::endl;
		return lock;
	}

private:
	// Count of existing caches
	u32 m_count;
	JMutex m_count_mutex;
	// This is locked always when there are some caches
	JMutex m_cache_mutex;
	// Locked so that when waitCaches() is called, no more caches are created
	JMutex m_waitcache_mutex;
};

#define MAPTYPE_BASE 0
#define MAPTYPE_SERVER 1
#define MAPTYPE_CLIENT 2

class Map : public NodeContainer, public Heightmappish
{
protected:

	std::ostream &m_dout;

	core::map<v2s16, MapSector*> m_sectors;
	JMutex m_sector_mutex;

	v3f m_camera_position;
	v3f m_camera_direction;
	JMutex m_camera_mutex;

	// Be sure to set this to NULL when the cached sector is deleted 
	MapSector *m_sector_cache;
	v2s16 m_sector_cache_p;

	WrapperHeightmap m_hwrapper;

public:

	v3s16 drawoffset; // for drawbox()
	
	/*
		Used by MapBlockPointerCache.

		waitCaches() can be called to remove all caches before continuing
	*/
	CacheLock m_blockcachelock;

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

	virtual void drop()
	{
		delete this;
	}

	void updateCamera(v3f pos, v3f dir)
	{
		JMutexAutoLock lock(m_camera_mutex);
		m_camera_position = pos;
		m_camera_direction = dir;
	}

	/*void StartUpdater()
	{
		updater.Start();
	}

	void StopUpdater()
	{
		updater.setRun(false);
		while(updater.IsRunning())
			sleep_s(1);
	}

	bool UpdaterIsRunning()
	{
		return updater.IsRunning();
	}*/

	static core::aabbox3d<f32> getNodeBox(v3s16 p)
	{
		return core::aabbox3d<f32>(
			(float)p.X * BS - 0.5*BS,
			(float)p.Y * BS - 0.5*BS,
			(float)p.Z * BS - 0.5*BS,
			(float)p.X * BS + 0.5*BS,
			(float)p.Y * BS + 0.5*BS,
			(float)p.Z * BS + 0.5*BS
		);
	}

	//bool sectorExists(v2s16 p);
	MapSector * getSectorNoGenerate(v2s16 p2d);
	/*
		This is overloaded by ClientMap and ServerMap to allow
		their differing fetch methods.
	*/
	virtual MapSector * emergeSector(v2s16 p) = 0;
	
	// Returns InvalidPositionException if not found
	MapBlock * getBlockNoCreate(v3s16 p);
	// Returns NULL if not found
	MapBlock * getBlockNoCreateNoEx(v3s16 p);
	
	// Returns InvalidPositionException if not found
	f32 getGroundHeight(v2s16 p, bool generate=false);
	void setGroundHeight(v2s16 p, f32 y, bool generate=false);

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

	/*MapNode getNodeGenerate(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlock(blockpos);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;

		return blockref->getNode(relpos);
	}*/

	/*void setNodeGenerate(v3s16 p, MapNode & n)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlock(blockpos);
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		blockref->setNode(relpos, n);
	}*/

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
	
#ifndef SERVER
	void expireMeshes(bool only_daynight_diffed);
	
	/*
		Updates the faces of the given block and blocks on the
		leading edge.
	*/
	void updateMeshes(v3s16 blockpos, u32 daynight_ratio);
#endif

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
};

// Master heightmap parameters
struct HMParams
{
	HMParams()
	{
		blocksize = 64;
		randmax = "constant 70.0";
		randfactor = "constant 0.6";
		base = "linear 0 80 0";
	}
	s16 blocksize;
	std::string randmax;
	std::string randfactor;
	std::string base;
};

// Map parameters
struct MapParams
{
	MapParams()
	{
		plants_amount = 1.0;
		ravines_amount = 1.0;
		//max_objects_in_block = 30;
	}
	float plants_amount;
	float ravines_amount;
	//u16 max_objects_in_block;
};

class ServerMap : public Map
{
public:
	/*
		savedir: directory to which map data should be saved
	*/
	ServerMap(std::string savedir, HMParams hmp, MapParams mp);
	~ServerMap();

	s32 mapType() const
	{
		return MAPTYPE_SERVER;
	}

	/*
		Forcefully get a sector from somewhere
	*/
	MapSector * emergeSector(v2s16 p);
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

	void saveMasterHeightmap();
	void loadMasterHeightmap();

	// The sector mutex should be locked when calling most of these
	
	// This only saves sector-specific data such as the heightmap
	// (no MapBlocks)
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

	// Gets from master heightmap
	void getSectorCorners(v2s16 p2d, s16 *corners);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);

private:
	UnlimitedHeightmap *m_heightmap;
	MapParams m_params;

	std::string m_savedir;
	bool m_map_saving_enabled;
};

#ifndef SERVER

class Client;

class ClientMap : public Map, public scene::ISceneNode
{
public:
	ClientMap(
			Client *client,
			JMutex &range_mutex,
			s16 &viewing_range_nodes,
			bool &viewing_range_all,
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
		Return value is position of changed block.
	*/
	v3s16 setTempMod(v3s16 p, NodeMod mod);
	v3s16 clearTempMod(v3s16 p);
	// Efficient implementation needs a cache of TempMods
	//void clearTempMods();

	// For debug printing
	virtual void PrintInfo(std::ostream &out);
	
private:
	Client *m_client;
	
	core::aabbox3d<f32> m_box;
	
	// This is the master heightmap mesh
	scene::SMesh *mesh;
	JMutex mesh_mutex;

	JMutex &m_range_mutex;
	s16 &m_viewing_range_nodes;
	bool &m_viewing_range_all;
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

private:
	Map *m_map;
	/*
		NOTE: This might be used or not
		bool is dummy value
		SUGG: How 'bout an another VoxelManipulator for storing the
		      information about which block is loaded?
	*/
	core::map<v3s16, bool> m_loaded_blocks;
};

#endif

