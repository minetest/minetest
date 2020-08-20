/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include <list>

#include "irrlichttypes_bloated.h"
#include "mapnode.h"
#include "constants.h"
#include "voxel.h"
#include "modifiedstate.h"
#include "util/container.h"
#include "util/metricsbackend.h"
#include "nodetimer.h"
#include "map_settings_manager.h"
#include "debug.h"

class Settings;
class MapDatabase;
class ClientMap;
class MapSector;
class ServerMapSector;
class MapBlock;
class NodeMetadata;
class IGameDef;
class IRollbackManager;
class EmergeManager;
class MetricsBackend;
class ServerEnvironment;
struct BlockMakeData;

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
	// Node swapped (changed without metadata change)
	MEET_SWAPNODE,
	// Node metadata changed
	MEET_BLOCK_NODE_METADATA_CHANGED,
	// Anything else (modified_blocks are set unsent)
	MEET_OTHER
};

struct MapEditEvent
{
	MapEditEventType type = MEET_OTHER;
	v3s16 p;
	MapNode n = CONTENT_AIR;
	std::set<v3s16> modified_blocks;
	bool is_private_change = false;

	MapEditEvent() = default;

	VoxelArea getArea() const
	{
		switch(type){
		case MEET_ADDNODE:
			return VoxelArea(p);
		case MEET_REMOVENODE:
			return VoxelArea(p);
		case MEET_SWAPNODE:
			return VoxelArea(p);
		case MEET_BLOCK_NODE_METADATA_CHANGED:
		{
			v3s16 np1 = p*MAP_BLOCKSIZE;
			v3s16 np2 = np1 + v3s16(1,1,1)*MAP_BLOCKSIZE - v3s16(1,1,1);
			return VoxelArea(np1, np2);
		}
		case MEET_OTHER:
		{
			VoxelArea a;
			for (v3s16 p : modified_blocks) {
				v3s16 np1 = p*MAP_BLOCKSIZE;
				v3s16 np2 = np1 + v3s16(1,1,1)*MAP_BLOCKSIZE - v3s16(1,1,1);
				a.addPoint(np1);
				a.addPoint(np2);
			}
			return a;
		}
		}
		return VoxelArea();
	}
};

class MapEventReceiver
{
public:
	// event shall be deleted by caller after the call.
	virtual void onMapEditEvent(const MapEditEvent &event) = 0;
};

class Map /*: public NodeContainer*/
{
public:

	Map(std::ostream &dout, IGameDef *gamedef);
	virtual ~Map();
	DISABLE_CLASS_COPY(Map);

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
	void dispatchEvent(const MapEditEvent &event);

	// On failure returns NULL
	MapSector * getSectorNoGenerateNoLock(v2s16 p2d);
	// Same as the above (there exists no lock anymore)
	MapSector * getSectorNoGenerate(v2s16 p2d);
	// Gets an existing sector or creates an empty one
	//MapSector * getSectorCreate(v2s16 p2d);

	/*
		This is overloaded by ClientMap and ServerMap to allow
		their differing fetch methods.
	*/
	virtual MapSector * emergeSector(v2s16 p){ return NULL; }

	// Returns InvalidPositionException if not found
	MapBlock * getBlockNoCreate(v3s16 p);
	// Returns NULL if not found
	MapBlock * getBlockNoCreateNoEx(v3s16 p);

	/* Server overrides */
	virtual MapBlock * emergeBlock(v3s16 p, bool create_blank=true)
	{ return getBlockNoCreateNoEx(p); }

	inline const NodeDefManager * getNodeDefManager() { return m_nodedef; }

	// Returns InvalidPositionException if not found
	bool isNodeUnderground(v3s16 p);

	bool isValidPosition(v3s16 p);

	// throws InvalidPositionException if not found
	void setNode(v3s16 p, MapNode & n);

	// Returns a CONTENT_IGNORE node if not found
	// If is_valid_position is not NULL then this will be set to true if the
	// position is valid, otherwise false
	MapNode getNode(v3s16 p, bool *is_valid_position = NULL);

	/*
		These handle lighting but not faces.
	*/
	void addNodeAndUpdate(v3s16 p, MapNode n,
			std::map<v3s16, MapBlock*> &modified_blocks,
			bool remove_metadata = true);
	void removeNodeAndUpdate(v3s16 p,
			std::map<v3s16, MapBlock*> &modified_blocks);

	/*
		Wrappers for the latter ones.
		These emit events.
		Return true if succeeded, false if not.
	*/
	bool addNodeWithEvent(v3s16 p, MapNode n, bool remove_metadata = true);
	bool removeNodeWithEvent(v3s16 p);

	// Call these before and after saving of many blocks
	virtual void beginSave() {}
	virtual void endSave() {}

	virtual void save(ModifiedState save_level) { FATAL_ERROR("FIXME"); }

	// Server implements these.
	// Client leaves them as no-op.
	virtual bool saveBlock(MapBlock *block) { return false; }
	virtual bool deleteBlock(v3s16 blockpos) { return false; }

	/*
		Updates usage timers and unloads unused blocks and sectors.
		Saves modified blocks before unloading on MAPTYPE_SERVER.
	*/
	void timerUpdate(float dtime, float unload_timeout, u32 max_loaded_blocks,
			std::vector<v3s16> *unloaded_blocks=NULL);

	/*
		Unloads all blocks with a zero refCount().
		Saves modified blocks before unloading on MAPTYPE_SERVER.
	*/
	void unloadUnreferencedBlocks(std::vector<v3s16> *unloaded_blocks=NULL);

	// Deletes sectors and their blocks from memory
	// Takes cache into account
	// If deleted sector is in sector cache, clears cache
	void deleteSectors(std::vector<v2s16> &list);

	// For debug printing. Prints "Map: ", "ServerMap: " or "ClientMap: "
	virtual void PrintInfo(std::ostream &out);

	void transformLiquids(std::map<v3s16, MapBlock*> & modified_blocks,
			ServerEnvironment *env);

	/*
		Node metadata
		These are basically coordinate wrappers to MapBlock
	*/

	std::vector<v3s16> findNodesWithMetadata(v3s16 p1, v3s16 p2);
	NodeMetadata *getNodeMetadata(v3s16 p);

	/**
	 * Sets metadata for a node.
	 * This method sets the metadata for a given node.
	 * On success, it returns @c true and the object pointed to
	 * by @p meta is then managed by the system and should
	 * not be deleted by the caller.
	 *
	 * In case of failure, the method returns @c false and the
	 * caller is still responsible for deleting the object!
	 *
	 * @param p node coordinates
	 * @param meta pointer to @c NodeMetadata object
	 * @return @c true on success, false on failure
	 */
	bool setNodeMetadata(v3s16 p, NodeMetadata *meta);
	void removeNodeMetadata(v3s16 p);

	/*
		Node Timers
		These are basically coordinate wrappers to MapBlock
	*/

	NodeTimer getNodeTimer(v3s16 p);
	void setNodeTimer(const NodeTimer &t);
	void removeNodeTimer(v3s16 p);

	/*
		Misc.
	*/
	std::map<v2s16, MapSector*> *getSectorsPtr(){return &m_sectors;}

	/*
		Variables
	*/

	void transforming_liquid_add(v3s16 p);

	bool isBlockOccluded(MapBlock *block, v3s16 cam_pos_nodes);
protected:
	friend class LuaVoxelManip;

	std::ostream &m_dout; // A bit deprecated, could be removed

	IGameDef *m_gamedef;

	std::set<MapEventReceiver*> m_event_receivers;

	std::map<v2s16, MapSector*> m_sectors;

	// Be sure to set this to NULL when the cached sector is deleted
	MapSector *m_sector_cache = nullptr;
	v2s16 m_sector_cache_p;

	// Queued transforming water nodes
	UniqueQueue<v3s16> m_transforming_liquid;

	// This stores the properties of the nodes on the map.
	const NodeDefManager *m_nodedef;

	bool determineAdditionalOcclusionCheck(const v3s16 &pos_camera,
		const core::aabbox3d<s16> &block_bounds, v3s16 &check);
	bool isOccluded(const v3s16 &pos_camera, const v3s16 &pos_target,
		float step, float stepfac, float start_offset, float end_offset,
		u32 needed_count);

private:
	f32 m_transforming_liquid_loop_count_multiplier = 1.0f;
	u32 m_unprocessed_count = 0;
	u64 m_inc_trending_up_start_time = 0; // milliseconds
	bool m_queue_size_timer_started = false;
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
	ServerMap(const std::string &savedir, IGameDef *gamedef, EmergeManager *emerge, MetricsBackend *mb);
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
	MapSector *createSector(v2s16 p);

	/*
		Blocks are generated by using these and makeBlock().
	*/
	bool blockpos_over_mapgen_limit(v3s16 p);
	bool initBlockMake(v3s16 blockpos, BlockMakeData *data);
	void finishBlockMake(BlockMakeData *data,
		std::map<v3s16, MapBlock*> *changed_blocks);

	/*
		Get a block from somewhere.
		- Memory
		- Create blank
	*/
	MapBlock *createBlock(v3s16 p);

	/*
		Forcefully get a block from somewhere.
		- Memory
		- Load from disk
		- Create blank filled with CONTENT_IGNORE

	*/
	MapBlock *emergeBlock(v3s16 p, bool create_blank=true);

	/*
		Try to get a block.
		If it does not exist in memory, add it to the emerge queue.
		- Memory
		- Emerge Queue (deferred disk or generate)
	*/
	MapBlock *getBlockOrEmerge(v3s16 p3d);

	// Helper for placing objects on ground level
	s16 findGroundLevel(v2s16 p2d);

	/*
		Misc. helper functions for fiddling with directory and file
		names when saving
	*/
	void createDirs(const std::string &path);

	/*
		Database functions
	*/
	static MapDatabase *createDatabase(const std::string &name, const std::string &savedir, Settings &conf);

	// Call these before and after saving of blocks
	void beginSave();
	void endSave();

	void save(ModifiedState save_level);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	void listAllLoadedBlocks(std::vector<v3s16> &dst);

	MapgenParams *getMapgenParams();

	bool saveBlock(MapBlock *block);
	static bool saveBlock(MapBlock *block, MapDatabase *db);
	MapBlock* loadBlock(v3s16 p);
	// Database version
	void loadBlock(std::string *blob, v3s16 p3d, MapSector *sector, bool save_after_load=false);

	bool deleteBlock(v3s16 blockpos);

	void updateVManip(v3s16 pos);

	// For debug printing
	virtual void PrintInfo(std::ostream &out);

	bool isSavingEnabled(){ return m_map_saving_enabled; }

	u64 getSeed();
	s16 getWaterLevel();

	/*!
	 * Fixes lighting in one map block.
	 * May modify other blocks as well, as light can spread
	 * out of the specified block.
	 * Returns false if the block is not generated (so nothing
	 * changed), true otherwise.
	 */
	bool repairBlockLight(v3s16 blockpos,
		std::map<v3s16, MapBlock *> *modified_blocks);

	MapSettingsManager settings_mgr;

private:
	// Emerge manager
	EmergeManager *m_emerge;

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
	bool m_map_metadata_changed = true;
	MapDatabase *dbase = nullptr;
	MapDatabase *dbase_ro = nullptr;

	MetricCounterPtr m_save_time_counter;
};


#define VMANIP_BLOCK_DATA_INEXIST     1
#define VMANIP_BLOCK_CONTAINS_CIGNORE 2

class MMVManip : public VoxelManipulator
{
public:
	MMVManip(Map *map);
	virtual ~MMVManip() = default;

	virtual void clear()
	{
		VoxelManipulator::clear();
		m_loaded_blocks.clear();
	}

	void initialEmerge(v3s16 blockpos_min, v3s16 blockpos_max,
		bool load_if_inexistent = true);

	// This is much faster with big chunks of generated data
	void blitBackAll(std::map<v3s16, MapBlock*> * modified_blocks,
		bool overwrite_generated = true);

	bool m_is_dirty = false;

protected:
	Map *m_map;
	/*
		key = blockpos
		value = flags describing the block
	*/
	std::map<v3s16, u8> m_loaded_blocks;
};
