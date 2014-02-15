/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#ifndef EMERGE_HEADER
#define EMERGE_HEADER

#include <map>
#include "irr_v3d.h"
#include "util/container.h"
#include "map.h" // for ManualMapVoxelManipulator
#include "mapgen.h" // for MapgenParams

#define MGPARAMS_SET_MGNAME      1
#define MGPARAMS_SET_SEED        2
#define MGPARAMS_SET_WATER_LEVEL 4
#define MGPARAMS_SET_FLAGS       8

#define BLOCK_EMERGE_ALLOWGEN (1<<0)

#define EMERGE_DBG_OUT(x) \
	{ if (enable_mapgen_debug_info) \
	infostream << "EmergeThread: " x << std::endl; }

class EmergeThread;
//class Mapgen;
//struct MapgenFactory;
class Biome;
class BiomeDefManager;
class Decoration;
class Ore;
class INodeDefManager;
class Settings;

struct BlockMakeData {
	ManualMapVoxelManipulator *vmanip;
	u64 seed;
	v3s16 blockpos_min;
	v3s16 blockpos_max;
	v3s16 blockpos_requested;
	UniqueQueue<v3s16> transforming_liquid;
	INodeDefManager *nodedef;

	BlockMakeData():
		vmanip(NULL),
		seed(0),
		nodedef(NULL)
	{}

	~BlockMakeData() { delete vmanip; }
};

struct BlockEmergeData {
	u16 peer_requested;
	u8 flags;
};

class EmergeManager {
public:
	INodeDefManager *ndef;

	std::map<std::string, MapgenFactory *> mglist;

	std::vector<Mapgen *> mapgen;
	std::vector<EmergeThread *> emergethread;

	bool threads_active;

	//settings
	MapgenParams params;
	bool mapgen_debug_info;
	u16 qlimit_total;
	u16 qlimit_diskonly;
	u16 qlimit_generate;

	u32 gennotify;

	//block emerge queue data structures
	JMutex queuemutex;
	std::map<v3s16, BlockEmergeData *> blocks_enqueued;
	std::map<u16, u16> peer_queue_count;

	//Mapgen-related structures
	BiomeDefManager *biomedef;
	std::vector<Ore *> ores;
	std::vector<Decoration *> decorations;

	EmergeManager(IGameDef *gamedef);
	~EmergeManager();

	void loadMapgenParams();
	void initMapgens();
	Mapgen *getCurrentMapgen();
	Mapgen *createMapgen(std::string mgname, int mgid,
						MapgenParams *mgparams);
	MapgenSpecificParams *createMapgenParams(std::string mgname);
	void startThreads();
	void stopThreads();
	bool enqueueBlockEmerge(u16 peer_id, v3s16 p, bool allow_generate);

	void registerMapgen(std::string name, MapgenFactory *mgfactory);
	void loadParamsFromSettings(Settings *settings);
	void saveParamsToSettings(Settings *settings);

	//mapgen helper methods
	Biome *getBiomeAtPoint(v3s16 p);
	int getGroundLevelAtPoint(v2s16 p);
	bool isBlockUnderground(v3s16 blockpos);
	u32 getBlockSeed(v3s16 p);
};

#endif
