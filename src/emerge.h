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
//class ServerMap;
//class MapBlock;

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

//	BlockMakeData();
//	~BlockMakeData();
	
BlockMakeData():
	no_op(false),
	vmanip(NULL),
	seed(0),
	nodedef(NULL)
{}

~BlockMakeData()
{
	delete vmanip;
}
};

class EmergeManager {
public:
	std::map<std::string, MapgenFactory *> mglist;

	//settings
	MapgenParams *params;

	JMutex queuemutex;
	std::map<v3s16, u8> blocks_enqueued; //change to a hashtable later
	Mapgen *mapgen;
	EmergeThread *emergethread;

	//biome manager
	BiomeDefManager *biomedef;

	EmergeManager(IGameDef *gamedef, BiomeDefManager *bdef);
	~EmergeManager();

	void initMapgens(MapgenParams *mgparams);
	Mapgen *createMapgen(std::string mgname, int mgid,
						MapgenParams *mgparams, EmergeManager *emerge);
	MapgenParams *createMapgenParams(std::string mgname);
	Mapgen *getMapgen();
	bool enqueueBlockEmerge(u16 peer_id, v3s16 p, bool allow_generate);
	bool popBlockEmerge(v3s16 *pos, u8 *flags);
	
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
	
public:
	Event qevent;
	std::queue<v3s16> blockqueue;
	
	EmergeThread(Server *server):
		SimpleThread(),
		m_server(server),
		map(NULL),
		emerge(NULL),
		mapgen(NULL)
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

	bool getBlockOrStartGen(v3s16 p, MapBlock **b, 
							BlockMakeData *data, bool allow_generate);
};

#endif
