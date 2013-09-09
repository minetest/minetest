#ifndef DATABASE_DUMMY_HEADER
#define DATABASE_DUMMY_HEADER

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"
#include "database.h"

class Database_Dummy : public Database
{
public:
	Database_Dummy(ServerMap *map);
	virtual void beginSave();
	virtual void endSave();
        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(std::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_Dummy();
private:
	ServerMap *srvmap;
	std::map<unsigned long long, std::string> m_database;
};
#endif
