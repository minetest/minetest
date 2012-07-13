#include "gamedef.h"
#include "map.h"
#include "mapblock.h"
#include "util/thread.h"

#include <list>
#include <map>

class BlockSaver : public SimpleThread {
	ServerMap map;
	JMutex lock;
	std::list<MapBlock*> queue;
	std::map<v3s16,bool> queued;
public:

BlockSaver(const std::string& path, IGameDef* def) : map(path,def)
	{}

	u64 seed;

	void setSeed(u64 newseed) {
		JMutexAutoLock al(lock);
		map.setSeed(newseed);
	}
	
	void enqueue(MapBlock& block) {
		JMutexAutoLock al(lock);
		if(queued.find(block.getPos())!=queued.end()) return;
		queue.push_back(&block);
		queued[block.getPos()] = true;
	}

	void* Thread();
};
