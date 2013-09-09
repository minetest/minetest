#ifndef DATABASE_SQLITE3_HEADER
#define DATABASE_SQLITE3_HEADER

#include "config.h"
#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"
#include "database.h"

extern "C" {
	#include "sqlite3.h"
}

class Database_SQLite3 : public Database
{
public:
	Database_SQLite3(ServerMap *map, std::string savedir);
        virtual void beginSave();
        virtual void endSave();

        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(std::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_SQLite3();
private:
	ServerMap *srvmap;
	std::string m_savedir;
	sqlite3 *m_database;
	sqlite3_stmt *m_database_read;
	sqlite3_stmt *m_database_write;
	sqlite3_stmt *m_database_list;

	// Create the database structure
	void createDatabase();
        // Verify we can read/write to the database
        void verifyDatabase();
        void createDirs(std::string path);
};

#endif
