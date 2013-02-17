#ifndef EMERGE_HEADER
#define EMERGE_HEADER

#include <map>
#include <queue>
#include "util/thread.h"

#define BLOCK_EMERGE_ALLOWGEN (1<<0)

class Mapgen;
class MapgenParams;
class MapgenFactory;
class Biome;
class BiomeDefManager;
class EmergeThread;
class ManualMapVoxelManipulator;

#include "server.h"

struct BlockMakeData {
	bool no_op;
	ManualMapVoxelManipulator *vmanip;
	u64 seed;
	v3s16 blockpos_min;
	v3s16 blockpos_max;
	v3s16 blockpos_requested;
	UniqueQueue<v3s16> transforming_liquid;
	INodeDefManager *nodedef;

	BlockMakeData():
		no_op(false),
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
	std::map<std::string, MapgenFactory *> mglist;
	
	std::vector<Mapgen *> mapgen;
	std::vector<EmergeThread *> emergethread;
	
	//settings
	MapgenParams *params;
	u16 qlimit_total;
	u16 qlimit_diskonly;
	u16 qlimit_generate;
	
	//block emerge queue data structures
	JMutex queuemutex;
	std::map<v3s16, BlockEmergeData *> blocks_enqueued;
	std::map<u16, u16> peer_queue_count;

	//biome manager
	BiomeDefManager *biomedef;

	EmergeManager(IGameDef *gamedef, BiomeDefManager *bdef);
	~EmergeManager();

	void initMapgens(MapgenParams *mgparams);
	Mapgen *createMapgen(std::string mgname, int mgid,
						MapgenParams *mgparams);
	MapgenParams *createMapgenParams(std::string mgname);
	bool enqueueBlockEmerge(u16 peer_id, v3s16 p, bool allow_generate);
	
	bool registerMapgen(std::string name, MapgenFactory *mgfactory);
	MapgenParams *getParamsFromSettings(Settings *settings);
	void setParamsToSettings(Settings *settings);
	
	//mapgen helper methods
	Biome *getBiomeAtPoint(v3s16 p);
	int getGroundLevelAtPoint(v2s16 p);
	bool isBlockUnderground(v3s16 blockpos);
	u32 getBlockSeed(v3s16 p);
};

class EmergeThread : public SimpleThread
{
	Server *m_server;
	ServerMap *map;
	EmergeManager *emerge;
	Mapgen *mapgen;
	bool enable_mapgen_debug_info;
	int id;
	
public:
	Event qevent;
	std::queue<v3s16> blockqueue;
	
	EmergeThread(Server *server, int ethreadid):
		SimpleThread(),
		m_server(server),
		map(NULL),
		emerge(NULL),
		mapgen(NULL),
		id(ethreadid)
	{
		enable_mapgen_debug_info = g_settings->getBool("enable_mapgen_debug_info");
	}

	void *Thread();

	void trigger()
	{
		setRun(true);
		if(IsRunning() == false)
		{
			Start();
		}
	}

	bool popBlockEmerge(v3s16 *pos, u8 *flags);
	bool getBlockOrStartGen(v3s16 p, MapBlock **b, 
							BlockMakeData *data, bool allow_generate);
};

#endif
