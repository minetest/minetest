#include "config.h"

#if USE_LEVELDB
#ifndef DATABASE_LEVELDB_HEADER
#define DATABASE_LEVELDB_HEADER

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"
#include "database.h"

#include "leveldb/db.h"

class Database_LevelDB : public Database
{
public:
	Database_LevelDB(ServerMap *map, std::string savedir);
	virtual void beginSave();
	virtual void endSave();
        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(std::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_LevelDB();
private:
	ServerMap *srvmap;
	leveldb::DB* m_database;
};
#endif
#endif
