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
#include "mapgen.h" // for MapgenParams
#include "map.h"

#define BLOCK_EMERGE_ALLOW_GEN   (1 << 0)
#define BLOCK_EMERGE_FORCE_QUEUE (1 << 1)

#define EMERGE_DBG_OUT(x) do {                         \
	if (enable_mapgen_debug_info)                      \
		infostream << "EmergeThread: " x << std::endl; \
} while (0)

class EmergeThread;
class INodeDefManager;
class Settings;

class BiomeManager;
class OreManager;
class DecorationManager;
class SchematicManager;

// Structure containing inputs/outputs for chunk generation
struct BlockMakeData {
	MMVManip *vmanip;
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

// Result from processing an item on the emerge queue
enum EmergeAction {
	EMERGE_CANCELLED,
	EMERGE_ERRORED,
	EMERGE_FROM_MEMORY,
	EMERGE_FROM_DISK,
	EMERGE_GENERATED,
};

// Callback
typedef void (*EmergeCompletionCallback)(
	v3s16 blockpos, EmergeAction action, void *param);

typedef std::vector<
	std::pair<
		EmergeCompletionCallback,
		void *
	>
> EmergeCallbackList;

struct BlockEmergeData {
	u16 peer_requested;
	u16 flags;
	EmergeCallbackList callbacks;
};

class EmergeManager {
public:
	INodeDefManager *ndef;
	bool enable_mapgen_debug_info;

	// Generation Notify
	u32 gen_notify_on;
	std::set<u32> gen_notify_on_deco_ids;

	// Map generation parameters
	MapgenParams params;

	// Managers of various map generation-related components
	BiomeManager *biomemgr;
	OreManager *oremgr;
	DecorationManager *decomgr;
	SchematicManager *schemmgr;

	// Methods
	EmergeManager(IGameDef *gamedef);
	~EmergeManager();

	void loadMapgenParams();
	void initMapgens();

	void startThreads();
	void stopThreads();
	bool isRunning();

	bool enqueueBlockEmerge(
		u16 peer_id,
		v3s16 blockpos,
		bool allow_generate,
		bool ignore_queue_limits=false);

	bool enqueueBlockEmergeEx(
		v3s16 blockpos,
		u16 peer_id,
		u16 flags,
		EmergeCompletionCallback callback,
		void *callback_param);

	v3s16 getContainingChunk(v3s16 blockpos);

	Mapgen *getCurrentMapgen();

	// Mapgen helpers methods
	Biome *getBiomeAtPoint(v3s16 p);
	int getSpawnLevelAtPoint(v2s16 p);
	int getGroundLevelAtPoint(v2s16 p);
	bool isBlockUnderground(v3s16 blockpos);

	static MapgenFactory *getMapgenFactory(const std::string &mgname);
	static void getMapgenNames(
		std::vector<const char *> *mgnames, bool include_hidden);
	static v3s16 getContainingChunk(v3s16 blockpos, s16 chunksize);

private:
	std::vector<Mapgen *> m_mapgens;
	std::vector<EmergeThread *> m_threads;
	bool m_threads_active;

	Mutex m_queue_mutex;
	std::map<v3s16, BlockEmergeData> m_blocks_enqueued;
	std::map<u16, u16> m_peer_queue_count;

	u16 m_qlimit_total;
	u16 m_qlimit_diskonly;
	u16 m_qlimit_generate;

	// Requires m_queue_mutex held
	EmergeThread *getOptimalThread();

	bool pushBlockEmergeData(
		v3s16 pos,
		u16 peer_requested,
		u16 flags,
		EmergeCompletionCallback callback,
		void *callback_param,
		bool *entry_already_exists);

	bool popBlockEmergeData(v3s16 pos, BlockEmergeData *bedata);

	friend class EmergeThread;

	DISABLE_CLASS_COPY(EmergeManager);
};

#endif
