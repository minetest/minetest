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

#ifndef MAP_HEADER
#define MAP_HEADER

#include <jmutex.h>
#include <jmutexautolock.h>
#include <jthread.h>
#include <iostream>
#include <sstream>

#include "common_irrlicht.h"
#include "mapnode.h"
#include "mapblock_nodemod.h"
#include "constants.h"
#include "voxel.h"

extern "C" {
	#include "sqlite3.h"
}

class MapSector;
class ServerMapSector;
class ClientMapSector;
class MapBlock;
class NodeMetadata;

namespace mapgen{
	struct BlockMakeData;
};

/*
	MapEditEvent
*/

#define MAPTYPE_BASE 0
#define MAPTYPE_SERVER 1
#define MAPTYPE_CLIENT 2

enum MapEditEventType{
	// Node added (changed from air or something else to something)
	MEET_ADDNODE,
	// Node removed (changed to air)
	MEET_REMOVENODE,
	// Node metadata of block changed (not knowing which node exactly)
	// p stores block coordinate
	MEET_BLOCK_NODE_METADATA_CHANGED,
	// Anything else (modified_blocks are set unsent)
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

class Map /*: public NodeContainer*/
{
public:

	Map(std::ostream &dout);
	virtual ~Map();

	/*virtual u16 nodeContainerId() const
	{
		return NODECONTAINER_ID_MAP;
	}*/

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
	// Same as the above (there exists no lock anymore)
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
	
	/* Server overrides */
	virtual MapBlock * emergeBlock(v3s16 p, bool allow_generate=true)
	{ return getBlockNoCreateNoEx(p); }

	// Returns InvalidPositionException if not found
	bool isNodeUnderground(v3s16 p);
	
	bool isValidPosition(v3s16 p);
	
	// throws InvalidPositionException if not found
	MapNode getNode(v3s16 p);

	// throws InvalidPositionException if not found
	void setNode(v3s16 p, MapNode & n);
	
	// Returns a CONTENT_IGNORE node if not found
	MapNode getNodeNoEx(v3s16 p);

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
			core::map<v3s16, MapBlock*> &modified_blocks, std::string &player_name);
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

	// Call these before and after saving of many blocks
	virtual void beginSave() {return;};
	virtual void endSave() {return;};
	
	virtual void save(bool only_changed){assert(0);};
	
	// Server implements this.
	// Client leaves it as no-op.
	virtual void saveBlock(MapBlock *block){};

	/*
		Updates usage timers and unloads unused blocks and sectors.
		Saves modified blocks before unloading on MAPTYPE_SERVER.
	*/
	void timerUpdate(float dtime, float unload_timeout,
			core::list<v3s16> *unloaded_blocks=NULL);
		
	// Deletes sectors and their blocks from memory
	// Takes cache into account
	// If deleted sector is in sector cache, clears cache
	void deleteSectors(core::list<v2s16> &list);

#if 0
	/*
		Unload unused data
		= flush changed to disk and delete from memory, if usage timer of
		  block is more than timeout
	*/
	void unloadUnusedData(float timeout,
			core::list<v3s16> *deleted_blocks=NULL);
#endif

	// For debug printing. Prints "Map: ", "ServerMap: " or "ClientMap: "
	virtual void PrintInfo(std::ostream &out);
	
	void transformLiquids(core::map<v3s16, MapBlock*> & modified_blocks);

	/*
		Node metadata
		These are basically coordinate wrappers to MapBlock
	*/
	
	NodeMetadata* getNodeMetadata(v3s16 p);
	void setNodeMetadata(v3s16 p, NodeMetadata *meta);
	void removeNodeMetadata(v3s16 p);
	void nodeMetadataStep(float dtime,
			core::map<v3s16, MapBlock*> &changed_blocks);
	
	/*
		Misc.
	*/
	core::map<v2s16, MapSector*> *getSectorsPtr(){return &m_sectors;}

	/*
		Variables
	*/
	
protected:

	std::ostream &m_dout;

	core::map<MapEventReceiver*, bool> m_event_receivers;
	
	core::map<v2s16, MapSector*> m_sectors;

	// Be sure to set this to NULL when the cached sector is deleted 
	MapSector *m_sector_cache;
	v2s16 m_sector_cache_p;

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
		Get a sector from somewhere.
		- Check memory
		- Check disk (doesn't load blocks)
		- Create blank one
	*/
	ServerMapSector * createSector(v2s16 p);

	/*
		Blocks are generated by using these and makeBlock().
	*/
	void initBlockMake(mapgen::BlockMakeData *data, v3s16 blockpos);
	MapBlock* finishBlockMake(mapgen::BlockMakeData *data,
			core::map<v3s16, MapBlock*> &changed_blocks);
	
	// A non-threaded wrapper to the above
	MapBlock * generateBlock(
			v3s16 p,
			core::map<v3s16, MapBlock*> &modified_blocks
	);
	
	/*
		Get a block from somewhere.
		- Memory
		- Create blank
	*/
	MapBlock * createBlock(v3s16 p);

	/*
		Forcefully get a block from somewhere.
		- Memory
		- Load from disk
		- Generate
	*/
	MapBlock * emergeBlock(v3s16 p, bool allow_generate=true);
	
	// Helper for placing objects on ground level
	s16 findGroundLevel(v2s16 p2d);

	/*
		Misc. helper functions for fiddling with directory and file
		names when saving
	*/
	void createDirs(std::string path);
	// returns something like "map/sectors/xxxxxxxx"
	std::string getSectorDir(v2s16 pos, int layout = 2);
	// dirname: final directory name
	v2s16 getSectorPos(std::string dirname);
	v3s16 getBlockPos(std::string sectordir, std::string blockfile);
	static std::string getBlockFilename(v3s16 p);

	/*
		Database functions
	*/
	// Create the database structure
	void createDatabase();
	// Verify we can read/write to the database
	void verifyDatabase();
	// Get an integer suitable for a block
	static sqlite3_int64 getBlockAsInteger(const v3s16 pos);
	static v3s16 getIntegerAsBlock(sqlite3_int64 i);

	// Returns true if the database file does not exist
	bool loadFromFolders();

	// Call these before and after saving of blocks
	void beginSave();
	void endSave();

	void save(bool only_changed);
	//void loadAll();
	
	void listAllLoadableBlocks(core::list<v3s16> &dst);
	
	// Saves map seed and possibly other stuff
	void saveMapMeta();
	void loadMapMeta();
	
	/*void saveChunkMeta();
	void loadChunkMeta();*/
	
	// The sector mutex should be locked when calling most of these
	
	// This only saves sector-specific data such as the heightmap
	// (no MapBlocks)
	// DEPRECATED? Sectors have no metadata anymore.
	void saveSectorMeta(ServerMapSector *sector);
	MapSector* loadSectorMeta(std::string dirname, bool save_after_load);
	bool loadSectorMeta(v2s16 p2d);
	
	// Full load of a sector including all blocks.
	// returns true on success, false on failure.
	bool loadSectorFull(v2s16 p2d);
	// If sector is not found in memory, try to load it from disk.
	// Returns true if sector now resides in memory
	//bool deFlushSector(v2s16 p2d);
	
	void saveBlock(MapBlock *block);
	// This will generate a sector with getSector if not found.
	void loadBlock(std::string sectordir, std::string blockfile, MapSector *sector, bool save_after_load=false);
	MapBlock* loadBlock(v3s16 p);
	// Database version
	void loadBlock(std::string *blob, v3s16 p3d, MapSector *sector, bool save_after_load=false);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);

	bool isSavingEnabled(){ return m_map_saving_enabled; }

	u64 getSeed(){ return m_seed; }

private:
	// Seed used for all kinds of randomness
	u64 m_seed;

	std::string m_savedir;
	bool m_map_saving_enabled;

#if 0
	// Chunk size in MapSectors
	// If 0, chunks are disabled.
	s16 m_chunksize;
	// Chunks
	core::map<v2s16, MapChunk*> m_chunks;
#endif

	/*
		Metadata is re-written on disk only if this is true.
		This is reset to false when written on disk.
	*/
	bool m_map_metadata_changed;
	
	/*
		SQLite database and statements
	*/
	sqlite3 *m_database;
	sqlite3_stmt *m_database_read;
	sqlite3_stmt *m_database_write;
	sqlite3_stmt *m_database_list;
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

	void updateCamera(v3f pos, v3f dir, f32 fov)
	{
		JMutexAutoLock lock(m_camera_mutex);
		m_camera_position = pos;
		m_camera_direction = dir;
		m_camera_fov = fov;
	}

	/*
		Forcefully get a sector from somewhere
	*/
	MapSector * emergeSector(v2s16 p);

	//void deSerializeSector(v2s16 p2d, std::istream &is);

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

	void renderPostFx();

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
	
	// Check if sector was drawn on last render()
	bool sectorWasDrawn(v2s16 p)
	{
		return (m_last_drawn_sectors.find(p) != NULL);
	}
	
private:
	Client *m_client;
	
	core::aabbox3d<f32> m_box;
	
	// This is the master heightmap mesh
	//scene::SMesh *mesh;
	//JMutex mesh_mutex;
	
	MapDrawControl &m_control;

	v3f m_camera_position;
	v3f m_camera_direction;
	f32 m_camera_fov;
	JMutex m_camera_mutex;
	
	core::map<v2s16, bool> m_last_drawn_sectors;
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

	void setMap(Map *map)
	{m_map = map;}
	
	virtual void emerge(VoxelArea a, s32 caller_id=-1);

	void initialEmerge(v3s16 blockpos_min, v3s16 blockpos_max);
	
	// This is much faster with big chunks of generated data
	void blitBackAll(core::map<v3s16, MapBlock*> * modified_blocks);

protected:
	bool m_create_area;
};

#endif

