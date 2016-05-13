/*
Minetest
Copyright (C) 2014 jjb, James Dornan <james@catch22.com>

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

#include "database.h"
#include "settings.h"
#include <string>

#include <postgresql/libpq-fe.h>

class ServerMap;

class Database_PostgreSQL : public Database
{
public:
	Database_PostgreSQL(Settings &conf);

	virtual void beginSave();
	virtual void endSave();

	virtual bool saveBlock(const v3s16 &pos, const std::string &data);
	virtual std::string loadBlock(const v3s16 &pos);
	virtual void listAllLoadableBlocks(std::vector<v3s16> &dst);
	virtual bool deleteBlock(const v3s16 &pos);
	virtual bool initialized();

	~Database_PostgreSQL();
private:
	PGconn* m_conn;
	int pg_version;
	std::string conn_info;

	void connectDB();
	void connectString(Settings &conf);
	void dbconnect();
	void prepare();
	bool ping();
	void verifyDatabase();
	void createDatabase();
	void prepareStatement(std::string name, std::string sql);
	PGresult* resultsCheck(PGresult* res, bool clear = true);
	PGresult* execPrepared(std::string name, int nParams, const v3s16 &pos,
		bool clear = true, const std::string &data = "");
};
