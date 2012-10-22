#ifndef DATABASE_HEADER
#define DATABASE_HEADER

#include "config.h"
#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"

class Database;
class ServerMap;

class Database
{
public:
	virtual void beginSave()=0;
	virtual void endSave()=0;

	virtual void saveBlock(MapBlock *block)=0;
	virtual MapBlock* loadBlock(v3s16 blockpos)=0;
	long long getBlockAsInteger(const v3s16 pos);
	v3s16 getIntegerAsBlock(long long i);
	virtual void listAllLoadableBlocks(core::list<v3s16> &dst)=0;
	virtual int Initialized(void)=0;
	virtual ~Database() {};
};
#endif
